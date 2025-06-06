/**
 * @file stm32_sdio.c
 * @brief STM32平台SDIO驱动实现
 *
 * 该文件实现了STM32平台的SDIO驱动接口
 */

#include "base/sdio_api.h"
#include "common/error_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* STM32 SDIO超时时间 (ms) */
#define SDIO_TIMEOUT          5000

/* STM32 SDIO设备结构�?*/
typedef struct {
    SD_HandleTypeDef hsd;           /* HAL SD句柄 */
    sdio_config_t config;           /* SDIO配置 */
    sdio_card_info_t card_info;     /* 卡信�?*/
    sdio_callback_t callback;       /* 回调函数 */
    void *arg;                      /* 回调参数 */
    volatile sdio_status_t status;  /* 操作状�?*/
    bool initialized;               /* 初始化标�?*/
} stm32_sdio_device_t;

/* SDIO设备 */
static stm32_sdio_device_t g_sdio_device;

/* 转换总线宽度 */
static uint32_t convert_bus_width(sdio_bus_width_t bus_width)
{
    switch (bus_width) {
        case SDIO_BUS_WIDTH_1BIT:
            return SDIO_BUS_WIDE_1B;
        case SDIO_BUS_WIDTH_4BIT:
            return SDIO_BUS_WIDE_4B;
        case SDIO_BUS_WIDTH_8BIT:
            return SDIO_BUS_WIDE_8B;
        default:
            return SDIO_BUS_WIDE_1B;
    }
}

/* 转换卡类�?*/
static sdio_card_type_t convert_card_type(uint32_t hal_card_type)
{
    if (hal_card_type == CARD_SDSC) {
        return SDIO_CARD_TYPE_SD;
    } else if (hal_card_type == CARD_SDHC_SDXC) {
        return SDIO_CARD_TYPE_SDHC;
    } else if (hal_card_type == CARD_SECURED) {
        return SDIO_CARD_TYPE_SD;
    } else {
        return SDIO_CARD_TYPE_UNKNOWN;
    }
}

/* 更新卡信�?*/
static void update_card_info(stm32_sdio_device_t *dev)
{
    HAL_SD_CardInfoTypeDef hal_card_info;
    
    /* 获取HAL卡信�?*/
    if (HAL_SD_GetCardInfo(&dev->hsd, &hal_card_info) != HAL_OK) {
        return;
    }
    
    /* 更新卡信�?*/
    dev->card_info.card_type = convert_card_type(hal_card_info.CardType);
    dev->card_info.block_size = hal_card_info.BlockSize;
    dev->card_info.block_count = hal_card_info.BlockNbr;
    dev->card_info.card_capacity = hal_card_info.BlockNbr * hal_card_info.BlockSize;
    
    /* 设置卡名�?*/
    snprintf(dev->card_info.card_name, sizeof(dev->card_info.card_name), "SD%lu", hal_card_info.CardType);
    
    /* 其他信息需要通过CMD10等命令获取，这里简化处�?*/
    snprintf(dev->card_info.manufacturer_id, sizeof(dev->card_info.manufacturer_id), "0x%02X", (uint8_t)(hal_card_info.CardCID[0] >> 24));
    memset(dev->card_info.product_name, 0, sizeof(dev->card_info.product_name));
    memset(dev->card_info.serial_number, 0, sizeof(dev->card_info.serial_number));
    dev->card_info.manufacturing_date[0] = 0;
    dev->card_info.manufacturing_date[1] = 0;
}

