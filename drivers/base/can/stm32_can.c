/**
 * @file stm32_can.c
 * @brief STM32 CAN驱动实现
 */

#include "base/can_api.h"
#include "stm32_platform.h"
#include <string.h>

/* 私有定义 */
#define STM32_CAN_MAX_FILTERS      14  /* STM32 CAN外设最大过滤器数量 */
#define STM32_CAN_MAX_INSTANCES    2   /* STM32 CAN外设最大实例数�?*/

/* 波特率预分频和时序定�?*/
typedef struct {
    uint32_t prescaler;    /* 预分频器�?*/
    uint8_t bs1;           /* 位段1时间 */
    uint8_t bs2;           /* 位段2时间 */
    uint8_t sjw;           /* 重同步跳跃宽�?*/
} stm32_can_timing_t;

/* 波特率定义表 */
static const stm32_can_timing_t stm32_can_timing[] = {
    [CAN_BAUDRATE_10K]  = { .prescaler = 450, .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 10 kbit/s @36MHz */
    [CAN_BAUDRATE_20K]  = { .prescaler = 225, .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 20 kbit/s */
    [CAN_BAUDRATE_50K]  = { .prescaler = 90,  .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 50 kbit/s */
    [CAN_BAUDRATE_100K] = { .prescaler = 45,  .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 100 kbit/s */
    [CAN_BAUDRATE_125K] = { .prescaler = 36,  .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 125 kbit/s */
    [CAN_BAUDRATE_250K] = { .prescaler = 18,  .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 250 kbit/s */
    [CAN_BAUDRATE_500K] = { .prescaler = 9,   .bs1 = 13, .bs2 = 2, .sjw = 1 }, /* 500 kbit/s */
    [CAN_BAUDRATE_800K] = { .prescaler = 9,   .bs1 = 7,  .bs2 = 2, .sjw = 1 }, /* 800 kbit/s */
    [CAN_BAUDRATE_1M]   = { .prescaler = 4,   .bs1 = 15, .bs2 = 2, .sjw = 1 }  /* 1 Mbit/s */
};

/* STM32 CAN设备数据结构 */
typedef struct {
    CAN_HandleTypeDef hcan;                /* HAL CAN句柄 */
    CAN_FilterTypeDef filter[STM32_CAN_MAX_FILTERS]; /* 过滤器配�?*/
    uint32_t filter_bank_mask;              /* 过滤器组使用掩码 */
    can_callback_t callback;                /* 用户回调函数 */
    void *callback_arg;                     /* 回调函数参数 */
    bool initialized;                       /* 初始化标�?*/
    can_config_t config;                    /* CAN配置参数 */
    can_stats_t stats;                      /* 统计信息 */
    can_status_t status;                    /* 设备状�?*/
    bool rx_pending;                        /* 接收挂起标志 */
    can_message_t rx_msg;                   /* 接收消息缓存 */
    uint32_t timestamp;                     /* 时间�?*/
} stm32_can_t;

/* CAN设备实例 */
static stm32_can_t can_instances[STM32_CAN_MAX_INSTANCES];

/**
 * @brief CAN接收中断回调函数
 */
static void stm32_can_rx_callback(CAN_HandleTypeDef *hcan, uint32_t fifo) {
    stm32_can_t *can_dev = NULL;
    
    /* 查找对应的设备实�?*/
    for (int i = 0; i < STM32_CAN_MAX_INSTANCES; i++) {
        if (can_instances[i].initialized && &can_instances[i].hcan == hcan) {
            can_dev = &can_instances[i];
            break;
        }
    }
    
    if (can_dev == NULL) {
        return;
    }
    
    /* 接收CAN消息 */
    CAN_RxHeaderTypeDef rx_header;
    uint8_t data[8];
    
    if (HAL_CAN_GetRxMessage(hcan, fifo, &rx_header, data) != HAL_OK) {
        can_dev->stats.error_count++;
        return;
    }
    
    can_dev->stats.rx_count++;
    
    /* 填充接收消息结构 */
    can_message_t msg;
    msg.id = rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId;
    msg.id_type = rx_header.IDE == CAN_ID_STD ? CAN_ID_STANDARD : CAN_ID_EXTENDED;
    msg.frame_type = rx_header.RTR == CAN_RTR_DATA ? CAN_FRAME_DATA : CAN_FRAME_REMOTE;
    msg.dlc = rx_header.DLC;
    memcpy(msg.data, data, msg.dlc);
    msg.timestamp = can_dev->timestamp++;
    
    /* 存储消息并设置标�?*/
    memcpy(&can_dev->rx_msg, &msg, sizeof(can_message_t));
    can_dev->rx_pending = true;
    
    /* 调用用户回调函数 */
    if (can_dev->callback != NULL) {
        can_dev->callback(can_dev->callback_arg, CAN_STATUS_COMPLETE, &msg);
    }
}

/**
 * @brief CAN错误中断回调函数
 */
static void stm32_can_error_callback(CAN_HandleTypeDef *hcan) {
    stm32_can_t *can_dev = NULL;
    
    /* 查找对应的设备实�?*/
    for (int i = 0; i < STM32_CAN_MAX_INSTANCES; i++) {
        if (can_instances[i].initialized && &can_instances[i].hcan == hcan) {
            can_dev = &can_instances[i];
            break;
        }
    }
    
    if (can_dev == NULL) {
        return;
    }
    
    can_dev->stats.error_count++;
    
    /* 检查错误类�?*/
    uint32_t error = HAL_CAN_GetError(hcan);
    
    /* 更新统计信息 */
    if (error & HAL_CAN_ERROR_BOF) {
        can_dev->stats.bus_off = true;
    }
    
    if (error & HAL_CAN_ERROR_EPV) {
        can_dev->stats.passive_error = true;
    }
    
    if (error & HAL_CAN_ERROR_STUFF) {
        /* 位填充错�?*/
    }
    
    if (error & HAL_CAN_ERROR_CRC) {
        /* CRC错误 */
    }
    
    if (error & HAL_CAN_ERROR_FOR) {
        /* 格式错误 */
    }
    
    if (error & HAL_CAN_ERROR_ACK) {
        /* 应答错误 */
    }
    
    /* 调用用户回调函数 */
    if (can_dev->callback != NULL) {
        can_dev->callback(can_dev->callback_arg, CAN_STATUS_ERROR, NULL);
    }
}

/**
 * @brief 初始化CAN设备
 */
int can_init(const can_config_t *config, can_callback_t callback, void *arg, can_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return -1;
    }
    
    /* 查找空闲实例 */
    stm32_can_t *can_dev = NULL;
    int instance_idx = -1;
    
    for (int i = 0; i < STM32_CAN_MAX_INSTANCES; i++) {
        if (!can_instances[i].initialized) {
            can_dev = &can_instances[i];
            instance_idx = i;
            break;
        }
    }
    
    if (can_dev == NULL) {
        return -1; /* 没有空闲实例 */
    }
    
    /* 初始化设备数�?*/
    memset(can_dev, 0, sizeof(stm32_can_t));
    can_dev->callback = callback;
    can_dev->callback_arg = arg;
    memcpy(&can_dev->config, config, sizeof(can_config_t));
    
    /* 初始化HAL CAN句柄 */
    CAN_HandleTypeDef *hcan = &can_dev->hcan;
    
    /* 根据实例索引选择CAN外设 */
    if (instance_idx == 0) {
        hcan->Instance = CAN1;
    } 
#ifdef CAN2
    else if (instance_idx == 1) {
        hcan->Instance = CAN2;
    }
#endif
    else {
        return -1;
    }
    
    /* 配置CAN参数 */
    const stm32_can_timing_t *timing = &stm32_can_timing[config->baudrate];
    
    hcan->Init.Prescaler = timing->prescaler;
    hcan->Init.Mode = config->mode == CAN_MODE_LOOPBACK ? CAN_MODE_LOOPBACK :
                      config->mode == CAN_MODE_SILENT ? CAN_MODE_SILENT :
                      config->mode == CAN_MODE_SILENT_LOOPBACK ? CAN_MODE_SILENT_LOOPBACK :
                      CAN_MODE_NORMAL;
    hcan->Init.SyncJumpWidth = timing->sjw;
    hcan->Init.TimeSeg1 = timing->bs1;
    hcan->Init.TimeSeg2 = timing->bs2;
    hcan->Init.TimeTriggeredMode = DISABLE;
    hcan->Init.AutoBusOff = config->auto_bus_off_recovery ? ENABLE : DISABLE;
    hcan->Init.AutoWakeUp = DISABLE;
    hcan->Init.AutoRetransmission = config->auto_retransmit ? ENABLE : DISABLE;
    hcan->Init.ReceiveFifoLocked = config->rx_fifo_locked_mode ? ENABLE : DISABLE;
    hcan->Init.TransmitFifoPriority = config->tx_fifo_priority ? ENABLE : DISABLE;
    
    /* 初始化CAN外设 */
    if (HAL_CAN_Init(hcan) != HAL_OK) {
        return -1;
    }
    
    /* 启用中断 */
    if (HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, stm32_can_rx_callback) != HAL_OK) {
        HAL_CAN_DeInit(hcan);
        return -1;
    }
    
    if (HAL_CAN_RegisterCallback(hcan, HAL_CAN_ERROR_CB_ID, stm32_can_error_callback) != HAL_OK) {
        HAL_CAN_DeInit(hcan);
        return -1;
    }
    
    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING | 
                                           CAN_IT_ERROR | 
                                           CAN_IT_BUSOFF | 
                                           CAN_IT_LAST_ERROR_CODE) != HAL_OK) {
        HAL_CAN_DeInit(hcan);
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
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 停止CAN外设 */
    HAL_CAN_Stop(&can_dev->hcan);
    HAL_CAN_DeInit(&can_dev->hcan);
    
    /* 清除设备数据 */
    can_dev->initialized = false;
    
    return 0;
}

/**
 * @brief 配置CAN过滤�?
 */
int can_config_filter(can_handle_t handle, const can_filter_t *filter) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || filter == NULL) {
        return -1;
    }
    
    if (filter->filter_num >= STM32_CAN_MAX_FILTERS) {
        return -1;
    }
    
    /* 检查过滤器组是否已经被使用 */
    if (filter->active && (can_dev->filter_bank_mask & (1 << filter->filter_num))) {
        /* 已使用，先禁�?*/
        CAN_FilterTypeDef sFilterConfig = can_dev->filter[filter->filter_num];
        sFilterConfig.FilterActivation = DISABLE;
        
        if (HAL_CAN_ConfigFilter(&can_dev->hcan, &sFilterConfig) != HAL_OK) {
            return -1;
        }
    }
    
    /* 配置过滤�?*/
    CAN_FilterTypeDef sFilterConfig;
    
    /* 设置过滤器组�?*/
    sFilterConfig.FilterBank = filter->filter_num;
    
    /* 设置FIFO */
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    
    /* 设置过滤器模�?*/
    sFilterConfig.FilterMode = filter->type == CAN_FILTER_MASK ? CAN_FILTERMODE_IDMASK : CAN_FILTERMODE_IDLIST;
    
    /* 设置过滤器尺�?*/
    sFilterConfig.FilterScale = filter->id_type == CAN_ID_STANDARD ? CAN_FILTERSCALE_16BIT : CAN_FILTERSCALE_32BIT;
    
    /* 设置过滤器ID */
    if (filter->id_type == CAN_ID_STANDARD) {
        /* 标准ID模式 */
        sFilterConfig.FilterIdHigh = (filter->id << 5) & 0xFFFF;
        sFilterConfig.FilterIdLow = 0;
        sFilterConfig.FilterMaskIdHigh = (filter->mask << 5) & 0xFFFF;
        sFilterConfig.FilterMaskIdLow = 0;
    } else {
        /* 扩展ID模式 */
        sFilterConfig.FilterIdHigh = (filter->id >> 13) & 0xFFFF;
        sFilterConfig.FilterIdLow = ((filter->id << 3) & 0xFFF8) | 0x04; /* IDE位设置为1 */
        sFilterConfig.FilterMaskIdHigh = (filter->mask >> 13) & 0xFFFF;
        sFilterConfig.FilterMaskIdLow = ((filter->mask << 3) & 0xFFF8) | 0x04;
    }
    
    /* 设置是否激�?*/
    sFilterConfig.FilterActivation = filter->active ? ENABLE : DISABLE;
    
    /* 保存过滤器配�?*/
    memcpy(&can_dev->filter[filter->filter_num], &sFilterConfig, sizeof(CAN_FilterTypeDef));
    
    /* 更新过滤器组使用掩码 */
    if (filter->active) {
        can_dev->filter_bank_mask |= (1 << filter->filter_num);
    } else {
        can_dev->filter_bank_mask &= ~(1 << filter->filter_num);
    }
    
    /* 应用过滤器配�?*/
    if (HAL_CAN_ConfigFilter(&can_dev->hcan, &sFilterConfig) != HAL_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 启动CAN设备
 */
int can_start(can_handle_t handle) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 启动CAN外设 */
    if (HAL_CAN_Start(&can_dev->hcan) != HAL_OK) {
        return -1;
    }
    
    can_dev->status = CAN_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 停止CAN设备
 */
int can_stop(can_handle_t handle) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 停止CAN外设 */
    if (HAL_CAN_Stop(&can_dev->hcan) != HAL_OK) {
        return -1;
    }
    
    can_dev->status = CAN_STATUS_IDLE;
    
    return 0;
}

/**
 * @brief 发送CAN消息
 */
int can_transmit(can_handle_t handle, const can_message_t *msg, uint32_t timeout_ms) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || msg == NULL) {
        return -1;
    }
    
    /* 检查CAN设备状�?*/
    if (HAL_CAN_GetState(&can_dev->hcan) != HAL_CAN_STATE_READY) {
        return -1;
    }
    
    /* 检查是否有空闲邮箱 */
    uint32_t mailbox;
    if (HAL_CAN_GetTxMailboxesFreeLevel(&can_dev->hcan) == 0) {
        return -1;
    }
    
    /* 准备发送数�?*/
    CAN_TxHeaderTypeDef tx_header;
    
    if (msg->id_type == CAN_ID_STANDARD) {
        tx_header.StdId = msg->id;
        tx_header.IDE = CAN_ID_STD;
    } else {
        tx_header.ExtId = msg->id;
        tx_header.IDE = CAN_ID_EXT;
    }
    
    tx_header.RTR = msg->frame_type == CAN_FRAME_DATA ? CAN_RTR_DATA : CAN_RTR_REMOTE;
    tx_header.DLC = msg->dlc;
    tx_header.TransmitGlobalTime = DISABLE;
    
    /* 发送消�?*/
    if (HAL_CAN_AddTxMessage(&can_dev->hcan, &tx_header, (uint8_t *)msg->data, &mailbox) != HAL_OK) {
        return -1;
    }
    
    /* 等待发送完�?*/
    uint32_t start_time = HAL_GetTick();
    while (HAL_CAN_IsTxMessagePending(&can_dev->hcan, mailbox)) {
        if (timeout_ms > 0 && (HAL_GetTick() - start_time > timeout_ms)) {
            return -1; /* 超时 */
        }
        
        /* 让出CPU时间 */
        if (timeout_ms > 10) {
            HAL_Delay(1);
        }
    }
    
    /* 更新统计信息 */
    can_dev->stats.tx_count++;
    
    return 0;
}

