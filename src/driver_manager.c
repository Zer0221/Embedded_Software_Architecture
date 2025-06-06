/**
 * @file driver_manager.c
 * @brief 驱动管理器实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/driver_manager.h"
#include "common/error_handling.h"
#include "common/memory_manager.h"

#ifdef CONFIG_USE_RTOS
#include "common/rtos_api.h"
#endif

/* 驱动链表节点 */
typedef struct driver_node {
    driver_info_t *driver;        /**< 驱动信息 */
    struct driver_node *next;     /**< 下一个节点 */
} driver_node_t;

/* 驱动管理器状态 */
static struct {
    driver_node_t *drivers;       /**< 驱动链表头 */
    uint8_t driver_count;         /**< 驱动数量 */
    bool initialized;             /**< 是否已初始化 */
#ifdef CONFIG_USE_RTOS
    mutex_handle_t mutex;         /**< 互斥锁 */
#endif
} g_driver_manager;

/* 自动注册驱动数组段定义 */
#if defined(CONFIG_AUTO_DRIVER_REGISTER) && (defined(COMPILER_KEIL) || defined(COMPILER_IAR))
extern driver_info_t* const __drivers_section_start;
extern driver_info_t* const __drivers_section_end;
#endif

/**
 * @brief 初始化驱动管理器
 */
int driver_manager_init(void) {
    // 检查是否已初始化
    if (g_driver_manager.initialized) {
        return 0;  // 已经初始化，直接返回成功
    }
    
    // 初始化驱动管理器状态
    g_driver_manager.drivers = NULL;
    g_driver_manager.driver_count = 0;
    
#ifdef CONFIG_USE_RTOS
    // 创建互斥锁
    if (mutex_create(&g_driver_manager.mutex) != 0) {
        return ERROR_MUTEX_CREATE_FAILED;
    }
#endif
    
    g_driver_manager.initialized = true;
    
    // 注册编译器特定段中的驱动
#if defined(CONFIG_AUTO_DRIVER_REGISTER) && (defined(COMPILER_KEIL) || defined(COMPILER_IAR))
    driver_info_t **driver_ptr = (driver_info_t **)&__drivers_section_start;
    while (driver_ptr < (driver_info_t **)&__drivers_section_end) {
        if (*driver_ptr != NULL) {
            driver_register(*driver_ptr);
        }
        driver_ptr++;
    }
#endif
    
    return 0;
}

/**
 * @brief 注册驱动
 */
int driver_register(driver_info_t *driver_info) {
    driver_node_t *node, *current;
    
    // 参数检查
    if (driver_info == NULL || driver_info->name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        ERROR_CHECK(driver_manager_init());
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 检查驱动是否已注册
    current = g_driver_manager.drivers;
    while (current != NULL) {
        if (current->driver == driver_info || 
            strcmp(current->driver->name, driver_info->name) == 0) {
            // 驱动已注册
#ifdef CONFIG_USE_RTOS
            mutex_unlock(g_driver_manager.mutex);
#endif
            return ERROR_DRIVER_ALREADY_REGISTERED;
        }
        current = current->next;
    }
    
    // 创建新节点
    node = (driver_node_t *)mem_alloc(sizeof(driver_node_t));
    if (node == NULL) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(g_driver_manager.mutex);
#endif
        return ERROR_NO_MEMORY;
    }
    
    // 初始化节点
    node->driver = driver_info;
    node->next = g_driver_manager.drivers;
    g_driver_manager.drivers = node;
    g_driver_manager.driver_count++;
    
    // 设置驱动状态
    driver_info->status = DRIVER_STATUS_UNINITIALIZED;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    printf("Driver registered: %s (%s)\n", driver_info->name, driver_info->description);
    
    return 0;
}

/**
 * @brief 注销驱动
 */
int driver_unregister(const char *name) {
    driver_node_t *current, *prev;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 查找驱动节点
    prev = NULL;
    current = g_driver_manager.drivers;
    
    while (current != NULL) {
        if (strcmp(current->driver->name, name) == 0) {
            // 检查驱动状态
            if (current->driver->status == DRIVER_STATUS_RUNNING) {
                // 驱动正在运行，先停止
                if (current->driver->deinit != NULL) {
                    current->driver->deinit();
                }
            }
            
            // 从链表中移除
            if (prev == NULL) {
                g_driver_manager.drivers = current->next;
            } else {
                prev->next = current->next;
            }
            
            // 释放节点
            mem_free(current);
            g_driver_manager.driver_count--;
            
#ifdef CONFIG_USE_RTOS
            // 解锁互斥锁
            mutex_unlock(g_driver_manager.mutex);
#endif
            
            printf("Driver unregistered: %s\n", name);
            
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    // 未找到驱动
    return ERROR_DRIVER_NOT_FOUND;
}

/**
 * @brief 查找驱动
 */
driver_info_t *driver_find(const char *name) {
    driver_node_t *current;
    
    // 参数检查
    if (name == NULL) {
        return NULL;
    }
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        return NULL;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 查找驱动
    current = g_driver_manager.drivers;
    while (current != NULL) {
        if (strcmp(current->driver->name, name) == 0) {
            // 找到驱动
#ifdef CONFIG_USE_RTOS
            mutex_unlock(g_driver_manager.mutex);
#endif
            return current->driver;
        }
        current = current->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    // 未找到驱动
    return NULL;
}

/**
 * @brief 按类型查找驱动
 */
int driver_find_by_type(driver_type_t type, driver_info_t **drivers, uint8_t max_count, uint8_t *count) {
    driver_node_t *current;
    uint8_t found = 0;
    
    // 参数检查
    if (drivers == NULL || max_count == 0 || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        *count = 0;
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 查找驱动
    current = g_driver_manager.drivers;
    while (current != NULL && found < max_count) {
        if (current->driver->type == type) {
            // 添加到结果数组
            drivers[found++] = current->driver;
        }
        current = current->next;
    }
    
    *count = found;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    return 0;
}

/**
 * @brief 获取所有已注册的驱动
 */
int driver_get_all(driver_info_t **drivers, uint8_t max_count, uint8_t *count) {
    driver_node_t *current;
    uint8_t index = 0;
    
    // 参数检查
    if (drivers == NULL || max_count == 0 || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        *count = 0;
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 获取所有驱动
    current = g_driver_manager.drivers;
    while (current != NULL && index < max_count) {
        drivers[index++] = current->driver;
        current = current->next;
    }
    
    *count = index;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    return 0;
}

/**
 * @brief 初始化所有驱动
 */
int driver_init_all(void) {
    driver_node_t *current;
    int ret;
    
    // 确保驱动管理器已初始化
    if (!g_driver_manager.initialized) {
        ERROR_CHECK(driver_manager_init());
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_driver_manager.mutex);
#endif
    
    // 初始化所有驱动
    current = g_driver_manager.drivers;
    while (current != NULL) {
        if (current->driver->status == DRIVER_STATUS_UNINITIALIZED && 
            current->driver->init != NULL) {
            
            printf("Initializing driver: %s\n", current->driver->name);
            
            // 调用驱动初始化函数
            ret = current->driver->init();
            if (ret != 0) {
                printf("Failed to initialize driver %s: error %d\n", 
                       current->driver->name, ret);
                current->driver->status = DRIVER_STATUS_ERROR;
            } else {
                current->driver->status = DRIVER_STATUS_RUNNING;
            }
        }
        
        current = current->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_driver_manager.mutex);
#endif
    
    return 0;
}
