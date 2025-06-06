/**
 * @file bluetooth_api.h
 * @brief 蓝牙接口定义
 *
 * 该头文件定义了蓝牙相关的统一接口，支持经典蓝牙和低功耗蓝牙(BLE)
 */

#ifndef BLUETOOTH_API_H
#define BLUETOOTH_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 蓝牙句柄 */
typedef void* bt_handle_t;

/* 蓝牙角色 */
typedef enum {
    BT_ROLE_PERIPHERAL = 0,     /**< 外围设备角色（从设备） */
    BT_ROLE_CENTRAL,            /**< 中心设备角色（主设备） */
    BT_ROLE_OBSERVER,           /**< 观察者角色 */
    BT_ROLE_BROADCASTER         /**< 广播者角色 */
} bt_role_t;

/* 蓝牙模式 */
typedef enum {
    BT_MODE_CLASSIC = 0,        /**< 经典蓝牙模式 */
    BT_MODE_BLE,                /**< 低功耗蓝牙模式 */
    BT_MODE_DUAL                /**< 双模式(经典+BLE) */
} bt_mode_t;

/* 蓝牙地址类型 */
typedef enum {
    BT_ADDR_TYPE_PUBLIC = 0,    /**< 公共地址 */
    BT_ADDR_TYPE_RANDOM,        /**< 随机地址 */
    BT_ADDR_TYPE_RPA_PUBLIC,    /**< 可解析公共地址 */
    BT_ADDR_TYPE_RPA_RANDOM     /**< 可解析随机地址 */
} bt_addr_type_t;

/* 蓝牙地址 */
typedef struct {
    uint8_t addr[6];            /**< 蓝牙地址 */
    bt_addr_type_t type;        /**< 地址类型 */
} bt_addr_t;

/* 蓝牙扫描参数 */
typedef struct {
    uint16_t interval;          /**< 扫描间隔 (单位: 0.625ms) */
    uint16_t window;            /**< 扫描窗口 (单位: 0.625ms) */
    uint16_t timeout;           /**< 扫描超时 (单位: 秒)，0表示不超时 */
    bool active;                /**< 是否主动扫描 */
    bool filter_dup;            /**< 是否过滤重复广播 */
} bt_scan_params_t;

/* 蓝牙广播参数 */
typedef struct {
    uint16_t interval_min;      /**< 最小广播间隔 (单位: 0.625ms) */
    uint16_t interval_max;      /**< 最大广播间隔 (单位: 0.625ms) */
    uint8_t type;               /**< 广播类型 */
    uint8_t addr_type;          /**< 地址类型 */
    uint8_t channel_map;        /**< 广播信道映射 */
    uint8_t filter_policy;      /**< 过滤策略 */
} bt_adv_params_t;

/* 蓝牙连接参数 */
typedef struct {
    uint16_t interval_min;      /**< 最小连接间隔 (单位: 1.25ms) */
    uint16_t interval_max;      /**< 最大连接间隔 (单位: 1.25ms) */
    uint16_t latency;           /**< 从设备延迟 */
    uint16_t timeout;           /**< 监督超时 (单位: 10ms) */
} bt_conn_params_t;

/* 蓝牙特性属性 */
typedef struct {
    uint16_t handle;            /**< 特性句柄 */
    uint8_t properties;         /**< 特性属性 */
    uint16_t value_handle;      /**< 值句柄 */
    uint16_t user_desc_handle;  /**< 用户描述符句柄 */
    uint16_t cccd_handle;       /**< 客户端特性配置描述符句柄 */
    uint16_t sccd_handle;       /**< 服务器特性配置描述符句柄 */
} bt_gatt_char_t;

/* 蓝牙服务属性 */
typedef struct {
    uint16_t start_handle;      /**< 服务起始句柄 */
    uint16_t end_handle;        /**< 服务结束句柄 */
    uint8_t uuid[16];           /**< 服务UUID */
    uint8_t uuid_len;           /**< UUID长度 */
} bt_gatt_service_t;

/* 蓝牙回调函数类型 */
typedef void (*bt_scan_cb_t)(const bt_addr_t *addr, int8_t rssi, uint8_t *adv_data, uint8_t adv_len);
typedef void (*bt_conn_cb_t)(uint16_t conn_handle, uint8_t status);
typedef void (*bt_disc_cb_t)(uint16_t conn_handle, uint8_t reason);
typedef void (*bt_data_cb_t)(uint16_t conn_handle, uint16_t attr_handle, uint8_t *data, uint16_t len);
typedef void (*bt_mtu_cb_t)(uint16_t conn_handle, uint16_t mtu);

/* 蓝牙回调函数集 */
typedef struct {
    bt_scan_cb_t scan_cb;        /**< 扫描结果回调 */
    bt_conn_cb_t connect_cb;     /**< 连接回调 */
    bt_disc_cb_t disconnect_cb;  /**< 断开连接回调 */
    bt_data_cb_t data_cb;        /**< 数据接收回调 */
    bt_mtu_cb_t  mtu_cb;         /**< MTU更新回调 */
} bt_callbacks_t;

