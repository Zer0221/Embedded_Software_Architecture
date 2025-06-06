/**
 * @file esp32_usb.c
 * @brief ESP32 USB驱动实现
 */

#include "base/usb_api.h"
#include "esp32_platform.h"
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"

/* ESP32使用TinyUSB库实现USB功能 */
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_hid.h"
#include "tusb_msc.h"

/* 私有定义 */
#define ESP32_USB_MAX_ENDPOINTS    8  /* 最大端点数 */
#define ESP32_USB_MAX_INSTANCES    1  /* ESP32只有一个USB控制�?*/

/* 标签用于日志输出 */
static const char *TAG = "ESP32_USB";

/* USB端点数据结构 */
typedef struct {
    uint8_t ep_addr;                /* 端点地址 */
    usb_endpoint_type_t type;       /* 端点类型 */
    uint16_t max_packet_size;       /* 最大包大小 */
    usb_callback_t callback;        /* 端点回调函数 */
    void *callback_arg;             /* 回调函数参数 */
    bool active;                    /* 激活状�?*/
    uint8_t *buffer;                /* 数据缓冲�?*/
    uint32_t buffer_size;           /* 缓冲区大�?*/
} esp32_usb_endpoint_t;

/* USB设备数据结构 */
typedef struct {
    esp32_usb_endpoint_t endpoints[ESP32_USB_MAX_ENDPOINTS]; /* 端点数据 */
    usb_device_state_callback_t state_callback; /* 设备状态回调函�?*/
    void *state_callback_arg;            /* 回调函数参数 */
    bool initialized;                    /* 初始化标�?*/
    usb_config_t config;                 /* USB配置参数 */
    usb_status_t status;                 /* 设备状�?*/
    usb_device_state_t device_state;     /* USB设备状�?*/
    usb_speed_t speed;                   /* USB速度 */
    TaskHandle_t usb_task_handle;        /* USB任务句柄 */
    bool usb_task_running;               /* USB任务运行标志 */
    SemaphoreHandle_t mutex;             /* 互斥�?*/
} esp32_usb_t;

/* USB设备实例 */
static esp32_usb_t usb_instances[ESP32_USB_MAX_INSTANCES];

/* 前向声明 */
static void usb_task(void *arg);

/**
 * @brief 获取端点索引
 */
