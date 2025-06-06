/**
 * @file stm32_usb.c
 * @brief STM32 USB驱动实现
 */

#include "base/usb_api.h"
#include "stm32_platform.h"
#include <string.h>

/* 私有定义 */
#define STM32_USB_MAX_ENDPOINTS    8  /* STM32 USB最大端点数 */
#define STM32_USB_MAX_INSTANCES    2  /* STM32 USB最大实例数�?*/

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
} stm32_usb_endpoint_t;

/* USB设备数据结构 */
typedef struct {
    PCD_HandleTypeDef hpcd;              /* HAL PCD句柄 */
    USBD_HandleTypeDef husb;             /* HAL USB设备句柄 */
    stm32_usb_endpoint_t endpoints[STM32_USB_MAX_ENDPOINTS]; /* 端点数据 */
    usb_device_state_callback_t state_callback; /* 设备状态回调函�?*/
    void *state_callback_arg;            /* 回调函数参数 */
    bool initialized;                    /* 初始化标�?*/
    usb_config_t config;                 /* USB配置参数 */
    usb_status_t status;                 /* 设备状�?*/
    usb_device_state_t device_state;     /* USB设备状�?*/
    usb_speed_t speed;                   /* USB速度 */
} stm32_usb_t;

/* USB设备实例 */
static stm32_usb_t usb_instances[STM32_USB_MAX_INSTANCES];

/* 前向声明 */
static void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum);
static void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum);
static void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum);
static void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum);
static void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd);
static void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd);

/**
 * @brief 获取USB设备实例
 */
static stm32_usb_t *get_usb_instance_from_pcd(PCD_HandleTypeDef *hpcd) {
    for (int i = 0; i < STM32_USB_MAX_INSTANCES; i++) {
        if (usb_instances[i].initialized && &usb_instances[i].hpcd == hpcd) {
            return &usb_instances[i];
        }
    }
    return NULL;
}

/**
 * @brief 获取端点索引
 */
static int get_endpoint_index(stm32_usb_t *usb_dev, uint8_t ep_addr) {
    for (int i = 0; i < STM32_USB_MAX_ENDPOINTS; i++) {
        if (usb_dev->endpoints[i].active && usb_dev->endpoints[i].ep_addr == ep_addr) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief USB设置阶段回调
 */
static void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_SetupStage(&usb_dev->husb, (uint8_t *)hpcd->Setup);
}

/**
 * @brief USB数据输出阶段回调
 */
static void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_DataOutStage(&usb_dev->husb, epnum, hpcd->OUT_ep[epnum].xfer_buff);
    
    /* 查找对应端点 */
    int ep_idx = get_endpoint_index(usb_dev, epnum);
    if (ep_idx >= 0 && usb_dev->endpoints[ep_idx].callback != NULL) {
        /* 构造传输结构体 */
        usb_transfer_t transfer;
        transfer.ep_addr = epnum;
        transfer.buffer = usb_dev->endpoints[ep_idx].buffer;
        transfer.length = usb_dev->endpoints[ep_idx].buffer_size;
        transfer.actual_length = USBD_LL_GetRxDataSize(&usb_dev->husb, epnum);
        transfer.type = USB_TRANSFER_DATA;
        transfer.user_data = NULL;
        
        /* 调用回调函数 */
        usb_dev->endpoints[ep_idx].callback(usb_dev->endpoints[ep_idx].callback_arg, USB_STATUS_COMPLETE, &transfer);
    }
}

/**
 * @brief USB数据输入阶段回调
 */
static void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_DataInStage(&usb_dev->husb, epnum, hpcd->IN_ep[epnum].xfer_buff);
    
    /* 查找对应端点 */
    int ep_idx = get_endpoint_index(usb_dev, epnum | 0x80);
    if (ep_idx >= 0 && usb_dev->endpoints[ep_idx].callback != NULL) {
        /* 构造传输结构体 */
        usb_transfer_t transfer;
        transfer.ep_addr = epnum | 0x80;
        transfer.buffer = usb_dev->endpoints[ep_idx].buffer;
        transfer.length = usb_dev->endpoints[ep_idx].buffer_size;
        transfer.actual_length = transfer.length - USBD_LL_GetTxDataSize(&usb_dev->husb, epnum);
        transfer.type = USB_TRANSFER_DATA;
        transfer.user_data = NULL;
        
        /* 调用回调函数 */
        usb_dev->endpoints[ep_idx].callback(usb_dev->endpoints[ep_idx].callback_arg, USB_STATUS_COMPLETE, &transfer);
    }
}

