/**
 * @file network_api.h
 * @brief 网络接口抽象层定义
 *
 * 该头文件定义了网络的统一抽象接口，使上层应用能够与底层网络硬件实现解耦
 */

#ifndef NETWORK_API_H
#define NETWORK_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 网络接口类型 */
typedef enum {
    NETWORK_TYPE_ETHERNET,        /**< 以太网接口 */
    NETWORK_TYPE_WIFI,            /**< WiFi接口 */
    NETWORK_TYPE_PPP,             /**< PPP接口 */
    NETWORK_TYPE_CELLULAR,        /**< 蜂窝网络接口 */
    NETWORK_TYPE_BLUETOOTH        /**< 蓝牙网络接口 */
} network_type_t;

/* 网络接口状态 */
typedef enum {
    NETWORK_STATUS_DOWN,          /**< 接口关闭 */
    NETWORK_STATUS_UP,            /**< 接口开启 */
    NETWORK_STATUS_CONNECTING,    /**< 正在连接 */
    NETWORK_STATUS_CONNECTED,     /**< 已连接 */
    NETWORK_STATUS_DISCONNECTING, /**< 正在断开连接 */
    NETWORK_STATUS_DISCONNECTED,  /**< 已断开连接 */
    NETWORK_STATUS_ERROR          /**< 错误状态 */
} network_status_t;

/* IP地址类型 */
typedef enum {
    NETWORK_IP_TYPE_IPV4,         /**< IPv4地址 */
    NETWORK_IP_TYPE_IPV6          /**< IPv6地址 */
} network_ip_type_t;

/* IP地址结构体 */
typedef struct {
    network_ip_type_t type;       /**< IP地址类型 */
    union {
        uint32_t ipv4;            /**< IPv4地址 */
        uint8_t ipv6[16];         /**< IPv6地址 */
    } addr;
} network_ip_addr_t;

/* MAC地址结构体 */
typedef struct {
    uint8_t addr[6];              /**< MAC地址 */
} network_mac_addr_t;

/* 网络接口配置参数 */
typedef struct {
    network_type_t type;          /**< 接口类型 */
    bool dhcp_enabled;            /**< 是否启用DHCP */
    network_ip_addr_t ip_addr;    /**< IP地址 */
    network_ip_addr_t netmask;    /**< 子网掩码 */
    network_ip_addr_t gateway;    /**< 网关地址 */
    network_ip_addr_t dns1;       /**< 主DNS服务器 */
    network_ip_addr_t dns2;       /**< 备用DNS服务器 */
    char hostname[32];            /**< 主机名 */
} network_config_t;

/* WiFi安全类型 */
typedef enum {
    NETWORK_WIFI_SECURITY_OPEN,           /**< 开放网络 */
    NETWORK_WIFI_SECURITY_WEP,            /**< WEP安全类型 */
    NETWORK_WIFI_SECURITY_WPA,            /**< WPA安全类型 */
    NETWORK_WIFI_SECURITY_WPA2,           /**< WPA2安全类型 */
    NETWORK_WIFI_SECURITY_WPA3,           /**< WPA3安全类型 */
    NETWORK_WIFI_SECURITY_WPA_WPA2_MIXED, /**< WPA/WPA2混合安全类型 */
    NETWORK_WIFI_SECURITY_WPA2_WPA3_MIXED /**< WPA2/WPA3混合安全类型 */
} network_wifi_security_t;

/* WiFi模式 */
typedef enum {
    NETWORK_WIFI_MODE_STATION,    /**< 站点模式 */
    NETWORK_WIFI_MODE_AP,         /**< 接入点模式 */
    NETWORK_WIFI_MODE_MIXED       /**< 混合模式 */
} network_wifi_mode_t;

/* WiFi配置参数 */
typedef struct {
    network_wifi_mode_t mode;                 /**< WiFi模式 */
    char ssid[33];                            /**< SSID (最大32字符) */
    char password[65];                        /**< 密码 (最大64字符) */
    network_wifi_security_t security;         /**< 安全类型 */
    uint8_t channel;                          /**< 信道 */
    bool hidden;                              /**< 是否隐藏SSID */
    int8_t max_connection;                    /**< 最大连接数 (仅AP模式) */
    uint8_t beacon_interval;                  /**< 信标间隔 (仅AP模式) */
} network_wifi_config_t;