/**
 * @brief 接收CAN消息
 */
int can_receive(can_handle_t handle, can_message_t *msg, uint32_t timeout_ms) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || msg == NULL) {
        return -1;
    }
    
    /* 如果有挂起的消息，直接返�?*/
    if (can_dev->rx_pending) {
        memcpy(msg, &can_dev->rx_msg, sizeof(can_message_t));
        can_dev->rx_pending = false;
        return 0;
    }
    
    /* 如果没有挂起的消息，检查是否有消息可接�?*/
    if (HAL_CAN_GetRxFifoFillLevel(&can_dev->hcan, CAN_RX_FIFO0) > 0) {
        CAN_RxHeaderTypeDef rx_header;
        uint8_t data[8];
        
        if (HAL_CAN_GetRxMessage(&can_dev->hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK) {
            can_dev->stats.error_count++;
            return -1;
        }
        
        /* 填充接收消息结构 */
        msg->id = rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId;
        msg->id_type = rx_header.IDE == CAN_ID_STD ? CAN_ID_STANDARD : CAN_ID_EXTENDED;
        msg->frame_type = rx_header.RTR == CAN_RTR_DATA ? CAN_FRAME_DATA : CAN_FRAME_REMOTE;
        msg->dlc = rx_header.DLC;
        memcpy(msg->data, data, msg->dlc);
        msg->timestamp = can_dev->timestamp++;
        
        /* 更新统计信息 */
        can_dev->stats.rx_count++;
        
        return 0;
    }
    
    /* 如果没有消息，根据超时设置等�?*/
    if (timeout_ms == 0) {
        return -1; /* 无消息且不等�?*/
    }
    
    /* 等待消息 */
    uint32_t start_time = HAL_GetTick();
    while (!can_dev->rx_pending) {
        if (timeout_ms != UINT32_MAX && (HAL_GetTick() - start_time > timeout_ms)) {
            return -1; /* 超时 */
        }
        
        /* 让出CPU时间 */
        HAL_Delay(1);
    }
    
    /* 返回接收到的消息 */
    memcpy(msg, &can_dev->rx_msg, sizeof(can_message_t));
    can_dev->rx_pending = false;
    
    return 0;
}

