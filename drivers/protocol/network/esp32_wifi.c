/**
 * @file esp32_wifi.c
 * @brief ESP32平台WiFi网络驱动实现
 *
 * 该文件实现了ESP32平台的WiFi网络驱动接口
 */

#include "protocol/network_api.h"
#include "common/error_api.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>

/* ESP32 WiFi设备结构�?*/
typedef struct {
    network_config_t config;               /* 网络配置 */
    network_wifi_config_t wifi_config;     /* WiFi配置 */
    network_callback_t callback;           /* 回调函数 */
    void *arg;                             /* 回调参数 */
    esp_netif_t *netif_sta;                /* STA网络接口 */
    esp_netif_t *netif_ap;                 /* AP网络接口 */
    volatile network_status_t status;      /* 网络状�?*/
    network_stats_t stats;                 /* 网络统计信息 */
    bool initialized;                      /* 初始化标�?*/
    bool wifi_started;                     /* WiFi启动标志 */
} esp32_wifi_device_t;

/* WiFi设备 */
static esp32_wifi_device_t g_wifi_device;

/* 事件处理函数 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp32_wifi_device_t *dev = &g_wifi_device;
    
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                /* 站点模式启动 */
                if (dev->config.dhcp_enabled) {
                    esp_netif_dhcpc_start(dev->netif_sta);
                }
                /* 尝试连接WiFi */
                esp_wifi_connect();
                dev->status = NETWORK_STATUS_CONNECTING;
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                /* 站点模式已连�?*/
                dev->status = NETWORK_STATUS_CONNECTED;
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_CONNECTED);
                }
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                /* 站点模式已断开 */
                dev->status = NETWORK_STATUS_DISCONNECTED;
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_DISCONNECTED);
                }
                /* 重新连接 */
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_AP_START:
                /* 接入点模式启�?*/
                dev->status = NETWORK_STATUS_UP;
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_AP_STARTED);
                }
                break;
                
            case WIFI_EVENT_AP_STOP:
                /* 接入点模式停�?*/
                dev->status = NETWORK_STATUS_DOWN;
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_AP_STOPPED);
                }
                break;
                
            case WIFI_EVENT_AP_STACONNECTED:
                /* 站点连接到接入点 */
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_STA_CONNECTED);
                }
                break;
                
            case WIFI_EVENT_AP_STADISCONNECTED:
                /* 站点断开接入�?*/
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_STA_DISCONNECTED);
                }
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                /* 获取IP地址 */
                dev->status = NETWORK_STATUS_CONNECTED;
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_IP_ACQUIRED);
                }
                break;
                
            case IP_EVENT_STA_LOST_IP:
                /* 失去IP地址 */
                if (dev->callback) {
                    dev->callback(dev->arg, NETWORK_EVENT_IP_LOST);
                }
                break;
                
            default:
                break;
        }
    }
}

/* 转换WiFi模式 */
static wifi_mode_t convert_wifi_mode(network_wifi_mode_t mode)
{
    switch (mode) {
        case NETWORK_WIFI_MODE_STATION:
            return WIFI_MODE_STA;
        case NETWORK_WIFI_MODE_AP:
            return WIFI_MODE_AP;
        case NETWORK_WIFI_MODE_MIXED:
            return WIFI_MODE_APSTA;
        default:
            return WIFI_MODE_STA;
    }
}

/* 转换WiFi安全类型 */
static wifi_auth_mode_t convert_wifi_security(network_wifi_security_t security)
{
    switch (security) {
        case NETWORK_WIFI_SECURITY_OPEN:
            return WIFI_AUTH_OPEN;
        case NETWORK_WIFI_SECURITY_WEP:
            return WIFI_AUTH_WEP;
        case NETWORK_WIFI_SECURITY_WPA:
            return WIFI_AUTH_WPA_PSK;
        case NETWORK_WIFI_SECURITY_WPA2:
            return WIFI_AUTH_WPA2_PSK;
        case NETWORK_WIFI_SECURITY_WPA3:
            return WIFI_AUTH_WPA3_PSK;
        case NETWORK_WIFI_SECURITY_WPA_WPA2_MIXED:
            return WIFI_AUTH_WPA_WPA2_PSK;
        case NETWORK_WIFI_SECURITY_WPA2_WPA3_MIXED:
            return WIFI_AUTH_WPA2_WPA3_PSK;
        default:
            return WIFI_AUTH_OPEN;
    }
}