static int get_endpoint_index(esp32_usb_t *usb_dev, uint8_t ep_addr) {
    for (int i = 0; i < ESP32_USB_MAX_ENDPOINTS; i++) {
        if (usb_dev->endpoints[i].active && usb_dev->endpoints[i].ep_addr == ep_addr) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief TinyUSB设备事件回调
 */
static void tinyusb_device_event_cb(tinyusb_event_t event, void *arg) {
    esp32_usb_t *usb_dev = &usb_instances[0]; /* ESP32只有一个USB实例 */
    
    switch (event) {
        case TINYUSB_EVENT_MOUNT:
            ESP_LOGI(TAG, "USB设备已连�?);
            usb_dev->device_state = USB_DEVICE_CONNECTED;
            if (usb_dev->state_callback) {
                usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_CONNECTED);
            }
            break;
            
        case TINYUSB_EVENT_UNMOUNT:
            ESP_LOGI(TAG, "USB设备已断开");
            usb_dev->device_state = USB_DEVICE_DISCONNECTED;
            if (usb_dev->state_callback) {
                usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_DISCONNECTED);
            }
            break;
            
        case TINYUSB_EVENT_SUSPEND:
            ESP_LOGI(TAG, "USB设备已挂�?);
            usb_dev->device_state = USB_DEVICE_SUSPENDED;
            if (usb_dev->state_callback) {
                usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_SUSPENDED);
            }
            break;
            
        case TINYUSB_EVENT_RESUME:
            ESP_LOGI(TAG, "USB设备已恢�?);
            usb_dev->device_state = USB_DEVICE_RESUMED;
            if (usb_dev->state_callback) {
                usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_RESUMED);
            }
            break;
            
        case TINYUSB_EVENT_CONFIGURED:
            ESP_LOGI(TAG, "USB设备已配�?);
            usb_dev->device_state = USB_DEVICE_CONFIGURED;
            if (usb_dev->state_callback) {
                usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_CONFIGURED);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief CDC接收回调
 */
static void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t event, void *arg) {
    esp32_usb_t *usb_dev = &usb_instances[0]; /* ESP32只有一个USB实例 */
    
    if (event == CDC_EVENT_RX) {
        /* 查找匹配端点 */
        int ep_idx = get_endpoint_index(usb_dev, USB_CDC_OUT_EP);
        if (ep_idx >= 0 && usb_dev->endpoints[ep_idx].callback) {
            /* 读取接收到的数据 */
            size_t rx_size = 0;
            uint8_t buf[64];
            esp_err_t ret = tinyusb_cdcacm_read(itf, buf, sizeof(buf), &rx_size);
            
            if (ret == ESP_OK && rx_size > 0) {
                /* 构造传输结构体 */
                usb_transfer_t transfer;
                transfer.ep_addr = USB_CDC_OUT_EP;
                transfer.buffer = buf;
                transfer.length = sizeof(buf);
                transfer.actual_length = rx_size;
                transfer.type = USB_TRANSFER_DATA;
                transfer.user_data = NULL;
                
                /* 调用回调函数 */
                usb_dev->endpoints[ep_idx].callback(usb_dev->endpoints[ep_idx].callback_arg, USB_STATUS_COMPLETE, &transfer);
            }
        }
    }
}

/**
 * @brief CDC行编码回�?
 */
static void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t event, void *arg) {
    esp32_usb_t *usb_dev = &usb_instances[0]; /* ESP32只有一个USB实例 */
    
    if (event == CDC_EVENT_LINE_STATE_CHANGED) {
        /* 处理线路状态变�?*/
    }
}

/**
 * @brief USB任务
 */
static void usb_task(void *arg) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)arg;
    
    while (usb_dev->usb_task_running) {
        /* 处理USB事件 */
        tud_task();
        
        /* 让出CPU时间 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief 初始化USB设备
 */
int usb_init(const usb_config_t *config, usb_device_state_callback_t state_callback, void *arg, usb_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return -1;
    }
    
    /* 查找空闲实例 */
    esp32_usb_t *usb_dev = NULL;
    
    for (int i = 0; i < ESP32_USB_MAX_INSTANCES; i++) {
        if (!usb_instances[i].initialized) {
            usb_dev = &usb_instances[i];
            break;
        }
    }
    
    if (usb_dev == NULL) {
        return -1; /* 没有空闲实例 */
    }
    
    /* 初始化设备数�?*/
    memset(usb_dev, 0, sizeof(esp32_usb_t));
    usb_dev->state_callback = state_callback;
    usb_dev->state_callback_arg = arg;
    memcpy(&usb_dev->config, config, sizeof(usb_config_t));
    
    /* 创建互斥�?*/
    usb_dev->mutex = xSemaphoreCreateMutex();
    if (usb_dev->mutex == NULL) {
        return -1;
    }
    
    /* 配置TinyUSB */
    tinyusb_config_t tusb_cfg = {
        .descriptor = NULL, /* 使用TinyUSB默认描述�?*/
        .string_descriptor = NULL,
        .external_phy = false
    };
    
    /* 初始化TinyUSB设备堆栈 */
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    
    /* 注册TinyUSB事件回调 */
    tinyusb_set_mount_callback(tinyusb_device_event_cb, NULL);
    tinyusb_set_unmount_callback(tinyusb_device_event_cb, NULL);
    tinyusb_set_suspend_callback(tinyusb_device_event_cb, NULL);
    tinyusb_set_resume_callback(tinyusb_device_event_cb, NULL);
    
    /* 配置端点 */
    for (int i = 0; i < config->num_interfaces; i++) {
        const usb_interface_config_t *iface = &config->interfaces[i];
        
        /* 根据接口类型配置相应功能 */
        switch (iface->class_type) {
            case USB_CLASS_CDC:
                /* 配置CDC接口 */
                tinyusb_config_cdcacm_t cdc_cfg = {
                    .usb_dev = TINYUSB_USBDEV_0,
                    .cdc_port = TINYUSB_CDC_ACM_0,
                    .rx_unread_buf_sz = 64,
                    .callback_rx = tinyusb_cdc_rx_callback,
                    .callback_rx_wanted_char = NULL,
                    .callback_line_state_changed = tinyusb_cdc_line_state_changed_callback,
                    .callback_line_coding_changed = NULL
                };
                ESP_ERROR_CHECK(tusb_cdc_acm_init(&cdc_cfg));
                break;
                
            case USB_CLASS_HID:
                /* 配置HID接口 */
                /* 暂未实现 */
                break;
                
            case USB_CLASS_MASS_STORAGE:
                /* 配置MSC接口 */
                /* 暂未实现 */
                break;
                
            default:
                /* 其他接口类型暂不支持 */
                break;
        }
        
        /* 保存端点配置 */
        for (int j = 0; j < iface->num_endpoints; j++) {
            const usb_endpoint_config_t *ep_config = &iface->endpoints[j];
            
            /* 查找空闲端点槽位 */
            int ep_idx = -1;
            for (int k = 0; k < ESP32_USB_MAX_ENDPOINTS; k++) {
                if (!usb_dev->endpoints[k].active) {
                    ep_idx = k;
                    break;
                }
            }
            
            if (ep_idx < 0) {
                /* 清理资源 */
                xSemaphoreGive(usb_dev->mutex);
                vSemaphoreDelete(usb_dev->mutex);
                return -1; /* 没有空闲端点槽位 */
            }
            
            /* 保存端点配置 */
            usb_dev->endpoints[ep_idx].ep_addr = ep_config->ep_addr;
            usb_dev->endpoints[ep_idx].type = ep_config->type;
            usb_dev->endpoints[ep_idx].max_packet_size = ep_config->max_packet_size;
            usb_dev->endpoints[ep_idx].active = true;
        }
    }
    
    /* 创建USB任务 */
    usb_dev->usb_task_running = true;
    if (xTaskCreate(usb_task, "usb_task", 4096, usb_dev, 5, &usb_dev->usb_task_handle) != pdPASS) {
        /* 清理资源 */
        xSemaphoreGive(usb_dev->mutex);
        vSemaphoreDelete(usb_dev->mutex);
        return -1;
    }
    
    /* 标记为已初始�?*/
    usb_dev->initialized = true;
    usb_dev->status = USB_STATUS_IDLE;
    usb_dev->device_state = USB_DEVICE_DEFAULT;
    usb_dev->speed = USB_SPEED_FULL; /* ESP32只支持全速模�?*/
    
    /* 返回句柄 */
    *handle = (usb_handle_t)usb_dev;
    
    return 0;
}

/**
 * @brief 去初始化USB设备
 */
int usb_deinit(usb_handle_t handle) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 停止USB任务 */
    if (usb_dev->usb_task_handle != NULL) {
        usb_dev->usb_task_running = false;
        vTaskDelay(pdMS_TO_TICKS(200)); /* 等待任务结束 */
    }
    
    /* 停用TinyUSB */
    tinyusb_driver_uninstall();
    
    /* 清理资源 */
    if (usb_dev->mutex) {
        vSemaphoreDelete(usb_dev->mutex);
        usb_dev->mutex = NULL;
    }
    
    /* 清除设备数据 */
    usb_dev->initialized = false;
    
    return 0;
}