/**
 * @brief 获取CAN统计信息
 */
int can_get_stats(can_handle_t handle, can_stats_t *stats) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || stats == NULL) {
        return -1;
    }
    
    /* 获取最新错误计数器�?*/
    uint32_t error_state = HAL_CAN_GetError(&can_dev->hcan);
    
    /* 更新统计数据 */
    can_dev->stats.tx_error_counter = (HAL_CAN_GetState(&can_dev->hcan) == HAL_CAN_STATE_ERROR) ? 255 : 0;
    can_dev->stats.rx_error_counter = (HAL_CAN_GetState(&can_dev->hcan) == HAL_CAN_STATE_ERROR) ? 255 : 0;
    can_dev->stats.bus_off = (error_state & HAL_CAN_ERROR_BOF) ? true : false;
    can_dev->stats.passive_error = (error_state & HAL_CAN_ERROR_EPV) ? true : false;
    
    /* 复制统计信息 */
    memcpy(stats, &can_dev->stats, sizeof(can_stats_t));
    
    return 0;
}

/**
 * @brief 清除CAN统计信息
 */
int can_clear_stats(can_handle_t handle) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
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
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || bus_off == NULL || passive_error == NULL) {
        return -1;
    }
    
    /* 获取错误状�?*/
    uint32_t error_state = HAL_CAN_GetError(&can_dev->hcan);
    
    *bus_off = (error_state & HAL_CAN_ERROR_BOF) ? true : false;
    *passive_error = (error_state & HAL_CAN_ERROR_EPV) ? true : false;
    
    return 0;
}