/**
 * @brief 初始化网络接�?
 * 
 * @param config 网络配置参数
 * @param callback 网络事件回调函数
 * @param arg 传递给回调函数的参�?
 * @param handle 网络设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int network_init(const network_config_t *config, network_callback_t callback, void *arg, network_handle_t *handle)
{
    esp32_wifi_device_t *dev;
    esp_err_t err;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查接口类�?*/
    if (config->type != NETWORK_TYPE_WIFI) {
        return ERROR_NOT_SUPPORTED;
    }
    
    /* 获取WiFi设备 */
    dev = &g_wifi_device;
    
    /* 检查是否已初始�?*/
    if (dev->initialized) {
        return ERROR_ALREADY_INITIALIZED;
    }
    
    /* 初始化WiFi设备 */
    memset(dev, 0, sizeof(esp32_wifi_device_t));
    dev->callback = callback;
    dev->arg = arg;
    dev->status = NETWORK_STATUS_DOWN;
    
    /* 复制配置 */
    memcpy(&dev->config, config, sizeof(network_config_t));
    
    /* 初始化默认WiFi配置 */
    dev->wifi_config.mode = NETWORK_WIFI_MODE_STATION;
    
    /* 初始化ESP-NETIF */
    err = esp_netif_init();
    if (err != ESP_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 创建默认事件循环 */
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        esp_netif_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 创建STA和AP网络接口 */
    dev->netif_sta = esp_netif_create_default_wifi_sta();
    dev->netif_ap = esp_netif_create_default_wifi_ap();
    
    if (dev->netif_sta == NULL || dev->netif_ap == NULL) {
        esp_event_loop_delete_default();
        esp_netif_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 设置主机�?*/
    if (config->hostname[0] != '\0') {
        esp_netif_set_hostname(dev->netif_sta, config->hostname);
        esp_netif_set_hostname(dev->netif_ap, config->hostname);
    }
    
    /* 配置静态IP (如果DHCP禁用) */
    if (!config->dhcp_enabled) {
        esp_netif_dhcpc_stop(dev->netif_sta);
        
        esp_netif_ip_info_t ip_info;
        memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
        
        /* 设置IP地址 */
        if (config->ip_addr.type == NETWORK_IP_TYPE_IPV4) {
            ip_info.ip.addr = config->ip_addr.addr.ipv4;
            ip_info.netmask.addr = config->netmask.addr.ipv4;
            ip_info.gw.addr = config->gateway.addr.ipv4;
            
            esp_netif_set_ip_info(dev->netif_sta, &ip_info);
            
            /* 设置DNS服务�?*/
            esp_netif_dns_info_t dns_info;
            dns_info.ip.u_addr.ip4.addr = config->dns1.addr.ipv4;
            esp_netif_set_dns_info(dev->netif_sta, ESP_NETIF_DNS_MAIN, &dns_info);
            
            dns_info.ip.u_addr.ip4.addr = config->dns2.addr.ipv4;
            esp_netif_set_dns_info(dev->netif_sta, ESP_NETIF_DNS_BACKUP, &dns_info);
        }
    }
    
    /* 注册WiFi事件处理函数 */
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (err != ESP_OK) {
        esp_netif_destroy(dev->netif_sta);
        esp_netif_destroy(dev->netif_ap);
        esp_event_loop_delete_default();
        esp_netif_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 注册IP事件处理函数 */
    err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (err != ESP_OK) {
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_netif_destroy(dev->netif_sta);
        esp_netif_destroy(dev->netif_ap);
        esp_event_loop_delete_default();
        esp_netif_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 初始化WiFi */
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_init_config);
    if (err != ESP_OK) {
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_netif_destroy(dev->netif_sta);
        esp_netif_destroy(dev->netif_ap);
        esp_event_loop_delete_default();
        esp_netif_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 设置初始化标�?*/
    dev->initialized = true;
    
    /* 返回句柄 */
    *handle = (network_handle_t)dev;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_deinit(network_handle_t handle)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 停止WiFi */
    if (dev->wifi_started) {
        esp_wifi_stop();
        dev->wifi_started = false;
    }
    
    /* 去初始化WiFi */
    esp_wifi_deinit();
    
    /* 取消注册事件处理函数 */
    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    
    /* 销毁网络接�?*/
    esp_netif_destroy(dev->netif_sta);
    esp_netif_destroy(dev->netif_ap);
    
    /* 删除默认事件循环 */
    esp_event_loop_delete_default();
    
    /* 去初始化ESP-NETIF */
    esp_netif_deinit();
    
    /* 清除初始化标�?*/
    dev->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 启动网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_start(network_handle_t handle)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (dev->wifi_started) {
        return DRIVER_OK;
    }
    
    /* 设置WiFi模式 */
    err = esp_wifi_set_mode(convert_wifi_mode(dev->wifi_config.mode));
    if (err != ESP_OK) {
        return ERROR_DRIVER_START_FAILED;
    }
    
    /* 配置WiFi */
    if (dev->wifi_config.mode == NETWORK_WIFI_MODE_STATION || 
        dev->wifi_config.mode == NETWORK_WIFI_MODE_MIXED) {
        /* 配置站点模式 */
        wifi_config_t wifi_sta_config = {0};
        
        /* 设置SSID和密�?*/
        strncpy((char *)wifi_sta_config.sta.ssid, dev->wifi_config.ssid, sizeof(wifi_sta_config.sta.ssid) - 1);
        strncpy((char *)wifi_sta_config.sta.password, dev->wifi_config.password, sizeof(wifi_sta_config.sta.password) - 1);
        
        /* 配置站点 */
        err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config);
        if (err != ESP_OK) {
            return ERROR_DRIVER_START_FAILED;
        }
    }
    
    if (dev->wifi_config.mode == NETWORK_WIFI_MODE_AP || 
        dev->wifi_config.mode == NETWORK_WIFI_MODE_MIXED) {
        /* 配置接入点模�?*/
        wifi_config_t wifi_ap_config = {0};
        
        /* 设置SSID和密�?*/
        strncpy((char *)wifi_ap_config.ap.ssid, dev->wifi_config.ssid, sizeof(wifi_ap_config.ap.ssid) - 1);
        strncpy((char *)wifi_ap_config.ap.password, dev->wifi_config.password, sizeof(wifi_ap_config.ap.password) - 1);
        
        /* 设置安全类型 */
        wifi_ap_config.ap.authmode = convert_wifi_security(dev->wifi_config.security);
        
        /* 设置最大连接数 */
        wifi_ap_config.ap.max_connection = (dev->wifi_config.max_connection > 0) ? 
                                           dev->wifi_config.max_connection : 4;
        
        /* 设置是否隐藏SSID */
        wifi_ap_config.ap.ssid_hidden = dev->wifi_config.hidden ? 1 : 0;
        
        /* 设置信道 */
        wifi_ap_config.ap.channel = (dev->wifi_config.channel > 0) ? 
                                     dev->wifi_config.channel : 1;
        
        /* 设置信标间隔 */
        wifi_ap_config.ap.beacon_interval = (dev->wifi_config.beacon_interval > 0) ? 
                                           dev->wifi_config.beacon_interval : 100;
        
        /* 配置接入�?*/
        err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config);
        if (err != ESP_OK) {
            return ERROR_DRIVER_START_FAILED;
        }
    }
    
    /* 启动WiFi */
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return ERROR_DRIVER_START_FAILED;
    }
    
    /* 设置WiFi启动标志 */
    dev->wifi_started = true;
    
    /* 更新状�?*/
    dev->status = NETWORK_STATUS_UP;
    
    return DRIVER_OK;
}

