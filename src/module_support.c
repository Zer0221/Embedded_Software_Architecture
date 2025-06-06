/**
 * @file module_support.c
 * @brief 模块化支持实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/module_support.h"
#include "common/error_handling.h"
#include "common/memory_manager.h"

#ifdef CONFIG_USE_RTOS
#include "common/rtos_api.h"
#endif

/* 模块链表节点 */
typedef struct module_node {
    module_info_t *module;         /**< 模块信息 */
    struct module_node *next;      /**< 下一个节点 */
} module_node_t;

/* 模块系统状态 */
static struct {
    module_node_t *modules;        /**< 模块链表头 */
    uint8_t module_count;          /**< 模块数量 */
    bool initialized;              /**< 是否已初始化 */
#ifdef CONFIG_USE_RTOS
    mutex_handle_t mutex;          /**< 互斥锁 */
#endif
} g_module_system;

/* 自动注册模块数组段定义 */
#if defined(CONFIG_AUTO_MODULE_REGISTER) && (defined(COMPILER_KEIL) || defined(COMPILER_IAR))
extern module_info_t* const __modules_section_start;
extern module_info_t* const __modules_section_end;
#endif

/* 内部函数声明 */
static module_info_t *module_find_internal(const char *name);
static int module_sort_by_priority(module_info_t **modules, uint8_t count);
static int module_check_dependency_cycle(module_info_t *module, module_info_t **path, uint8_t path_len);
static int module_get_dependency_chain(module_info_t *module, module_info_t **chain, uint8_t max_count, uint8_t *count);

/**
 * @brief 初始化模块系统
 */
int module_system_init(void) {
    // 检查是否已初始化
    if (g_module_system.initialized) {
        return 0;  // 已经初始化，直接返回成功
    }
    
    // 初始化模块系统状态
    g_module_system.modules = NULL;
    g_module_system.module_count = 0;
    
#ifdef CONFIG_USE_RTOS
    // 创建互斥锁
    if (mutex_create(&g_module_system.mutex) != 0) {
        return ERROR_MUTEX_CREATE_FAILED;
    }
#endif
    
    g_module_system.initialized = true;
    
    // 注册编译器特定段中的模块
#if defined(CONFIG_AUTO_MODULE_REGISTER) && (defined(COMPILER_KEIL) || defined(COMPILER_IAR))
    module_info_t **module_ptr = (module_info_t **)&__modules_section_start;
    while (module_ptr < (module_info_t **)&__modules_section_end) {
        if (*module_ptr != NULL) {
            module_register(*module_ptr);
        }
        module_ptr++;
    }
#endif
    
    return 0;
}

/**
 * @brief 注册模块
 */
int module_register(module_info_t *module) {
    module_node_t *node, *current;
    
    // 参数检查
    if (module == NULL || module->name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        ERROR_CHECK(module_system_init());
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_module_system.mutex);
#endif
    
    // 检查模块是否已注册
    if (module_find_internal(module->name) != NULL) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(g_module_system.mutex);
#endif
        return ERROR_MODULE_ALREADY_REGISTERED;
    }
    
    // 创建新节点
    node = (module_node_t *)mem_alloc(sizeof(module_node_t));
    if (node == NULL) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(g_module_system.mutex);
#endif
        return ERROR_NO_MEMORY;
    }
    
    // 初始化节点
    node->module = module;
    
    // 按优先级插入链表
    if (g_module_system.modules == NULL || 
        module->priority <= g_module_system.modules->module->priority) {
        // 插入到链表头部
        node->next = g_module_system.modules;
        g_module_system.modules = node;
    } else {
        // 查找合适的位置插入
        current = g_module_system.modules;
        while (current->next != NULL && 
               module->priority > current->next->module->priority) {
            current = current->next;
        }
        node->next = current->next;
        current->next = node;
    }
    
    g_module_system.module_count++;
    
    // 设置模块状态
    module->status = MODULE_STATUS_UNINITIALIZED;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_module_system.mutex);
#endif
    
    printf("Module registered: %s (%s)\n", module->name, module->description);
    
    return 0;
}