/**
 * @brief 重置CAN错误计数�?
 */
int can_reset_error_counters(can_handle_t handle) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* STM32 HAL没有直接提供重置错误计数器的函数
     * 简单的重新启动CAN控制器以重置错误计数�?
     */
    HAL_CAN_Stop(&can_dev->hcan);
    HAL_CAN_Start(&can_dev->hcan);
    
    return 0;
}

/**
 * @brief 设置CAN工作模式
 */
int can_set_mode(can_handle_t handle, can_mode_t mode) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized) {
        return -1;
    }
    
    /* 停止CAN控制�?*/
    HAL_CAN_Stop(&can_dev->hcan);
    
    /* 更新模式配置 */
    can_dev->config.mode = mode;
    
    /* 更新HAL配置 */
    can_dev->hcan.Init.Mode = mode == CAN_MODE_LOOPBACK ? CAN_MODE_LOOPBACK :
                             mode == CAN_MODE_SILENT ? CAN_MODE_SILENT :
                             mode == CAN_MODE_SILENT_LOOPBACK ? CAN_MODE_SILENT_LOOPBACK :
                             CAN_MODE_NORMAL;
    
    /* 重新初始化CAN控制�?*/
    if (HAL_CAN_Init(&can_dev->hcan) != HAL_OK) {
        return -1;
    }
    
    /* 重新启动CAN控制�?*/
    if (HAL_CAN_Start(&can_dev->hcan) != HAL_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 设置CAN接收回调函数
 */
int can_set_rx_callback(can_handle_t handle, can_callback_t callback, void *arg) {
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
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
    stm32_can_t *can_dev = (stm32_can_t *)handle;
    
    if (can_dev == NULL || !can_dev->initialized || status == NULL) {
        return -1;
    }
    
    *status = can_dev->status;
    
    return 0;
}