/**
 * @brief 停止网络接口
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_stop(network_handle_t handle)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return DRIVER_OK;
    }
    
    /* 停止WiFi */
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        return ERROR_DRIVER_STOP_FAILED;
    }
    
    /* 清除WiFi启动标志 */
    dev->wifi_started = false;
    
    /* 更新状�?*/
    dev->status = NETWORK_STATUS_DOWN;
    
    return DRIVER_OK;
}

/**
 * @brief 连接网络
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_connect(network_handle_t handle)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 检查模式是否为站点模式 */
    if (dev->wifi_config.mode != NETWORK_WIFI_MODE_STATION && 
        dev->wifi_config.mode != NETWORK_WIFI_MODE_MIXED) {
        return ERROR_INVALID_STATE;
    }
    
    /* 连接WiFi */
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = NETWORK_STATUS_CONNECTING;
    
    return DRIVER_OK;
}

/**
 * @brief 断开网络连接
 * 
 * @param handle 网络设备句柄
 * @return int 0表示成功，非0表示失败
 */
int network_disconnect(network_handle_t handle)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 检查模式是否为站点模式 */
    if (dev->wifi_config.mode != NETWORK_WIFI_MODE_STATION && 
        dev->wifi_config.mode != NETWORK_WIFI_MODE_MIXED) {
        return ERROR_INVALID_STATE;
    }
    
    /* 断开WiFi连接 */
    err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = NETWORK_STATUS_DISCONNECTING;
    
    return DRIVER_OK;
}