/**
 * @brief 启动USB设备
 */
int usb_start(usb_handle_t handle) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TinyUSB在初始化时已经启�?*/
    usb_dev->status = USB_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 停止USB设备
 */
int usb_stop(usb_handle_t handle) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接停止功能 */
    usb_dev->status = USB_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 设置USB设备地址
 */
int usb_set_address(usb_handle_t handle, uint8_t address) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* TinyUSB会自动处理设备地址设置 */
    usb_dev->device_state = USB_DEVICE_ADDRESS;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_ADDRESS);
    }
    
    return 0;
}

/**
 * @brief 设置USB设备配置
 */
int usb_set_configuration(usb_handle_t handle, uint8_t config_num) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* TinyUSB会自动处理设备配置设�?*/
    usb_dev->device_state = USB_DEVICE_CONFIGURED;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_CONFIGURED);
    }
    
    return 0;
}

/**
 * @brief 设置USB接口
 */
int usb_set_interface(usb_handle_t handle, uint8_t interface_num, uint8_t alt_setting) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* TinyUSB会自动处理接口设�?*/
    
    return 0;
}

/**
 * @brief 挂起USB设备
 */
int usb_suspend(usb_handle_t handle) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接挂起功能 */
    
    return 0;
}

/**
 * @brief 恢复USB设备
 */
int usb_resume(usb_handle_t handle) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接恢复功能 */
    
    return 0;
}

/**
 * @brief 获取USB设备速度
 */
int usb_get_speed(usb_handle_t handle, usb_speed_t *speed) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || speed == NULL) {
        return -1;
    }
    
    *speed = usb_dev->speed;
    
    return 0;
}

/**
 * @brief 获取USB设备状�?
 */
int usb_get_device_state(usb_handle_t handle, usb_device_state_t *state) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || state == NULL) {
        return -1;
    }
    
    *state = usb_dev->device_state;
    
    return 0;
}

/**
 * @brief 传输USB数据
 */