/* 网络回调事件类型 */
typedef enum {
    NETWORK_EVENT_CONNECTED,      /**< 网络已连接 */
    NETWORK_EVENT_DISCONNECTED,   /**< 网络已断开 */
    NETWORK_EVENT_IP_ACQUIRED,    /**< 已获取IP地址 */
    NETWORK_EVENT_IP_LOST,        /**< 已失去IP地址 */
    NETWORK_EVENT_AP_STARTED,     /**< AP已启动 */
    NETWORK_EVENT_AP_STOPPED,     /**< AP已停止 */
    NETWORK_EVENT_STA_CONNECTED,  /**< 站点已连接 */
    NETWORK_EVENT_STA_DISCONNECTED/**< 站点已断开 */
} network_event_t;

/* 网络统计信息 */
typedef struct {
    uint32_t rx_packets;          /**< 接收的数据包数量 */
    uint32_t tx_packets;          /**< 发送的数据包数量 */
    uint32_t rx_bytes;            /**< 接收的字节数 */
    uint32_t tx_bytes;            /**< 发送的字节数 */
    uint32_t rx_errors;           /**< 接收错误数量 */
    uint32_t tx_errors;           /**< 发送错误数量 */
    uint32_t rx_dropped;          /**< 丢弃的接收数据包数量 */
    uint32_t tx_dropped;          /**< 丢弃的发送数据包数量 */
} network_stats_t;

/* 网络回调函数类型 */
typedef void (*network_callback_t)(void *arg, network_event_t event);

/* 网络设备句柄 */
typedef driver_handle_t network_handle_t;

/**
 * @brief 初始化网络接口
 * 
 * @param config 网络配置参数
 * @param callback 网络事件回调函数
 * @param arg 传递给回调函数的参数
 * @param handle 网络设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int network_init(const network_config_t *config, network_callback_t callback, void *arg, network_handle_t *handle);

/**
 * @brief 去初始化网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_deinit(network_handle_t handle);

/**
 * @brief 启动网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_start(network_handle_t handle);

/**
 * @brief 停止网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_stop(network_handle_t handle);

/**
 * @brief 连接网络
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_connect(network_handle_t handle);

/**
 * @brief 断开网络连接
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_disconnect(network_handle_t handle);

/**
 * @brief 获取网络接口状态
 * 
 * @param handle 网络设备句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_status(network_handle_t handle, network_status_t *status);

/**
 * @brief 获取网络接口IP地址
 * 
 * @param handle 网络设备句柄
 * @param ip_addr IP地址指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_ip_address(network_handle_t handle, network_ip_addr_t *ip_addr);

/**
 * @brief 获取网络接口MAC地址
 * 
 * @param handle 网络设备句柄
 * @param mac_addr MAC地址指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_mac_address(network_handle_t handle, network_mac_addr_t *mac_addr);

/**
 * @brief 设置网络接口MAC地址
 * 
 * @param handle 网络设备句柄
 * @param mac_addr MAC地址
 * @return int 0表示成功，非0表示失败
 */
int network_set_mac_address(network_handle_t handle, const network_mac_addr_t *mac_addr);

/**
 * @brief 获取网络接口统计信息
 * 
 * @param handle 网络设备句柄
 * @param stats 统计信息指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_stats(network_handle_t handle, network_stats_t *stats);

/**
 * @brief 配置WiFi接口
 * 
 * @param handle 网络设备句柄
 * @param wifi_config WiFi配置参数
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_config(network_handle_t handle, const network_wifi_config_t *wifi_config);

/**
 * @brief 扫描WiFi网络
 * 
 * @param handle 网络设备句柄
 * @param result_buffer 结果缓冲区
 * @param buffer_size 缓冲区大小
 * @param ap_count 扫描到的AP数量指针
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_scan(network_handle_t handle, void *result_buffer, uint32_t buffer_size, uint16_t *ap_count);

/**
 * @brief 获取WiFi信号强度
 * 
 * @param handle 网络设备句柄
 * @param rssi 信号强度指针
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_get_rssi(network_handle_t handle, int8_t *rssi);

/**
 * @brief 进入低功耗模式
 * 
 * @param handle 网络设备句柄
 * @param enable 是否启用
 * @return int 0表示成功，非0表示失败
 */
int network_enter_low_power(network_handle_t handle, bool enable);

/**
 * @brief 发送数据包
 * 
 * @param handle 网络设备句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return int 0表示成功，非0表示失败
 */
int network_send_packet(network_handle_t handle, const void *data, uint32_t length);

/**
 * @brief 接收数据包
 * 
 * @param handle 网络设备句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param received_length 实际接收长度指针
 * @return int 0表示成功，非0表示失败
 */
int network_receive_packet(network_handle_t handle, void *data, uint32_t length, uint32_t *received_length);

#endif /* NETWORK_API_H */