/**
 * @brief 初始化SDIO设备
 * 
 * @param config SDIO配置参数
 * @param callback SDIO操作完成回调函数
 * @param arg 传递给回调函数的参�?
 * @param handle SDIO设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_init(const sdio_config_t *config, sdio_callback_t callback, void *arg, sdio_handle_t *handle)
{
    stm32_sdio_device_t *dev;
    SD_HandleTypeDef *hsd;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取SDIO设备 */
    dev = &g_sdio_device;
    
    /* 检查是否已初始�?*/
    if (dev->initialized) {
        return ERROR_ALREADY_INITIALIZED;
    }
    
    /* 初始化SDIO设备 */
    memset(dev, 0, sizeof(stm32_sdio_device_t));
    dev->callback = callback;
    dev->arg = arg;
    dev->status = SDIO_STATUS_IDLE;
    
    /* 复制配置 */
    memcpy(&dev->config, config, sizeof(sdio_config_t));
    
    /* 获取HAL SD句柄 */
    hsd = &dev->hsd;
    
    /* 初始化SD配置 */
    hsd->Instance = SDIO;
    hsd->Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd->Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd->Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd->Init.BusWide = convert_bus_width(config->bus_width);
    hsd->Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd->Init.ClockDiv = 0; /* 根据频率模式在后续设�?*/
    
    /* 启用SDIO时钟 */
    __HAL_RCC_SDIO_CLK_ENABLE();
    
    /* 初始化HAL SD */
    if (HAL_SD_Init(hsd) != HAL_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 设置总线宽度 */
    if (config->enable_4bit) {
        if (HAL_SD_ConfigWideBusOperation(hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
            HAL_SD_DeInit(hsd);
            return ERROR_DRIVER_INIT_FAILED;
        }
    }
    
    /* 更新卡信�?*/
    update_card_info(dev);
    
    /* 设置初始化标�?*/
    dev->initialized = true;
    
    /* 返回句柄 */
    *handle = (sdio_handle_t)dev;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化SDIO设备
 * 
 * @param handle SDIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int sdio_deinit(sdio_handle_t handle)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 去初始化HAL SD */
    if (HAL_SD_DeInit(&dev->hsd) != HAL_OK) {
        return ERROR_DRIVER_DEINIT_FAILED;
    }
    
    /* 清除初始化标�?*/
    dev->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 检测SD�?
 * 
 * @param handle SDIO设备句柄
 * @param card_inserted 卡插入状态指�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_detect_card(sdio_handle_t handle, bool *card_inserted)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || card_inserted == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* STM32 HAL没有专门的检测函数，通过初始化状态判�?*/
    *card_inserted = (dev->hsd.State != HAL_SD_STATE_RESET);
    
    return DRIVER_OK;
}

/**
 * @brief 获取SD卡信�?
 * 
 * @param handle SDIO设备句柄
 * @param card_info 卡信息指�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_card_info(sdio_handle_t handle, sdio_card_info_t *card_info)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || card_info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新卡信�?*/
    update_card_info(dev);
    
    /* 复制卡信�?*/
    memcpy(card_info, &dev->card_info, sizeof(sdio_card_info_t));
    
    return DRIVER_OK;
}