/**
 * @brief 注销模块
 */
int module_unregister(const char *name) {
    module_node_t *current, *prev;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_module_system.mutex);
#endif
    
    // 查找模块节点
    prev = NULL;
    current = g_module_system.modules;
    
    while (current != NULL) {
        if (strcmp(current->module->name, name) == 0) {
            // 检查模块状态
            if (current->module->status == MODULE_STATUS_RUNNING) {
                // 模块正在运行，先停止
                if (current->module->interface.stop != NULL) {
                    current->module->interface.stop();
                }
                // 去初始化模块
                if (current->module->interface.deinit != NULL) {
                    current->module->interface.deinit();
                }
            } else if (current->module->status == MODULE_STATUS_INITIALIZED) {
                // 模块已初始化但未运行，直接去初始化
                if (current->module->interface.deinit != NULL) {
                    current->module->interface.deinit();
                }
            }
            
            // 从链表中移除
            if (prev == NULL) {
                g_module_system.modules = current->next;
            } else {
                prev->next = current->next;
            }
            
            // 释放节点
            mem_free(current);
            g_module_system.module_count--;
            
#ifdef CONFIG_USE_RTOS
            // 解锁互斥锁
            mutex_unlock(g_module_system.mutex);
#endif
            
            printf("Module unregistered: %s\n", name);
            
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_module_system.mutex);
#endif
    
    // 未找到模块
    return ERROR_MODULE_NOT_FOUND;
}

/**
 * @brief 查找模块
 */
module_info_t *module_find(const char *name) {
    module_info_t *module;
    
    // 参数检查
    if (name == NULL) {
        return NULL;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return NULL;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_module_system.mutex);
#endif
    
    module = module_find_internal(name);
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_module_system.mutex);
#endif
    
    return module;
}

/**
 * @brief 初始化所有模块
 */
int module_init_all(void) {
    module_info_t **modules;
    int ret, result = 0;
    uint8_t i, count;
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        ERROR_CHECK(module_system_init());
    }
    
    // 没有模块时直接返回
    if (g_module_system.module_count == 0) {
        return 0;
    }
    
    // 分配模块数组
    modules = (module_info_t **)mem_alloc(g_module_system.module_count * sizeof(module_info_t *));
    if (modules == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    // 获取所有模块
    ERROR_CHECK_GOTO(module_get_all(modules, g_module_system.module_count, &count), cleanup);
    
    // 检查依赖循环
    for (i = 0; i < count; i++) {
        module_info_t *path[CONFIG_MAX_MODULE_DEPENDENCY_DEPTH];
        ret = module_check_dependency_cycle(modules[i], path, 0);
        if (ret != 0) {
            printf("Error: Dependency cycle detected for module %s\n", modules[i]->name);
            result = ERROR_MODULE_DEPENDENCY_CYCLE;
            goto cleanup;
        }
    }
    
    // 按优先级排序模块
    module_sort_by_priority(modules, count);
    
    // 初始化模块
    for (i = 0; i < count; i++) {
        if (modules[i]->status == MODULE_STATUS_UNINITIALIZED) {
            // 检查依赖
            ret = module_check_dependencies(modules[i]->name);
            if (ret != 0) {
                printf("Warning: Cannot initialize module %s: dependencies not satisfied\n", 
                       modules[i]->name);
                result = ret;
                continue;
            }
            
            // 初始化模块
            printf("Initializing module: %s\n", modules[i]->name);
            if (modules[i]->interface.init != NULL) {
                ret = modules[i]->interface.init();
                if (ret != 0) {
                    printf("Failed to initialize module %s: error %d\n", 
                           modules[i]->name, ret);
                    modules[i]->status = MODULE_STATUS_ERROR;
                    result = ret;
                } else {
                    modules[i]->status = MODULE_STATUS_INITIALIZED;
                }
            } else {
                // 没有初始化函数，直接标记为已初始化
                modules[i]->status = MODULE_STATUS_INITIALIZED;
            }
        }
    }
    
cleanup:
    // 释放模块数组
    mem_free(modules);
    
    return result;
}

