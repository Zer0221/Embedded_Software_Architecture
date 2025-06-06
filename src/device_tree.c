/**
 * @file device_tree.c
 * @brief 设备树实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/device_tree.h"
#include "common/error_handling.h"
#include "common/memory_manager.h"

#ifdef CONFIG_USE_RTOS
#include "common/rtos_api.h"
#endif

/* 设备节点链表 */
typedef struct device_node_list {
    device_node_t *node;              /**< 设备节点 */
    struct device_node_list *next;    /**< 下一个节点 */
} device_node_list_t;

/* 设备树状态 */
static struct {
    device_node_list_t *nodes;        /**< 设备节点链表 */
    uint16_t node_count;              /**< 节点数量 */
    bool initialized;                 /**< 是否已初始化 */
#ifdef CONFIG_USE_RTOS
    mutex_handle_t mutex;             /**< 互斥锁 */
#endif
} g_device_tree;

/* 内部函数声明 */
static device_node_t *device_find_node_internal(const char *name);
static int device_add_child(device_node_t *parent, device_node_t *child);
static int device_remove_child(device_node_t *parent, device_node_t *child);

/**
 * @brief 初始化设备树
 */
int device_tree_init(void) {
    // 检查是否已初始化
    if (g_device_tree.initialized) {
        return 0;  // 已经初始化，直接返回成功
    }
    
    // 初始化设备树状态
    g_device_tree.nodes = NULL;
    g_device_tree.node_count = 0;
    
#ifdef CONFIG_USE_RTOS
    // 创建互斥锁
    if (mutex_create(&g_device_tree.mutex) != 0) {
        return ERROR_MUTEX_CREATE_FAILED;
    }
#endif
    
    g_device_tree.initialized = true;
    
    return 0;
}

/**
 * @brief 注册设备节点
 */
int device_register_node(device_node_t *node) {
    device_node_list_t *new_node, *current;
    
    // 参数检查
    if (node == NULL || node->name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保设备树已初始化
    if (!g_device_tree.initialized) {
        ERROR_CHECK(device_tree_init());
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_device_tree.mutex);
#endif
    
    // 检查节点是否已注册
    if (device_find_node_internal(node->name) != NULL) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(g_device_tree.mutex);
#endif
        return ERROR_DEVICE_ALREADY_REGISTERED;
    }
    
    // 创建新链表节点
    new_node = (device_node_list_t *)mem_alloc(sizeof(device_node_list_t));
    if (new_node == NULL) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(g_device_tree.mutex);
#endif
        return ERROR_NO_MEMORY;
    }
    
    // 初始化链表节点
    new_node->node = node;
    new_node->next = NULL;
    
    // 添加到链表
    if (g_device_tree.nodes == NULL) {
        g_device_tree.nodes = new_node;
    } else {
        current = g_device_tree.nodes;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    
    g_device_tree.node_count++;
    
    // 如果有父节点，添加到父节点的子节点列表
    if (node->parent != NULL) {
        device_add_child(node->parent, node);
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_device_tree.mutex);
#endif
    
    printf("Device node registered: %s (type: %d)\n", node->name, node->type);
    
    return 0;
}

/**
 * @brief 注销设备节点
 */