/**
 * @brief 读取SD卡数�?
 * 
 * @param handle SDIO设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲�?
 * @param block_count 块数�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_read_blocks(sdio_handle_t handle, uint32_t block_addr, void *data, uint32_t block_count)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL || block_count == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 读取数据 */
    hal_status = HAL_SD_ReadBlocks(&dev->hsd, data, block_addr, block_count, SDIO_TIMEOUT);
    
    if (hal_status != HAL_OK) {
        /* 更新状�?*/
        dev->status = SDIO_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_READ_FAILED;
    }
    
    /* 等待传输完成 */
    hal_status = HAL_SD_GetCardState(&dev->hsd);
    while (hal_status == HAL_SD_CARD_TRANSFER) {
        hal_status = HAL_SD_GetCardState(&dev->hsd);
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, SDIO_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 写入SD卡数�?
 * 
 * @param handle SDIO设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲�?
 * @param block_count 块数�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_write_blocks(sdio_handle_t handle, uint32_t block_addr, const void *data, uint32_t block_count)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL || block_count == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 写入数据 */
    hal_status = HAL_SD_WriteBlocks(&dev->hsd, (uint8_t *)data, block_addr, block_count, SDIO_TIMEOUT);
    
    if (hal_status != HAL_OK) {
        /* 更新状�?*/
        dev->status = SDIO_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_WRITE_FAILED;
    }
    
    /* 等待传输完成 */
    hal_status = HAL_SD_GetCardState(&dev->hsd);
    while (hal_status == HAL_SD_CARD_TRANSFER) {
        hal_status = HAL_SD_GetCardState(&dev->hsd);
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, SDIO_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 擦除SD卡数�?
 * 
 * @param handle SDIO设备句柄
 * @param start_block 起始块地址
 * @param end_block 结束块地址
 * @return int 0表示成功，非0表示失败
 */
int sdio_erase_blocks(sdio_handle_t handle, uint32_t start_block, uint32_t end_block)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || start_block > end_block) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 擦除�?*/
    hal_status = HAL_SD_Erase(&dev->hsd, start_block, end_block);
    
    if (hal_status != HAL_OK) {
        /* 更新状�?*/
        dev->status = SDIO_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_ERASE_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, SDIO_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取SDIO操作状�?
 * 
 * @param handle SDIO设备句柄
 * @param status 状态指�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_status(sdio_handle_t handle, sdio_status_t *status)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || status == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取状�?*/
    *status = dev->status;
    
    return DRIVER_OK;
}

/**
 * @brief 设置SDIO总线宽度
 * 
 * @param handle SDIO设备句柄
 * @param bus_width 总线宽度
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_bus_width(sdio_handle_t handle, sdio_bus_width_t bus_width)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 设置总线宽度 */
    hal_status = HAL_SD_ConfigWideBusOperation(&dev->hsd, convert_bus_width(bus_width));
    
    if (hal_status != HAL_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 更新配置 */
    dev->config.bus_width = bus_width;
    
    return DRIVER_OK;
}

/**
 * @brief 设置SDIO频率模式
 * 
 * @param handle SDIO设备句柄
 * @param freq_mode 频率模式
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_freq_mode(sdio_handle_t handle, sdio_freq_mode_t freq_mode)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* STM32 HAL没有直接设置频率模式的API，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 使能SDIO写保护检�?
 * 
 * @param handle SDIO设备句柄
 * @param enable 是否启用
 * @return int 0表示成功，非0表示失败
 */
int sdio_enable_write_protect(sdio_handle_t handle, bool enable)
{
    /* STM32 HAL没有写保护检测API，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 获取SDIO写保护状�?
 * 
 * @param handle SDIO设备句柄
 * @param is_protected 保护状态指�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_write_protect(sdio_handle_t handle, bool *is_protected)
{
    /* STM32 HAL没有获取写保护状态API，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 获取SDIO块大�?
 * 
 * @param handle SDIO设备句柄
 * @param block_size 块大小指�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_block_size(sdio_handle_t handle, uint32_t *block_size)
{
    stm32_sdio_device_t *dev = (stm32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || block_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新卡信�?*/
    update_card_info(dev);
    
    /* 获取块大�?*/
    *block_size = dev->card_info.block_size;
    
    return DRIVER_OK;
}

/**
 * @brief 设置SDIO块大�?
 * 
 * @param handle SDIO设备句柄
 * @param block_size 块大�?
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_block_size(sdio_handle_t handle, uint32_t block_size)
{
    /* STM32 HAL没有设置块大小API，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 执行SDIO命令
 * 
 * @param handle SDIO设备句柄
 * @param cmd 命令
 * @param arg 参数
 * @param resp 响应缓冲�?
 * @param resp_len 响应长度
 * @return int 0表示成功，非0表示失败
 */
int sdio_execute_command(sdio_handle_t handle, uint8_t cmd, uint32_t arg, uint32_t *resp, uint32_t resp_len)
{
    /* STM32 HAL没有直接执行命令API，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

