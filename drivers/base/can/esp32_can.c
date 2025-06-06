/**
 * @file esp32_can.c
 * @brief ESP32 CAN驱动实现
 */

#include "base/can_api.h"
#include "esp32_platform.h"
#include <string.h>
#include "driver/twai.h"
#include "esp_err.h"

/* ESP32使用TWAI接口（原CAN接口）进行CAN通信 */

/* 私有定义 */
#define ESP32_CAN_MAX_INSTANCES   1   /* ESP32只有一个CAN控制�?*/
#define ESP32_CAN_MAX_FILTERS     16  /* ESP32 CAN过滤器最大数�?*/

/* CAN设备数据结构 */
typedef struct {
    twai_timing_config_t timing_config;  /* 时序配置 */
    twai_filter_config_t filter_config;  /* 过滤器配�?*/
    twai_general_config_t general_config; /* 常规配置 */
    can_callback_t callback;            /* 用户回调函数 */
    void *callback_arg;                 /* 回调函数参数 */
    bool initialized;                   /* 初始化标�?*/
    can_config_t config;                /* CAN配置参数 */
    can_stats_t stats;                  /* 统计信息 */
    can_status_t status;                /* 设备状�?*/
    can_message_t rx_msg;               /* 接收消息缓存 */
    bool rx_pending;                    /* 接收挂起标志 */
    uint32_t timestamp;                 /* 时间�?*/
    TaskHandle_t rx_task_handle;        /* 接收任务句柄 */
    bool rx_task_running;               /* 接收任务运行标志 */
} esp32_can_t;

/* CAN设备实例 */
static esp32_can_t can_instances[ESP32_CAN_MAX_INSTANCES];

/**
 * @brief 将ESP32 TWAI消息转换为CAN消息
 */
static void twai_to_can_message(const twai_message_t *twai_msg, can_message_t *can_msg) {
    if (twai_msg == NULL || can_msg == NULL) {
        return;
    }
    
    can_msg->id = twai_msg->flags & TWAI_MSG_FLAG_EXTD ? twai_msg->identifier : twai_msg->identifier;
    can_msg->id_type = twai_msg->flags & TWAI_MSG_FLAG_EXTD ? CAN_ID_EXTENDED : CAN_ID_STANDARD;
    can_msg->frame_type = twai_msg->flags & TWAI_MSG_FLAG_RTR ? CAN_FRAME_REMOTE : CAN_FRAME_DATA;
    can_msg->dlc = twai_msg->data_length_code;
    memcpy(can_msg->data, twai_msg->data, can_msg->dlc);
}

/**
 * @brief 将CAN消息转换为ESP32 TWAI消息
 */
static void can_to_twai_message(const can_message_t *can_msg, twai_message_t *twai_msg) {
    if (can_msg == NULL || twai_msg == NULL) {
        return;
    }
    
    twai_msg->identifier = can_msg->id;
    twai_msg->data_length_code = can_msg->dlc;
    memcpy(twai_msg->data, can_msg->data, can_msg->dlc);
    
    /* 设置消息标志 */
    twai_msg->flags = 0;
    if (can_msg->id_type == CAN_ID_EXTENDED) {
        twai_msg->flags |= TWAI_MSG_FLAG_EXTD;
    }
    if (can_msg->frame_type == CAN_FRAME_REMOTE) {
        twai_msg->flags |= TWAI_MSG_FLAG_RTR;
    }
}

/**
 * @brief CAN接收任务
 */