/**
 * @brief 获取网络接口状�?
 * 
 * @param handle 网络设备句柄
 * @param status 状态指�?
 * @return int 0表示成功，非0表示失败
 */
int network_get_status(network_handle_t handle, network_status_t *status)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || status == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取状�?*/
    *status = dev->status;
    
    return DRIVER_OK;
}

/**
 * @brief 获取网络接口IP地址
 * 
 * @param handle 网络设备句柄
 * @param ip_addr IP地址指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_ip_address(network_handle_t handle, network_ip_addr_t *ip_addr)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_netif_t *netif;
    esp_netif_ip_info_t ip_info;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || ip_addr == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 选择网络接口 */
    if (dev->wifi_config.mode == NETWORK_WIFI_MODE_STATION || 
        dev->wifi_config.mode == NETWORK_WIFI_MODE_MIXED) {
        netif = dev->netif_sta;
    } else {
        netif = dev->netif_ap;
    }
    
    /* 获取IP信息 */
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 设置IP地址 */
    ip_addr->type = NETWORK_IP_TYPE_IPV4;
    ip_addr->addr.ipv4 = ip_info.ip.addr;
    
    return DRIVER_OK;
}

/**
 * @brief 获取网络接口MAC地址
 * 
 * @param handle 网络设备句柄
 * @param mac_addr MAC地址指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_mac_address(network_handle_t handle, network_mac_addr_t *mac_addr)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    uint8_t mac[6];
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || mac_addr == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 获取MAC地址 */
    if (dev->wifi_config.mode == NETWORK_WIFI_MODE_STATION || 
        dev->wifi_config.mode == NETWORK_WIFI_MODE_MIXED) {
        err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    } else {
        err = esp_wifi_get_mac(ESP_IF_WIFI_AP, mac);
    }
    
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 复制MAC地址 */
    memcpy(mac_addr->addr, mac, 6);
    
    return DRIVER_OK;
}

/**
 * @brief 设置网络接口MAC地址
 * 
 * @param handle 网络设备句柄
 * @param mac_addr MAC地址
 * @return int 0表示成功，非0表示失败
 */
int network_set_mac_address(network_handle_t handle, const network_mac_addr_t *mac_addr)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || mac_addr == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (dev->wifi_started) {
        return ERROR_INVALID_STATE;
    }
    
    /* 设置MAC地址 */
    if (dev->wifi_config.mode == NETWORK_WIFI_MODE_STATION || 
        dev->wifi_config.mode == NETWORK_WIFI_MODE_MIXED) {
        err = esp_wifi_set_mac(ESP_IF_WIFI_STA, mac_addr->addr);
    } else {
        err = esp_wifi_set_mac(ESP_IF_WIFI_AP, mac_addr->addr);
    }
    
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取网络接口统计信息
 * 
 * @param handle 网络设备句柄
 * @param stats 统计信息指针
 * @return int 0表示成功，非0表示失败
 */
