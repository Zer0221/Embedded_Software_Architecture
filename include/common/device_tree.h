/**
 * @file device_tree.h
 * @brief 设备树接口定义
 *
 * 该头文件定义了设备树相关的接口，提供设备节点的注册、查找和管理功能
 */

#ifndef DEVICE_TREE_H
#define DEVICE_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"
#include "driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设备节点类型 */
typedef enum {
    DEVICE_TYPE_BUS,          /**< 总线设备 */
    DEVICE_TYPE_GPIO,         /**< GPIO设备 */
    DEVICE_TYPE_UART,         /**< UART设备 */
    DEVICE_TYPE_I2C,          /**< I2C设备 */
    DEVICE_TYPE_SPI,          /**< SPI设备 */
    DEVICE_TYPE_ADC,          /**< ADC设备 */
    DEVICE_TYPE_PWM,          /**< PWM设备 */
    DEVICE_TYPE_TIMER,        /**< 定时器设备 */
    DEVICE_TYPE_FLASH,        /**< 闪存设备 */
    DEVICE_TYPE_STORAGE,      /**< 存储设备 */
    DEVICE_TYPE_DISPLAY,      /**< 显示设备 */
    DEVICE_TYPE_INPUT,        /**< 输入设备 */
    DEVICE_TYPE_SENSOR,       /**< 传感器设备 */
    DEVICE_TYPE_ACTUATOR,     /**< 执行器设备 */
    DEVICE_TYPE_NETWORK,      /**< 网络设备 */
    DEVICE_TYPE_POWER,        /**< 电源设备 */
    DEVICE_TYPE_MISC,         /**< 杂项设备 */
    DEVICE_TYPE_CUSTOM        /**< 自定义设备 */
} device_type_t;

/* 设备节点状态 */
typedef enum {
    DEVICE_STATUS_DISABLED,   /**< 设备禁用 */
    DEVICE_STATUS_ENABLED,    /**< 设备启用 */
    DEVICE_STATUS_SUSPENDED   /**< 设备挂起 */
} device_status_t;

/* 设备属性类型 */
typedef enum {
    PROP_TYPE_INT,            /**< 整型属性 */
    PROP_TYPE_UINT,           /**< 无符号整型属性 */
    PROP_TYPE_BOOL,           /**< 布尔型属性 */
    PROP_TYPE_STRING,         /**< 字符串属性 */
    PROP_TYPE_ARRAY,          /**< 数组属性 */
    PROP_TYPE_POINTER         /**< 指针属性 */
} property_type_t;

/* 设备属性值 */
typedef union {
    int32_t i;                /**< 整型值 */
    uint32_t u;               /**< 无符号整型值 */
    bool b;                   /**< 布尔值 */
    const char *s;            /**< 字符串值 */
    struct {
        const void *data;     /**< 数组数据 */
        uint32_t size;        /**< 数组大小 */
    } a;                      /**< 数组值 */
    void *p;                  /**< 指针值 */
} property_value_t;

/* 设备属性 */
typedef struct {
    const char *name;         /**< 属性名称 */
    property_type_t type;     /**< 属性类型 */
    property_value_t value;   /**< 属性值 */
} device_property_t;

/* 设备节点 */
typedef struct device_node {
    const char *name;                 /**< 设备节点名称 */
    device_type_t type;               /**< 设备类型 */
    device_status_t status;           /**< 设备状态 */
    const char *compatible;           /**< 兼容性字符串 */
    struct device_node *parent;       /**< 父节点 */
    struct device_node **children;    /**< 子节点数组 */
    uint8_t child_count;              /**< 子节点数量 */
    device_property_t *properties;    /**< 属性数组 */
    uint8_t property_count;           /**< 属性数量 */
    driver_info_t *driver;            /**< 关联的驱动 */
    void *private_data;               /**< 私有数据 */
} device_node_t;

/**
 * @brief 初始化设备树
 * 
 * @return int 0表示成功，非0表示失败
 */
int device_tree_init(void);

/**
 * @brief 注册设备节点
 * 
 * @param node 设备节点指针
 * @return int 0表示成功，非0表示失败
 */
int device_register_node(device_node_t *node);

/**
 * @brief 注销设备节点
 * 
 * @param name 设备节点名称
 * @return int 0表示成功，非0表示失败
 */
int device_unregister_node(const char *name);

/**
 * @brief 查找设备节点
 * 
 * @param name 设备节点名称
 * @return device_node_t* 找到的设备节点指针，未找到返回NULL
 */
device_node_t *device_find_node(const char *name);

/**
 * @brief 按类型查找设备节点
 * 
 * @param type 设备类型
 * @param nodes 设备节点数组
 * @param max_count 数组最大容量
 * @param count 实际找到的节点数量
 * @return int 0表示成功，非0表示失败
 */
int device_find_nodes_by_type(device_type_t type, device_node_t **nodes, uint8_t max_count, uint8_t *count);

/**
 * @brief 按兼容性字符串查找设备节点
 * 
 * @param compatible 兼容性字符串
 * @param nodes 设备节点数组
 * @param max_count 数组最大容量
 * @param count 实际找到的节点数量
 * @return int 0表示成功，非0表示失败
 */
int device_find_nodes_by_compatible(const char *compatible, device_node_t **nodes, uint8_t max_count, uint8_t *count);

/**
 * @brief 获取设备节点属性
 * 
 * @param node 设备节点指针
 * @param name 属性名称
 * @return device_property_t* 找到的属性指针，未找到返回NULL
 */
device_property_t *device_get_property(device_node_t *node, const char *name);

/**
 * @brief 获取整型属性值
 * 
 * @param node 设备节点指针
 * @param name 属性名称
 * @param value 返回的属性值
 * @return int 0表示成功，非0表示失败
 */
int device_get_property_int(device_node_t *node, const char *name, int32_t *value);

/**
 * @brief 获取无符号整型属性值
 * 
 * @param node 设备节点指针
 * @param name 属性名称
 * @param value 返回的属性值
 * @return int 0表示成功，非0表示失败
 */
int device_get_property_uint(device_node_t *node, const char *name, uint32_t *value);

/**
 * @brief 获取布尔型属性值
 * 
 * @param node 设备节点指针
 * @param name 属性名称
 * @param value 返回的属性值
 * @return int 0表示成功，非0表示失败
 */
int device_get_property_bool(device_node_t *node, const char *name, bool *value);

/**
 * @brief 获取字符串属性值
 * 
 * @param node 设备节点指针
 * @param name 属性名称
 * @param value 返回的属性值
 * @return int 0表示成功，非0表示失败
 */
int device_get_property_string(device_node_t *node, const char *name, const char **value);

/**
 * @brief 设置设备状态
 * 
 * @param node 设备节点指针
 * @param status 设备状态
 * @return int 0表示成功，非0表示失败
 */
int device_set_status(device_node_t *node, device_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_TREE_H */