/**
 * @brief USB SOF回调
 */
static void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_SOF(&usb_dev->husb);
}

/**
 * @brief USB复位回调
 */
static void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;
    
    /* 设置USB速度 */
    switch (hpcd->Init.speed) {
        case PCD_SPEED_HIGH:
            speed = USBD_SPEED_HIGH;
            usb_dev->speed = USB_SPEED_HIGH;
            break;
        case PCD_SPEED_FULL:
            speed = USBD_SPEED_FULL;
            usb_dev->speed = USB_SPEED_FULL;
            break;
        default:
            speed = USBD_SPEED_FULL;
            usb_dev->speed = USB_SPEED_FULL;
            break;
    }
    
    USBD_LL_SetSpeed(&usb_dev->husb, speed);
    USBD_LL_Reset(&usb_dev->husb);
    
    /* 更新设备状�?*/
    usb_dev->device_state = USB_DEVICE_DEFAULT;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_DEFAULT);
    }
}

/**
 * @brief USB挂起回调
 */
static void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_Suspend(&usb_dev->husb);
    
    /* 更新设备状�?*/
    usb_dev->device_state = USB_DEVICE_SUSPENDED;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_SUSPENDED);
    }
}

/**
 * @brief USB恢复回调
 */
static void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_Resume(&usb_dev->husb);
    
    /* 更新设备状�?*/
    usb_dev->device_state = USB_DEVICE_RESUMED;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_RESUMED);
    }
}

/**
 * @brief USB ISO OUT不完整回�?
 */
static void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_IsoOUTIncomplete(&usb_dev->husb, epnum);
}

/**
 * @brief USB ISO IN不完整回�?
 */
static void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_IsoINIncomplete(&usb_dev->husb, epnum);
}

/**
 * @brief USB连接回调
 */
static void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_DevConnected(&usb_dev->husb);
    
    /* 更新设备状�?*/
    usb_dev->device_state = USB_DEVICE_CONNECTED;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_CONNECTED);
    }
}

/**
 * @brief USB断开连接回调
 */
static void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
    stm32_usb_t *usb_dev = get_usb_instance_from_pcd(hpcd);
    if (usb_dev == NULL) {
        return;
    }
    
    USBD_LL_DevDisconnected(&usb_dev->husb);
    
    /* 更新设备状�?*/
    usb_dev->device_state = USB_DEVICE_DISCONNECTED;
    
    /* 通知用户回调 */
    if (usb_dev->state_callback != NULL) {
        usb_dev->state_callback(usb_dev->state_callback_arg, USB_DEVICE_DISCONNECTED);
    }
}

/**
 * @brief 初始化USB设备
 */
int usb_init(const usb_config_t *config, usb_device_state_callback_t state_callback, void *arg, usb_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return -1;
    }
    
    /* 查找空闲实例 */
    stm32_usb_t *usb_dev = NULL;
    int instance_idx = -1;
    
    for (int i = 0; i < STM32_USB_MAX_INSTANCES; i++) {
        if (!usb_instances[i].initialized) {
            usb_dev = &usb_instances[i];
            instance_idx = i;
            break;
        }
    }
    
    if (usb_dev == NULL) {
        return -1; /* 没有空闲实例 */
    }
    
    /* 初始化设备数�?*/
    memset(usb_dev, 0, sizeof(stm32_usb_t));
    usb_dev->state_callback = state_callback;
    usb_dev->state_callback_arg = arg;
    memcpy(&usb_dev->config, config, sizeof(usb_config_t));
    
    /* 根据实例索引选择USB外设 */
    if (instance_idx == 0) {
        usb_dev->hpcd.Instance = USB_OTG_FS;
        usb_dev->hpcd.Init.dev_endpoints = 4;
        usb_dev->hpcd.Init.use_dedicated_ep1 = 0;
        usb_dev->hpcd.Init.dma_enable = 0;
        usb_dev->hpcd.Init.low_power_enable = 0;
        usb_dev->hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
        usb_dev->hpcd.Init.Sof_enable = 1;
        usb_dev->hpcd.Init.speed = PCD_SPEED_FULL;
        usb_dev->hpcd.Init.vbus_sensing_enable = 0;
    } 
#ifdef USB_OTG_HS
    else if (instance_idx == 1) {
        usb_dev->hpcd.Instance = USB_OTG_HS;
        usb_dev->hpcd.Init.dev_endpoints = 8;
        usb_dev->hpcd.Init.use_dedicated_ep1 = 0;
        usb_dev->hpcd.Init.dma_enable = 0;
        usb_dev->hpcd.Init.low_power_enable = 0;
        usb_dev->hpcd.Init.phy_itface = PCD_PHY_ULPI;
        usb_dev->hpcd.Init.Sof_enable = 1;
        usb_dev->hpcd.Init.speed = PCD_SPEED_HIGH;
        usb_dev->hpcd.Init.vbus_sensing_enable = 1;
    }