static void can_rx_task(void *arg) {
    esp32_can_t *can_dev = (esp32_can_t *)arg;
    twai_message_t twai_msg;
    
    while (can_dev->rx_task_running) {
        /* 等待接收消息 */
        esp_err_t ret = twai_receive(&twai_msg, pdMS_TO_TICKS(100));
        
        if (ret == ESP_OK) {
            /* 转换消息格式 */
            can_message_t can_msg;
            twai_to_can_message(&twai_msg, &can_msg);
            can_msg.timestamp = can_dev->timestamp++;
            
            /* 更新统计信息 */
            can_dev->stats.rx_count++;
            
            /* 存储消息 */
            memcpy(&can_dev->rx_msg, &can_msg, sizeof(can_message_t));
            can_dev->rx_pending = true;
            
            /* 调用用户回调函数 */
            if (can_dev->callback != NULL) {
                can_dev->callback(can_dev->callback_arg, CAN_STATUS_COMPLETE, &can_msg);
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            /* 超时，继续等�?*/
        } else {
            /* 其他错误 */
            can_dev->stats.error_count++;
            
            /* 检查错误类�?*/
            twai_status_info_t status_info;
            if (twai_get_status_info(&status_info) == ESP_OK) {
                if (status_info.state == TWAI_STATE_BUS_OFF) {
                    can_dev->stats.bus_off = true;
                    
                    /* 调用用户回调函数 */
                    if (can_dev->callback != NULL) {
                        can_dev->callback(can_dev->callback_arg, CAN_STATUS_ERROR, NULL);
                    }
                }
                
                if (status_info.state == TWAI_STATE_RECOVERING) {
                    can_dev->stats.passive_error = true;
                }
                
                can_dev->stats.tx_error_counter = status_info.tx_error_counter;
                can_dev->stats.rx_error_counter = status_info.rx_error_counter;
            }
        }
        
        /* 让出CPU时间 */
        vTaskDelay(1);
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief 初始化CAN设备
 */
int can_init(const can_config_t *config, can_callback_t callback, void *arg, can_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return -1;
    }
    
    /* 查找空闲实例 */
    esp32_can_t *can_dev = NULL;
    
    for (int i = 0; i < ESP32_CAN_MAX_INSTANCES; i++) {
        if (!can_instances[i].initialized) {
            can_dev = &can_instances[i];
            break;
        }
    }
    
    if (can_dev == NULL) {
        return -1; /* 没有空闲实例 */
    }
    
    /* 初始化设备数�?*/
    memset(can_dev, 0, sizeof(esp32_can_t));
    can_dev->callback = callback;
    can_dev->callback_arg = arg;
    memcpy(&can_dev->config, config, sizeof(can_config_t));
    
    /* 配置时序参数 */
    switch (config->baudrate) {
        case CAN_BAUDRATE_10K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_10KBITS();
            break;
        case CAN_BAUDRATE_20K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_20KBITS();
            break;
        case CAN_BAUDRATE_50K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_50KBITS();
            break;
        case CAN_BAUDRATE_100K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_100KBITS();
            break;
        case CAN_BAUDRATE_125K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_125KBITS();
            break;
        case CAN_BAUDRATE_250K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_250KBITS();
            break;
        case CAN_BAUDRATE_500K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_500KBITS();
            break;
        case CAN_BAUDRATE_800K:
            can_dev->timing_config = TWAI_TIMING_CONFIG_800KBITS();
            break;
        case CAN_BAUDRATE_1M:
            can_dev->timing_config = TWAI_TIMING_CONFIG_1MBITS();
            break;
        default:
            can_dev->timing_config = TWAI_TIMING_CONFIG_125KBITS();
            break;
    }
    
    /* 配置过滤�?*/
    can_dev->filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    /* 配置通用参数 */
    can_dev->general_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CONFIG_ESP32_CAN_TX_PIN, (gpio_num_t)CONFIG_ESP32_CAN_RX_PIN, config->mode);
    
    /* 根据模式设置工作模式 */
    switch (config->mode) {
        case CAN_MODE_NORMAL:
            can_dev->general_config.mode = TWAI_MODE_NORMAL;
            break;
        case CAN_MODE_LOOPBACK:
            can_dev->general_config.mode = TWAI_MODE_NO_ACK;
            break;
        case CAN_MODE_SILENT:
            can_dev->general_config.mode = TWAI_MODE_LISTEN_ONLY;
            break;
        case CAN_MODE_SILENT_LOOPBACK:
            /* ESP32 TWAI没有直接的静默回环模�?*/
            can_dev->general_config.mode = TWAI_MODE_NO_ACK;
            break;
        default:
            can_dev->general_config.mode = TWAI_MODE_NORMAL;
            break;
    }
    
    /* 初始化CAN外设 */
    if (twai_driver_install(&can_dev->general_config, &can_dev->timing_config, &can_dev->filter_config) != ESP_OK) {
        return -1;
    }
    
    /* 创建接收任务 */
    can_dev->rx_task_running = true;
    if (xTaskCreate(can_rx_task, "can_rx", 2048, can_dev, 5, &can_dev->rx_task_handle) != pdPASS) {
        twai_driver_uninstall();
        return -1;
    }
    
    /* 标记为已初始�?*/
    can_dev->initialized = true;
    can_dev->status = CAN_STATUS_IDLE;
    
    /* 返回句柄 */
    *handle = (can_handle_t)can_dev;
    
    return 0;
}

/**
 * @brief 去初始化CAN设备
 */
int can_deinit(can_handle_t handle) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 停止接收任务 */
    if (can_dev->rx_task_handle != NULL) {
        can_dev->rx_task_running = false;
        vTaskDelay(pdMS_TO_TICKS(200)); /* 等待任务结束 */
    }
    
    /* 停止CAN外设 */
    twai_stop();
    twai_driver_uninstall();
    
    /* 清除设备数据 */
    can_dev->initialized = false;
    
    return 0;
}