int device_unregister_node(const char *name) {
    device_node_list_t *current, *prev;
    device_node_t *node;
    
    // 参数检查
    if (name == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保设备树已初始化
    if (!g_device_tree.initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_device_tree.mutex);
#endif
    
    // 查找节点
    prev = NULL;
    current = g_device_tree.nodes;
    
    while (current != NULL) {
        if (strcmp(current->node->name, name) == 0) {
            node = current->node;
            
            // 从父节点的子节点列表中移除
            if (node->parent != NULL) {
                device_remove_child(node->parent, node);
            }
            
            // 从链表中移除
            if (prev == NULL) {
                g_device_tree.nodes = current->next;
            } else {
                prev->next = current->next;
            }
            
            // 释放节点
            mem_free(current);
            g_device_tree.node_count--;
            
#ifdef CONFIG_USE_RTOS
            // 解锁互斥锁
            mutex_unlock(g_device_tree.mutex);
#endif
            
            printf("Device node unregistered: %s\n", name);
            
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_device_tree.mutex);
#endif
    
    // 未找到节点
    return ERROR_DEVICE_NOT_FOUND;
}

/**
 * @brief 查找设备节点
 */
device_node_t *device_find_node(const char *name) {
    device_node_t *node;
    
    // 参数检查
    if (name == NULL) {
        return NULL;
    }
    
    // 确保设备树已初始化
    if (!g_device_tree.initialized) {
        return NULL;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_device_tree.mutex);
#endif
    
    node = device_find_node_internal(name);
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_device_tree.mutex);
#endif
    
    return node;
}

/**
 * @brief 按类型查找设备节点
 */
int device_find_nodes_by_type(device_type_t type, device_node_t **nodes, uint8_t max_count, uint8_t *count) {
    device_node_list_t *current;
    uint8_t found = 0;
    
    // 参数检查
    if (nodes == NULL || max_count == 0 || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保设备树已初始化
    if (!g_device_tree.initialized) {
        *count = 0;
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_device_tree.mutex);
#endif
    
    // 查找节点
    current = g_device_tree.nodes;
    while (current != NULL && found < max_count) {
        if (current->node->type == type) {
            nodes[found++] = current->node;
        }
        current = current->next;
    }
    
    *count = found;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_device_tree.mutex);
#endif
    
    return 0;
}

/**
 * @brief 按兼容性字符串查找设备节点
 */
int device_find_nodes_by_compatible(const char *compatible, device_node_t **nodes, uint8_t max_count, uint8_t *count) {
    device_node_list_t *current;
    uint8_t found = 0;
    
    // 参数检查
    if (compatible == NULL || nodes == NULL || max_count == 0 || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 确保设备树已初始化
    if (!g_device_tree.initialized) {
        *count = 0;
        return ERROR_NOT_INITIALIZED;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(g_device_tree.mutex);
#endif
    
    // 查找节点
    current = g_device_tree.nodes;
    while (current != NULL && found < max_count) {
        if (current->node->compatible != NULL && 
            strcmp(current->node->compatible, compatible) == 0) {
            nodes[found++] = current->node;
        }
        current = current->next;
    }
    
    *count = found;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(g_device_tree.mutex);
#endif
    
    return 0;
}

/**
 * @brief 获取设备节点属性
 */
device_property_t *device_get_property(device_node_t *node, const char *name) {
    uint8_t i;
    
    // 参数检查
    if (node == NULL || name == NULL || node->properties == NULL) {
        return NULL;
    }
    
    // 查找属性
    for (i = 0; i < node->property_count; i++) {
        if (strcmp(node->properties[i].name, name) == 0) {
            return &node->properties[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 获取整型属性值
 */
int device_get_property_int(device_node_t *node, const char *name, int32_t *value) {
    device_property_t *prop;
    
    // 参数检查
    if (node == NULL || name == NULL || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 获取属性
    prop = device_get_property(node, name);
    if (prop == NULL) {
        return ERROR_PROPERTY_NOT_FOUND;
    }
    
    // 检查类型
    if (prop->type != PROP_TYPE_INT) {
        return ERROR_PROPERTY_TYPE_MISMATCH;
    }
    
    // 获取值
    *value = prop->value.i;
    
    return 0;
}

/**
 * @brief 获取无符号整型属性值
 */
int device_get_property_uint(device_node_t *node, const char *name, uint32_t *value) {
    device_property_t *prop;
    
    // 参数检查
    if (node == NULL || name == NULL || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 获取属性
    prop = device_get_property(node, name);
    if (prop == NULL) {
        return ERROR_PROPERTY_NOT_FOUND;
    }
    
    // 检查类型
    if (prop->type != PROP_TYPE_UINT) {
        return ERROR_PROPERTY_TYPE_MISMATCH;
    }
    
    // 获取值
    *value = prop->value.u;
    
    return 0;
}

/**
 * @brief 获取布尔型属性值
 */
int device_get_property_bool(device_node_t *node, const char *name, bool *value) {
    device_property_t *prop;
    
    // 参数检查
    if (node == NULL || name == NULL || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 获取属性
    prop = device_get_property(node, name);
    if (prop == NULL) {
        return ERROR_PROPERTY_NOT_FOUND;
    }
    
    // 检查类型
    if (prop->type != PROP_TYPE_BOOL) {
        return ERROR_PROPERTY_TYPE_MISMATCH;
    }
    
    // 获取值
    *value = prop->value.b;
    
    return 0;
}

/**
 * @brief 获取字符串属性值
 */
int device_get_property_string(device_node_t *node, const char *name, const char **value) {
    device_property_t *prop;
    
    // 参数检查
    if (node == NULL || name == NULL || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 获取属性
    prop = device_get_property(node, name);
    if (prop == NULL) {
        return ERROR_PROPERTY_NOT_FOUND;
    }
    
    // 检查类型
    if (prop->type != PROP_TYPE_STRING) {
        return ERROR_PROPERTY_TYPE_MISMATCH;
    }
    
    // 获取值
    *value = prop->value.s;
    
    return 0;
}

/**
 * @brief 设置设备状态
 */
int device_set_status(device_node_t *node, device_status_t status) {
    // 参数检查
    if (node == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    node->status = status;
    
    return 0;
}

/**
 * @brief 内部函数：查找设备节点
 * 
 * 注意：调用此函数前必须持有互斥锁
 */
static device_node_t *device_find_node_internal(const char *name) {
    device_node_list_t *current;
    
    current = g_device_tree.nodes;
    while (current != NULL) {
        if (strcmp(current->node->name, name) == 0) {
            return current->node;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 内部函数：添加子节点
 * 
 * 注意：调用此函数前必须持有互斥锁
 */
static int device_add_child(device_node_t *parent, device_node_t *child) {
    device_node_t **new_children;
    uint8_t i;
    
    // 参数检查
    if (parent == NULL || child == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 检查子节点是否已存在
    for (i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            return 0;  // 已经是子节点，无需添加
        }
    }
    
    // 扩展子节点数组
    new_children = (device_node_t **)realloc(parent->children, 
                                          (parent->child_count + 1) * sizeof(device_node_t *));
    if (new_children == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    // 添加子节点
    parent->children = new_children;
    parent->children[parent->child_count] = child;
    parent->child_count++;
    
    // 设置子节点的父节点
    child->parent = parent;
    
    return 0;
}

/**
 * @brief 内部函数：移除子节点
 * 
 * 注意：调用此函数前必须持有互斥锁
 */
static int device_remove_child(device_node_t *parent, device_node_t *child) {
    uint8_t i, j;
    
    // 参数检查
    if (parent == NULL || child == NULL || parent->children == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 查找子节点
    for (i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            // 移动后续子节点
            for (j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            
            // 更新子节点数量
            parent->child_count--;
            
            // 如果没有子节点了，释放子节点数组
            if (parent->child_count == 0) {
                free(parent->children);
                parent->children = NULL;
            }
            
            // 清除子节点的父节点引用
            child->parent = NULL;
            
            return 0;
        }
    }
    
    return ERROR_DEVICE_NOT_FOUND;
}