#endif
    else {
        return -1;
    }
    
    /* 初始化USB外设 */
    if (HAL_PCD_Init(&usb_dev->hpcd) != HAL_OK) {
        return -1;
    }
    
    /* 注册回调函数 */
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_SETUP_STAGE_CB_ID, HAL_PCD_SetupStageCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_DATA_OUT_STAGE_CB_ID, HAL_PCD_DataOutStageCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_DATA_IN_STAGE_CB_ID, HAL_PCD_DataInStageCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_SOF_CB_ID, HAL_PCD_SOFCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_RESET_CB_ID, HAL_PCD_ResetCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_SUSPEND_CB_ID, HAL_PCD_SuspendCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_RESUME_CB_ID, HAL_PCD_ResumeCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_CONNECT_CB_ID, HAL_PCD_ConnectCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_DISCONNECT_CB_ID, HAL_PCD_DisconnectCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_ISO_OUT_INCOMPLETE_CB_ID, HAL_PCD_ISOOUTIncompleteCallback);
    HAL_PCD_RegisterCallback(&usb_dev->hpcd, HAL_PCD_ISO_IN_INCOMPLETE_CB_ID, HAL_PCD_ISOINIncompleteCallback);
    
    /* 初始化USB设备�?*/
    USBD_Init(&usb_dev->husb, NULL, 0);
    
    /* 配置端点 */
    for (int i = 0; i < config->num_interfaces; i++) {
        const usb_interface_config_t *iface = &config->interfaces[i];
        for (int j = 0; j < iface->num_endpoints; j++) {
            const usb_endpoint_config_t *ep_config = &iface->endpoints[j];
            
            /* 查找空闲端点槽位 */
            int ep_idx = -1;
            for (int k = 0; k < STM32_USB_MAX_ENDPOINTS; k++) {
                if (!usb_dev->endpoints[k].active) {
                    ep_idx = k;
                    break;
                }
            }
            
            if (ep_idx < 0) {
                HAL_PCD_DeInit(&usb_dev->hpcd);
                return -1; /* 没有空闲端点槽位 */
            }
            
            /* 保存端点配置 */
            usb_dev->endpoints[ep_idx].ep_addr = ep_config->ep_addr;
            usb_dev->endpoints[ep_idx].type = ep_config->type;
            usb_dev->endpoints[ep_idx].max_packet_size = ep_config->max_packet_size;
            usb_dev->endpoints[ep_idx].active = true;
            
            /* 打开端点 */
            uint8_t ep_num = ep_config->ep_addr & 0x7F;
            uint8_t ep_dir = ep_config->ep_addr & 0x80 ? PCD_ENDP_IN : PCD_ENDP_OUT;
            uint8_t ep_type;
            
            switch (ep_config->type) {
                case USB_ENDPOINT_CONTROL:
                    ep_type = EP_TYPE_CTRL;
                    break;
                case USB_ENDPOINT_ISOCHRONOUS:
                    ep_type = EP_TYPE_ISOC;
                    break;
                case USB_ENDPOINT_BULK:
                    ep_type = EP_TYPE_BULK;
                    break;
                case USB_ENDPOINT_INTERRUPT:
                    ep_type = EP_TYPE_INTR;
                    break;
                default:
                    ep_type = EP_TYPE_BULK;
                    break;
            }
            
            HAL_PCD_EP_Open(&usb_dev->hpcd, ep_config->ep_addr, ep_config->max_packet_size, ep_type);
        }
    }
    
    /* 标记为已初始�?*/
    usb_dev->initialized = true;
    usb_dev->status = USB_STATUS_IDLE;
    usb_dev->device_state = USB_DEVICE_DEFAULT;
    
    /* 返回句柄 */
    *handle = (usb_handle_t)usb_dev;
    
    return 0;
}

/**
 * @brief 去初始化USB设备
 */
int usb_deinit(usb_handle_t handle) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 停止USB外设 */
    HAL_PCD_Stop(&usb_dev->hpcd);
    HAL_PCD_DeInit(&usb_dev->hpcd);
    
    /* 清除设备数据 */
    usb_dev->initialized = false;
    
    return 0;
}

/**
 * @brief 启动USB设备
 */