/**
 * @brief 配置CAN过滤�?
 */
int can_config_filter(can_handle_t handle, const can_filter_t *filter) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || filter == NULL) {
        return -1;
    }
    
    /* ESP32 TWAI不支持在运行时更改过滤器
     * 必须停止驱动程序，重新配置，然后重新启动
     */
    
    /* 停止CAN控制�?*/
    twai_stop();
    twai_driver_uninstall();
    
    /* 配置过滤�?*/
    if (filter->active) {
        if (filter->type == CAN_FILTER_MASK) {
            /* 掩码过滤模式 */
            if (filter->id_type == CAN_ID_STANDARD) {
                /* 标准ID */
                can_dev->filter_config.acceptance_code = filter->id << 21;
                can_dev->filter_config.acceptance_mask = ~(filter->mask << 21);
                can_dev->filter_config.single_filter = true;
            } else {
                /* 扩展ID */
                can_dev->filter_config.acceptance_code = filter->id;
                can_dev->filter_config.acceptance_mask = ~filter->mask;
                can_dev->filter_config.single_filter = true;
            }
        } else {
            /* ESP32 TWAI不直接支持列表模式过滤器
             * 可以使用多个掩码过滤器模拟，但当前实现不支持
             */
            return -1;
        }
    } else {
        /* 禁用过滤器，接受所有帧 */
        can_dev->filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    }
    
    /* 重新安装驱动程序 */
    if (twai_driver_install(&can_dev->general_config, &can_dev->timing_config, &can_dev->filter_config) != ESP_OK) {
        return -1;
    }
    
    /* 重新启动CAN控制�?*/
    if (twai_start() != ESP_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 启动CAN设备
 */
int can_start(can_handle_t handle) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 启动CAN外设 */
    if (twai_start() != ESP_OK) {
        return -1;
    }
    
    can_dev->status = CAN_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 停止CAN设备
 */
int can_stop(can_handle_t handle) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 停止CAN外设 */
    if (twai_stop() != ESP_OK) {
        return -1;
    }
    
    can_dev->status = CAN_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 发送CAN消息
 */
int can_transmit(can_handle_t handle, const can_message_t *msg, uint32_t timeout_ms) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || msg == NULL) {
        return -1;
    }
    
    /* 准备发送数�?*/
    twai_message_t twai_msg;
    can_to_twai_message(msg, &twai_msg);
    
    /* 发送消�?*/
    if (twai_transmit(&twai_msg, pdMS_TO_TICKS(timeout_ms)) != ESP_OK) {
        return -1;
    }
    
    /* 更新统计信息 */
    can_dev->stats.tx_count++;
    
    return 0;
}

/**
 * @brief 接收CAN消息
 */
int can_receive(can_handle_t handle, can_message_t *msg, uint32_t timeout_ms) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || msg == NULL) {
        return -1;
    }
    
    /* 如果有挂起的消息，直接返�?*/
    if (can_dev->rx_pending) {
        memcpy(msg, &can_dev->rx_msg, sizeof(can_message_t));
        can_dev->rx_pending = false;
        return 0;
    }
    
    /* 如果没有挂起的消息，直接接收 */
    twai_message_t twai_msg;
    if (twai_receive(&twai_msg, pdMS_TO_TICKS(timeout_ms)) == ESP_OK) {
        /* 转换消息格式 */
        twai_to_can_message(&twai_msg, msg);
        msg->timestamp = can_dev->timestamp++;
        
        /* 更新统计信息 */
        can_dev->stats.rx_count++;
        
        return 0;
    }
    
    return -1; /* 接收超时或失�?*/
}