int network_get_stats(network_handle_t handle, network_stats_t *stats)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || stats == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 复制统计信息 */
    memcpy(stats, &dev->stats, sizeof(network_stats_t));
    
    return DRIVER_OK;
}

/**
 * @brief 配置WiFi接口
 * 
 * @param handle 网络设备句柄
 * @param wifi_config WiFi配置参数
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_config(network_handle_t handle, const network_wifi_config_t *wifi_config)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || wifi_config == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (dev->wifi_started) {
        return ERROR_INVALID_STATE;
    }
    
    /* 复制WiFi配置 */
    memcpy(&dev->wifi_config, wifi_config, sizeof(network_wifi_config_t));
    
    return DRIVER_OK;
}

/**
 * @brief 扫描WiFi网络
 * 
 * @param handle 网络设备句柄
 * @param result_buffer 结果缓冲�?
 * @param buffer_size 缓冲区大�?
 * @param ap_count 扫描到的AP数量指针
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_scan(network_handle_t handle, void *result_buffer, uint32_t buffer_size, uint16_t *ap_count)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    wifi_scan_config_t scan_config = {0};
    uint16_t max_ap_count = buffer_size / sizeof(wifi_ap_record_t);
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || result_buffer == NULL || 
        buffer_size == 0 || ap_count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 检查模式是否为站点模式 */
    if (dev->wifi_config.mode != NETWORK_WIFI_MODE_STATION && 
        dev->wifi_config.mode != NETWORK_WIFI_MODE_MIXED) {
        return ERROR_INVALID_STATE;
    }
    
    /* 开始扫�?*/
    err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 获取扫描结果 */
    *ap_count = max_ap_count;
    err = esp_wifi_scan_get_ap_records(ap_count, (wifi_ap_record_t *)result_buffer);
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取WiFi信号强度
 * 
 * @param handle 网络设备句柄
 * @param rssi 信号强度指针
 * @return int 0表示成功，非0表示失败
 */
int network_wifi_get_rssi(network_handle_t handle, int8_t *rssi)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    wifi_ap_record_t ap_info;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || rssi == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 检查模式是否为站点模式 */
    if (dev->wifi_config.mode != NETWORK_WIFI_MODE_STATION && 
        dev->wifi_config.mode != NETWORK_WIFI_MODE_MIXED) {
        return ERROR_INVALID_STATE;
    }
    
    /* 检查是否已连接 */
    if (dev->status != NETWORK_STATUS_CONNECTED) {
        return ERROR_INVALID_STATE;
    }
    
    /* 获取AP信息 */
    err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 获取RSSI */
    *rssi = ap_info.rssi;
    
    return DRIVER_OK;
}

/**
 * @brief 进入低功耗模�?
 * 
 * @param handle 网络设备句柄
 * @param enable 是否启用
 * @return int 0表示成功，非0表示失败
 */
int network_enter_low_power(network_handle_t handle, bool enable)
{
    esp32_wifi_device_t *dev = (esp32_wifi_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已启动 */
    if (!dev->wifi_started) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 设置省电模式 */
    if (enable) {
        err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    } else {
        err = esp_wifi_set_ps(WIFI_PS_NONE);
    }
    
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 发送数据包
 * 
 * @param handle 网络设备句柄
 * @param data 数据缓冲�?
 * @param length 数据长度
 * @return int 0表示成功，非0表示失败
 */
int network_send_packet(network_handle_t handle, const void *data, uint32_t length)
{
    /* ESP32 WiFi API不支持直接发送数据包，需要使用套接字API */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 接收数据�?
 * 
 * @param handle 网络设备句柄
 * @param data 数据缓冲�?
 * @param length 数据长度
 * @param received_length 实际接收长度指针
 * @return int 0表示成功，非0表示失败
 */
int network_receive_packet(network_handle_t handle, void *data, uint32_t length, uint32_t *received_length)
{
    /* ESP32 WiFi API不支持直接接收数据包，需要使用套接字API */
    return ERROR_NOT_SUPPORTED;
}