int usb_start(usb_handle_t handle) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 启动USB外设 */
    HAL_PCD_Start(&usb_dev->hpcd);
    
    usb_dev->status = USB_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 停止USB设备
 */
int usb_stop(usb_handle_t handle) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 停止USB外设 */
    HAL_PCD_Stop(&usb_dev->hpcd);
    
    usb_dev->status = USB_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 设置USB设备地址
 */
int usb_set_address(usb_handle_t handle, uint8_t address) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 设置设备地址 */
    HAL_PCD_SetAddress(&usb_dev->hpcd, address);
    
    /* 更新设备状�?*/
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 更新设备状�?*/
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 在此处理接口切换，如有必�?*/
    
    return 0;
}

/**
 * @brief 挂起USB设备
 */
int usb_suspend(usb_handle_t handle) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 挂起USB设备 */
    HAL_PCD_DevDisconnect(&usb_dev->hpcd);
    
    return 0;
}

/**
 * @brief 恢复USB设备
 */
int usb_resume(usb_handle_t handle) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 恢复USB设备 */
    HAL_PCD_DevConnect(&usb_dev->hpcd);
    
    return 0;
}

/**
 * @brief 获取USB设备速度
 */
int usb_get_speed(usb_handle_t handle, usb_speed_t *speed) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
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
    
    /* 根据传输方向进行操作 */
    uint8_t ep_num = transfer->ep_addr & 0x7F;
    if (transfer->ep_addr & 0x80) {
        /* IN传输 */
        HAL_PCD_EP_Transmit(&usb_dev->hpcd, ep_num, transfer->buffer, transfer->length);
    } else {
        /* OUT传输 */
        HAL_PCD_EP_Receive(&usb_dev->hpcd, ep_num, transfer->buffer, transfer->length);
    }
    
    return 0;
}

/**
 * @brief 取消USB传输
 */
int usb_cancel_transfer(usb_handle_t handle, uint8_t ep_addr) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* 刷新端点 */
    uint8_t ep_num = ep_addr & 0x7F;
    HAL_PCD_EP_Flush(&usb_dev->hpcd, ep_num);
    
    return 0;
}

/**
 * @brief 获取USB设备描述�?
 */
int usb_get_descriptor(usb_handle_t handle, uint8_t type, uint8_t index, uint16_t lang_id, uint8_t *data, uint16_t length) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || data == NULL) {
        return -1;
    }
    
    /* 获取请求的描述符 */
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* 清除端点暂停状�?*/
    uint8_t ep_num = ep_addr & 0x7F;
    HAL_PCD_EP_ClrStall(&usb_dev->hpcd, ep_addr);
    
    return 0;
}

/**
 * @brief 获取端点状�?
 */
int usb_get_endpoint_status(usb_handle_t handle, uint8_t ep_addr, bool *halted) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
    if (usb_dev == NULL || !usb_dev->initialized || halted == NULL) {
        return -1;
    }
    
    /* 查找端点 */
    int ep_idx = get_endpoint_index(usb_dev, ep_addr);
    if (ep_idx < 0) {
        return -1;
    }
    
    /* 获取端点状�?*/
    uint8_t ep_num = ep_addr & 0x7F;
    if (ep_addr & 0x80) {
        /* IN端点 */
        *halted = (usb_dev->hpcd.IN_ep[ep_num].is_stall == 1);
    } else {
        /* OUT端点 */
        *halted = (usb_dev->hpcd.OUT_ep[ep_num].is_stall == 1);
    }
    
    return 0;
}

/**
 * @brief 注册端点回调函数
 */
int usb_register_endpoint_callback(usb_handle_t handle, uint8_t ep_addr, usb_callback_t callback, void *arg) {
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
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
    stm32_usb_t *usb_dev = (stm32_usb_t *)handle;
    
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
    /* STM32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 打开USB设备
 */
int usb_host_open_device(usb_handle_t handle, uint8_t address, usb_handle_t *device_handle) {
    /* STM32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 关闭USB设备
 */
int usb_host_close_device(usb_handle_t device_handle) {
    /* STM32 USB主机模式暂不支持 */
    return -1;
}

/**
 * @brief 获取连接的设备数�?
 */
int usb_host_get_device_count(usb_handle_t handle, uint8_t *count) {
    /* STM32 USB主机模式暂不支持 */
    if (count) {
        *count = 0;
    }
    return -1;
}

/**
 * @brief 获取设备信息
 */
int usb_host_get_device_info(usb_handle_t device_handle, usb_host_device_info_t *device_info) {
    /* STM32 USB主机模式暂不支持 */
    return -1;
}