/**
 * @brief 获取CAN统计信息
 */
int can_get_stats(can_handle_t handle, can_stats_t *stats) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || stats == NULL) {
        return -1;
    }
    
    /* 获取最新错误计数器�?*/
    twai_status_info_t status_info;
    if (twai_get_status_info(&status_info) == ESP_OK) {
        can_dev->stats.tx_error_counter = status_info.tx_error_counter;
        can_dev->stats.rx_error_counter = status_info.rx_error_counter;
        can_dev->stats.bus_off = status_info.state == TWAI_STATE_BUS_OFF;
        can_dev->stats.passive_error = status_info.state == TWAI_STATE_RECOVERING;
        can_dev->stats.overrun_count = status_info.rx_missed_count;
    }
    
    /* 复制统计信息 */
    memcpy(stats, &can_dev->stats, sizeof(can_stats_t));
    
    return 0;
}

/**
 * @brief 清除CAN统计信息
 */
int can_clear_stats(can_handle_t handle) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 清除统计数据 */
    memset(&can_dev->stats, 0, sizeof(can_stats_t));
    
    return 0;
}

/**
 * @brief 获取CAN错误状�?
 */
int can_get_error_status(can_handle_t handle, bool *bus_off, bool *passive_error) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || bus_off == NULL || passive_error == NULL) {
        return -1;
    }
    
    /* 获取错误状�?*/
    twai_status_info_t status_info;
    if (twai_get_status_info(&status_info) != ESP_OK) {
        return -1;
    }
    
    *bus_off = status_info.state == TWAI_STATE_BUS_OFF;
    *passive_error = status_info.state == TWAI_STATE_RECOVERING;
    
    return 0;
}

/**
 * @brief 重置CAN错误计数�?
 */
int can_reset_error_counters(can_handle_t handle) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TWAI没有直接提供重置错误计数器的函数
     * 通过重新启动控制器来实现
     */
    twai_stop();
    twai_start();
    
    return 0;
}

/**
 * @brief 设置CAN工作模式
 */
int can_set_mode(can_handle_t handle, can_mode_t mode) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TWAI不支持在运行时更改工作模�?
     * 必须停止驱动程序，重新配置，然后重新启动
     */
    
    /* 停止CAN控制�?*/
    twai_stop();
    twai_driver_uninstall();
    
    /* 更新模式配置 */
    can_dev->config.mode = mode;
    
    /* 更新通用配置 */
    switch (mode) {
        case CAN_MODE_NORMAL:
            can_dev->general_config.mode = TWAI_MODE_NORMAL;
            break;
        case CAN_MODE_LOOPBACK:
            can_dev->general_config.mode = TWAI_MODE_NO_ACK;
            break;
        case CAN_MODE_SILENT:
            can_dev->general_config.mode = TWAI_MODE_LISTEN_ONLY;
            break;
        case CAN_MODE_SILENT_LOOPBACK:
            /* ESP32 TWAI没有直接的静默回环模�?*/
            can_dev->general_config.mode = TWAI_MODE_NO_ACK;
            break;
        default:
            can_dev->general_config.mode = TWAI_MODE_NORMAL;
            break;
    }
    
    /* 重新安装驱动程序 */
    if (twai_driver_install(&can_dev->general_config, &can_dev->timing_config, &can_dev->filter_config) != ESP_OK) {
        return -1;
    }
    
    /* 重新启动CAN控制�?*/
    if (twai_start() != ESP_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 设置CAN接收回调函数
 */
int can_set_rx_callback(can_handle_t handle, can_callback_t callback, void *arg) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 更新回调函数 */
    can_dev->callback = callback;
    can_dev->callback_arg = arg;
    
    return 0;
}

/**
 * @brief 获取CAN设备状�?
 */
int can_get_status(can_handle_t handle, can_status_t *status) {
    esp32_can_t *can_dev = (esp32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || status == NULL) {
        return -1;
    }
    
    *status = can_dev->status;
    
    return 0;
}