/* 蓝牙配置 */
typedef struct {
    bt_mode_t mode;              /**< 蓝牙模式 */
    bt_role_t role;              /**< 蓝牙角色 */
    const char *device_name;     /**< 设备名称 */
    uint8_t *adv_data;           /**< 广播数据 */
    uint8_t adv_len;             /**< 广播数据长度 */
    uint8_t *scan_rsp_data;      /**< 扫描响应数据 */
    uint8_t scan_rsp_len;        /**< 扫描响应数据长度 */
    bt_callbacks_t callbacks;    /**< 回调函数集 */
} bt_config_t;

/**
 * @brief 初始化蓝牙
 * 
 * @param config 蓝牙配置
 * @return bt_handle_t 蓝牙句柄，NULL表示失败
 */
bt_handle_t bt_init(bt_config_t *config);

/**
 * @brief 释放蓝牙
 * 
 * @param handle 蓝牙句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_deinit(bt_handle_t handle);

/**
 * @brief 启动扫描
 * 
 * @param handle 蓝牙句柄
 * @param params 扫描参数，NULL表示使用默认参数
 * @return int 0表示成功，负值表示失败
 */
int bt_start_scan(bt_handle_t handle, bt_scan_params_t *params);

/**
 * @brief 停止扫描
 * 
 * @param handle 蓝牙句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_stop_scan(bt_handle_t handle);

/**
 * @brief 启动广播
 * 
 * @param handle 蓝牙句柄
 * @param params 广播参数，NULL表示使用默认参数
 * @return int 0表示成功，负值表示失败
 */
int bt_start_adv(bt_handle_t handle, bt_adv_params_t *params);

/**
 * @brief 停止广播
 * 
 * @param handle 蓝牙句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_stop_adv(bt_handle_t handle);

/**
 * @brief 连接设备
 * 
 * @param handle 蓝牙句柄
 * @param addr 设备地址
 * @param params 连接参数，NULL表示使用默认参数
 * @return int 连接句柄，负值表示失败
 */
int bt_connect(bt_handle_t handle, bt_addr_t *addr, bt_conn_params_t *params);

/**
 * @brief 断开连接
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_disconnect(bt_handle_t handle, uint16_t conn_handle);

/**
 * @brief 发现服务
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_discover_services(bt_handle_t handle, uint16_t conn_handle);

/**
 * @brief 读取特性值
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @param attr_handle 特性值句柄
 * @return int 0表示成功，负值表示失败
 */
int bt_read_characteristic(bt_handle_t handle, uint16_t conn_handle, uint16_t attr_handle);

/**
 * @brief 写入特性值
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @param attr_handle 特性值句柄
 * @param data 数据
 * @param len 数据长度
 * @param response 是否需要响应
 * @return int 0表示成功，负值表示失败
 */
int bt_write_characteristic(bt_handle_t handle, uint16_t conn_handle, uint16_t attr_handle, const uint8_t *data, uint16_t len, bool response);

/**
 * @brief 设置本地属性值
 * 
 * @param handle 蓝牙句柄
 * @param attr_handle 属性句柄
 * @param data 数据
 * @param len 数据长度
 * @return int 0表示成功，负值表示失败
 */
int bt_set_attribute_value(bt_handle_t handle, uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief 启用通知
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @param attr_handle 特性值句柄
 * @param enable 是否启用
 * @return int 0表示成功，负值表示失败
 */
int bt_enable_notification(bt_handle_t handle, uint16_t conn_handle, uint16_t attr_handle, bool enable);

/**
 * @brief 更新MTU
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @param mtu 期望的MTU大小
 * @return int 0表示成功，负值表示失败
 */
int bt_update_mtu(bt_handle_t handle, uint16_t conn_handle, uint16_t mtu);

/**
 * @brief 获取蓝牙地址
 * 
 * @param handle 蓝牙句柄
 * @param addr 蓝牙地址
 * @return int 0表示成功，负值表示失败
 */
int bt_get_address(bt_handle_t handle, bt_addr_t *addr);

/**
 * @brief 设置蓝牙地址
 * 
 * @param handle 蓝牙句柄
 * @param addr 蓝牙地址
 * @return int 0表示成功，负值表示失败
 */
int bt_set_address(bt_handle_t handle, bt_addr_t *addr);

/**
 * @brief 设置蓝牙发射功率
 * 
 * @param handle 蓝牙句柄
 * @param tx_power 发射功率(dBm)
 * @return int 0表示成功，负值表示失败
 */
int bt_set_tx_power(bt_handle_t handle, int8_t tx_power);

/**
 * @brief 获取连接RSSI
 * 
 * @param handle 蓝牙句柄
 * @param conn_handle 连接句柄
 * @param rssi RSSI值
 * @return int 0表示成功，负值表示失败
 */
int bt_get_rssi(bt_handle_t handle, uint16_t conn_handle, int8_t *rssi);

#endif /* BLUETOOTH_API_H */