int usb_transfer(usb_handle_t handle, usb_transfer_t *transfer, usb_callback_t callback, void *arg) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || transfer == NULL) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, transfer->ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* 保存回调函数和数�?*/
    usb_dev->endpoints[ep_idx].callback = callback;
    usb_dev->endpoints[ep_idx].callback_arg = arg;
    usb_dev->endpoints[ep_idx].buffer = transfer->buffer;
    usb_dev->endpoints[ep_idx].buffer_size = transfer->length;
    
    /* 根据传输方向和端点类型进行操�?*/
    uint8_t ep_num = transfer->ep_addr & 0x7F;
    if (transfer->ep_addr & 0x80) {
        /* IN传输 */
        if (ep_num == USB_CDC_IN_EP) {
            /* CDC数据发�?*/
            if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, transfer->buffer, transfer->length) != ESP_OK) {
                return -1;
            }
            
            /* 刷新数据 */
            if (tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0) != ESP_OK) {
                return -1;
            }
        } else {
            /* 其他端点类型暂不支持 */
            return -1;
        }
    } else {
        /* OUT传输 */
        /* ESP32 TinyUSB会通过回调处理接收数据 */
    }
    
    return 0;
}

/**
 * @brief 取消USB传输
 */
int usb_cancel_transfer(usb_handle_t handle, uint8_t ep_addr) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接取消传输的功�?*/
    
    return 0;
}

/**
 * @brief 获取USB设备描述�?
 */
int usb_get_descriptor(usb_handle_t handle, uint8_t type, uint8_t index, uint16_t lang_id, uint8_t *data, uint16_t length) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || data == NULL) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接访问描述符的功能 */
    /* 如果在初始化时提供了描述符，可以从配置中获取 */
    
    switch (type) {
        case USB_DESC_TYPE_DEVICE:
            if (usb_dev->config.device_descriptor.buffer == NULL) {
                return -1;
            }
            memcpy(data, usb_dev->config.device_descriptor.buffer, 
                   length < usb_dev->config.device_descriptor.length ? length : usb_dev->config.device_descriptor.length);
            return 0;
            
        case USB_DESC_TYPE_CONFIGURATION:
            if (usb_dev->config.config_descriptor.buffer == NULL) {
                return -1;
            }
            memcpy(data, usb_dev->config.config_descriptor.buffer, 
                   length < usb_dev->config.config_descriptor.length ? length : usb_dev->config.config_descriptor.length);
            return 0;
            
        case USB_DESC_TYPE_STRING:
            if (index >= usb_dev->config.num_string_descriptors || usb_dev->config.string_descriptors == NULL) {
                return -1;
            }
            memcpy(data, usb_dev->config.string_descriptors[index].buffer, 
                   length < usb_dev->config.string_descriptors[index].length ? length : usb_dev->config.string_descriptors[index].length);
            return 0;
            
        default:
            return -1;
    }
}

/**
 * @brief 清除端点暂停状�?
 */
int usb_clear_halt(usb_handle_t handle, uint8_t ep_addr) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接清除端点暂停状态的功能 */
    
    return 0;
}

/**
 * @brief 获取端点状�?
 */
int usb_get_endpoint_status(usb_handle_t handle, uint8_t ep_addr, bool *halted) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || halted == NULL) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* ESP32 TinyUSB没有提供直接获取端点状态的功能 */
    /* 默认设置为非暂停状�?*/
    *halted = false;
    
    return 0;
}

/**
 * @brief 注册端点回调函数
 */
int usb_register_endpoint_callback(usb_handle_t handle, uint8_t ep_addr, usb_callback_t callback, void *arg) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* 注册回调函数 */
    usb_dev->endpoints[ep_idx].callback = callback;
    usb_dev->endpoints[ep_idx].callback_arg = arg;
    
    return 0;
}

/**
 * @brief 获取USB操作状�?
 */
int usb_get_status(usb_handle_t handle, usb_status_t *status) {
    esp32_usb_t *usb_dev = (esp32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || status == NULL) {
        return -1;
    }
    
    *status = usb_dev->status;
    
    return 0;
}

/* USB主机模式函数 */

/**
 * @brief 枚举USB设备
 */
int usb_host_enumerate_device(usb_handle_t handle, uint8_t port, usb_host_device_info_t *device_info) {
    /* ESP32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 打开USB设备
 */
int usb_host_open_device(usb_handle_t handle, uint8_t address, usb_handle_t *device_handle) {
    /* ESP32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 关闭USB设备
 */
int usb_host_close_device(usb_handle_t device_handle) {
    /* ESP32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 获取连接的设备数�?
 */
int usb_host_get_device_count(usb_handle_t handle, uint8_t *count) {
    /* ESP32 USB主机模式暂不支持 */
    if (count) {
        *count = 0;
    }
    return -1;
}

/**
 * @brief 获取设备信息
 */
int usb_host_get_device_info(usb_handle_t device_handle, usb_host_device_info_t *device_info) {
    /* ESP32 USB主机模式暂不支持 */
    return -1;
}