/**
 * @brief 启动所有模块
 */
int module_start_all(void) {
    module_info_t **modules;
    int ret, result = 0;
    uint8_t i, count;
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 没有模块时直接返回
    if (g_module_system.module_count == 0) {
        return 0;
    }
    
    // 分配模块数组
    modules = (module_info_t **)mem_alloc(g_module_system.module_count * sizeof(module_info_t *));
    if (modules == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    // 获取所有模块
    ERROR_CHECK_GOTO(module_get_all(modules, g_module_system.module_count, &count), cleanup);
    
    // 按优先级排序模块
    module_sort_by_priority(modules, count);
    
    // 启动模块
    for (i = 0; i < count; i++) {
        if (modules[i]->status == MODULE_STATUS_INITIALIZED) {
            // 启动模块
            printf("Starting module: %s\n", modules[i]->name);
            if (modules[i]->interface.start != NULL) {
                ret = modules[i]->interface.start();
                if (ret != 0) {
                    printf("Failed to start module %s: error %d\n", 
                           modules[i]->name, ret);
                    result = ret;
                } else {
                    modules[i]->status = MODULE_STATUS_RUNNING;
                }
            } else {
                // 没有启动函数，直接标记为运行中
                modules[i]->status = MODULE_STATUS_RUNNING;
            }
        }
    }
    
cleanup:
    // 释放模块数组
    mem_free(modules);
    
    return result;
}

/**
 * @brief 停止所有模块
 */
int module_stop_all(void) {
    module_info_t **modules;
    int ret, result = 0;
    uint8_t i, count;
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 没有模块时直接返回
    if (g_module_system.module_count == 0) {
        return 0;
    }
    
    // 分配模块数组
    modules = (module_info_t **)mem_alloc(g_module_system.module_count * sizeof(module_info_t *));
    if (modules == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    // 获取所有模块
    ERROR_CHECK_GOTO(module_get_all(modules, g_module_system.module_count, &count), cleanup);
    
    // 按优先级反序排序模块（高优先级的模块最后停止）
    module_sort_by_priority(modules, count);
    
    // 反转模块数组
    for (i = 0; i < count / 2; i++) {
        module_info_t *temp = modules[i];
        modules[i] = modules[count - i - 1];
        modules[count - i - 1] = temp;
    }
    
    // 停止模块
    for (i = 0; i < count; i++) {
        if (modules[i]->status == MODULE_STATUS_RUNNING) {
            // 停止模块
            printf("Stopping module: %s\n", modules[i]->name);
            if (modules[i]->interface.stop != NULL) {
                ret = modules[i]->interface.stop();
                if (ret != 0) {
                    printf("Failed to stop module %s: error %d\n", 
                           modules[i]->name, ret);
                    result = ret;
                } else {
                    modules[i]->status = MODULE_STATUS_INITIALIZED;
                }
            } else {
                // 没有停止函数，直接标记为已初始化
                modules[i]->status = MODULE_STATUS_INITIALIZED;
            }
        }
    }
    
cleanup:
    // 释放模块数组
    mem_free(modules);
    
    return result;
}

/**
 * @brief 初始化指定模块
 */
int module_init(const char *name) {
    module_info_t *module;
    int ret;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        ERROR_CHECK(module_system_init());
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查模块状态
    if (module->status != MODULE_STATUS_UNINITIALIZED) {
        // 已经初始化过，直接返回成功
        return 0;
    }
    
    // 检查依赖
    ret = module_check_dependencies(name);
    if (ret != 0) {
        return ret;
    }
    
    // 初始化模块
    printf("Initializing module: %s\n", module->name);
    if (module->interface.init != NULL) {
        ret = module->interface.init();
        if (ret != 0) {
            printf("Failed to initialize module %s: error %d\n", 
                   module->name, ret);
            module->status = MODULE_STATUS_ERROR;
            return ret;
        }
    }
    
    module->status = MODULE_STATUS_INITIALIZED;
    
    return 0;
}

/**
 * @brief 启动指定模块
 */
int module_start(const char *name) {
    module_info_t *module;
    int ret;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查模块状态
    if (module->status == MODULE_STATUS_UNINITIALIZED) {
        // 需要先初始化
        ret = module_init(name);
        if (ret != 0) {
            return ret;
        }
    } else if (module->status == MODULE_STATUS_RUNNING) {
        // 已经在运行，直接返回成功
        return 0;
    } else if (module->status == MODULE_STATUS_ERROR) {
        // 模块处于错误状态，不能启动
        return ERROR_MODULE_IN_ERROR_STATE;
    }
    
    // 启动模块
    printf("Starting module: %s\n", module->name);
    if (module->interface.start != NULL) {
        ret = module->interface.start();
        if (ret != 0) {
            printf("Failed to start module %s: error %d\n", 
                   module->name, ret);
            return ret;
        }
    }
    
    module->status = MODULE_STATUS_RUNNING;
    
    return 0;
}

/**
 * @brief 停止指定模块
 */
int module_stop(const char *name) {
    module_info_t *module;
    int ret;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查模块状态
    if (module->status != MODULE_STATUS_RUNNING) {
        // 不在运行状态，直接返回成功
        return 0;
    }
    
    // 停止模块
    printf("Stopping module: %s\n", module->name);
    if (module->interface.stop != NULL) {
        ret = module->interface.stop();
        if (ret != 0) {
            printf("Failed to stop module %s: error %d\n", 
                   module->name, ret);
            return ret;
        }
    }
    
    module->status = MODULE_STATUS_INITIALIZED;
    
    return 0;
}

/**
 * @brief 挂起指定模块
 */
int module_suspend(const char *name) {
    module_info_t *module;
    int ret;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查模块状态
    if (module->status != MODULE_STATUS_RUNNING) {
        // 不在运行状态，不能挂起
        return ERROR_MODULE_NOT_RUNNING;
    }
    
    // 挂起模块
    printf("Suspending module: %s\n", module->name);
    if (module->interface.suspend != NULL) {
        ret = module->interface.suspend();
        if (ret != 0) {
            printf("Failed to suspend module %s: error %d\n", 
                   module->name, ret);
            return ret;
        }
    }
    
    module->status = MODULE_STATUS_SUSPENDED;
    
    return 0;
}

/**
 * @brief 恢复指定模块
 */
int module_resume(const char *name) {
    module_info_t *module;
    int ret;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查模块状态
    if (module->status != MODULE_STATUS_SUSPENDED) {
        // 不在挂起状态，不能恢复
        return ERROR_MODULE_NOT_SUSPENDED;
    }
    
    // 恢复模块
    printf("Resuming module: %s\n", module->name);
    if (module->interface.resume != NULL) {
        ret = module->interface.resume();
        if (ret != 0) {
            printf("Failed to resume module %s: error %d\n", 
                   module->name, ret);
            return ret;
        }
    }
    
    module->status = MODULE_STATUS_RUNNING;
    
    return 0;
}

/**
 * @brief 获取模块状态
 */
int module_get_status(const char *name, module_status_t *status) {
    module_info_t *module;
    
    // 参数检查
    if (name == NULL || status == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 获取状态
    *status = module->status;
    
    return 0;
}

/**
 * @brief 获取所有模块信息
 */
int module_get_all(module_info_t **modules, uint8_t max_count, uint8_t *count) {
    module_node_t *current;
    uint8_t index = 0;
    
    // 参数检查
    if (modules == NULL || max_count == 0 || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        *count = 0;
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_module_system.mutex);
#endif
    
    // 获取所有模块
    current = g_module_system.modules;
    while (current != NULL && index < max_count) {
        modules[index++] = current->module;
        current = current->next;
    }
    
    *count = index;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_module_system.mutex);
#endif
    
    return 0;
}

/**
 * @brief 检查模块依赖关系
 */
int module_check_dependencies(const char *name) {
    module_info_t *module;
    module_info_t *dep_module;
    uint8_t i;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保模块系统已初始化
    if (!g_module_system.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // 查找模块
    module = module_find(name);
    if (module == NULL) {
        return ERROR_MODULE_NOT_FOUND;
    }
    
    // 检查依赖
    for (i = 0; i < module->dependency_count; i++) {
        // 查找依赖模块
        dep_module = module_find(module->dependencies[i].name);
        
        if (dep_module == NULL) {
            // 依赖模块不存在
            if (module->dependencies[i].optional) {
                // 可选依赖，忽略
                continue;
            } else {
                // 必需依赖，返回错误
                printf("Dependency not found: %s requires %s\n", 
                       module->name, module->dependencies[i].name);
                return ERROR_MODULE_DEPENDENCY_NOT_FOUND;
            }
        }
        
        // 检查依赖模块状态
        if (dep_module->status != MODULE_STATUS_INITIALIZED && 
            dep_module->status != MODULE_STATUS_RUNNING) {
            
            if (module->dependencies[i].optional) {
                // 可选依赖，忽略
                continue;
            } else {
                // 必需依赖，返回错误
                printf("Dependency not initialized: %s requires %s\n", 
                       module->name, module->dependencies[i].name);
                return ERROR_MODULE_DEPENDENCY_NOT_INITIALIZED;
            }
        }
    }
    
    return 0;
}

/**
 * @brief 内部函数：查找模块
 * 
 * 注意：调用此函数前必须持有互斥锁
 */
static module_info_t *module_find_internal(const char *name) {
    module_node_t *current;
    
    current = g_module_system.modules;
    while (current != NULL) {
        if (strcmp(current->module->name, name) == 0) {
            return current->module;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 内部函数：按优先级排序模块
 */
static int module_sort_by_priority(module_info_t **modules, uint8_t count) {
    uint8_t i, j;
    module_info_t *temp;
    
    // 冒泡排序
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (modules[j]->priority > modules[j + 1]->priority) {
                temp = modules[j];
                modules[j] = modules[j + 1];
                modules[j + 1] = temp;
            }
        }
    }
    
    return 0;
}

/**
 * @brief 内部函数：检查依赖循环
 */
static int module_check_dependency_cycle(module_info_t *module, module_info_t **path, uint8_t path_len) {
    uint8_t i, j;
    module_info_t *dep_module;
    
    // 检查是否已在路径中
    for (i = 0; i < path_len; i++) {
        if (path[i] == module) {
            // 检测到循环
            printf("Dependency cycle detected: ");
            for (j = i; j < path_len; j++) {
                printf("%s -> ", path[j]->name);
            }
            printf("%s\n", module->name);
            return ERROR_MODULE_DEPENDENCY_CYCLE;
        }
    }
    
    // 将当前模块添加到路径
    path[path_len++] = module;
    
    // 检查所有依赖
    for (i = 0; i < module->dependency_count; i++) {
        // 查找依赖模块
        dep_module = module_find_internal(module->dependencies[i].name);
        
        if (dep_module != NULL) {
            // 递归检查依赖
            int ret = module_check_dependency_cycle(dep_module, path, path_len);
            if (ret != 0) {
                return ret;
            }
        }
    }
    
    return 0;
}

/**
 * @brief 内部函数：获取依赖链
 */
static int module_get_dependency_chain(module_info_t *module, module_info_t **chain, uint8_t max_count, uint8_t *count) {
    uint8_t i, found = 0;
    module_info_t *dep_module;
    
    // 添加当前模块
    if (found < max_count) {
        chain[found++] = module;
    } else {
        return ERROR_BUFFER_TOO_SMALL;
    }
    
    // 添加所有依赖
    for (i = 0; i < module->dependency_count; i++) {
        // 查找依赖模块
        dep_module = module_find_internal(module->dependencies[i].name);
        
        if (dep_module != NULL) {
            // 递归获取依赖链
            uint8_t dep_count = 0;
            int ret = module_get_dependency_chain(dep_module, 
                                               chain + found, 
                                               max_count - found, 
                                               &dep_count);
            if (ret != 0) {
                return ret;
            }
            
            found += dep_count;
        }
    }
    
    *count = found;
    
    return 0;
}
