/**
 * @file esp32_sdio.c
 * @brief ESP32平台SDIO驱动实现
 *
 * 该文件实现了ESP32平台的SDIO驱动接口
 */

#include "base/sdio_api.h"
#include "common/error_api.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <string.h>

/* ESP32 SDIO超时时间 (ms) */
#define SDIO_TIMEOUT          5000

/* ESP32 SDIO使用SPI模式还是SDMMC模式 */
#define SDIO_USE_SPI_MODE     0

/* ESP32 SDIO设备结构�?*/
typedef struct {
    sdmmc_card_t *card;              /* SDMMC�?*/
    sdio_config_t config;            /* SDIO配置 */
    sdio_card_info_t card_info;      /* 卡信�?*/
    sdio_callback_t callback;        /* 回调函数 */
    void *arg;                       /* 回调参数 */
    volatile sdio_status_t status;   /* 操作状�?*/
    bool initialized;                /* 初始化标�?*/
} esp32_sdio_device_t;

/* SDIO设备 */
static esp32_sdio_device_t g_sdio_device;

/* 转换总线宽度 */
static uint32_t convert_bus_width(sdio_bus_width_t bus_width)
{
    switch (bus_width) {
        case SDIO_BUS_WIDTH_1BIT:
            return 1;
        case SDIO_BUS_WIDTH_4BIT:
            return 4;
        case SDIO_BUS_WIDTH_8BIT:
            return 8;
        default:
            return 1;
    }
}

/* 转换卡类�?*/
static sdio_card_type_t convert_card_type(sdmmc_card_t *card)
{
    if (card->is_mmc) {
        return SDIO_CARD_TYPE_MMC;
    } else if (card->is_sdio) {
        return SDIO_CARD_TYPE_SD;
    } else if (card->is_mem) {
        if (card->ocr & SD_OCR_SDHC_CAP) {
            return SDIO_CARD_TYPE_SDHC;
        } else {
            return SDIO_CARD_TYPE_SD;
        }
    } else {
        return SDIO_CARD_TYPE_UNKNOWN;
    }
}

/* 更新卡信�?*/
static void update_card_info(esp32_sdio_device_t *dev)
{
    sdmmc_card_t *card = dev->card;
    
    if (card == NULL) {
        return;
    }
    
    /* 更新卡信�?*/
    dev->card_info.card_type = convert_card_type(card);
    dev->card_info.block_size = card->csd.sector_size;
    dev->card_info.block_count = card->csd.capacity;
    dev->card_info.card_capacity = card->csd.capacity * card->csd.sector_size;
    
    /* 设置卡名�?*/
    if (card->is_mmc) {
        snprintf(dev->card_info.card_name, sizeof(dev->card_info.card_name), "MMC");
    } else if (card->is_sdio) {
        snprintf(dev->card_info.card_name, sizeof(dev->card_info.card_name), "SDIO");
    } else {
        snprintf(dev->card_info.card_name, sizeof(dev->card_info.card_name), "SD%s", 
                 (card->ocr & SD_OCR_SDHC_CAP) ? "HC" : "");
    }
    
    /* 设置制造商ID */
    snprintf(dev->card_info.manufacturer_id, sizeof(dev->card_info.manufacturer_id), "0x%02X", card->cid.mfg_id);
    
    /* 设置产品名称 */
    memcpy(dev->card_info.product_name, card->cid.name, sizeof(card->cid.name));
    dev->card_info.product_name[sizeof(card->cid.name)] = '\0';
    
    /* 设置序列�?*/
    snprintf(dev->card_info.serial_number, sizeof(dev->card_info.serial_number), "%u", card->cid.serial);
    
    /* 设置生产日期 */
    dev->card_info.manufacturing_date[0] = card->cid.date.month;
    dev->card_info.manufacturing_date[1] = card->cid.date.year;
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
    esp32_sdio_device_t *dev;
    esp_err_t err;
    sdmmc_host_t host;
    sdmmc_slot_config_t slot_config;
    
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
    memset(dev, 0, sizeof(esp32_sdio_device_t));
    dev->callback = callback;
    dev->arg = arg;
    dev->status = SDIO_STATUS_IDLE;
    
    /* 复制配置 */
    memcpy(&dev->config, config, sizeof(sdio_config_t));
    
#if SDIO_USE_SPI_MODE
    /* 使用SPI模式 */
    host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t spi_slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    err = sdspi_host_init();
    if (err != ESP_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    err = sdspi_host_init_slot(host.slot, &spi_slot_config);
    if (err != ESP_OK) {
        sdspi_host_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
#else
    /* 使用SDMMC模式 */
    host = SDMMC_HOST_DEFAULT();
    
    /* 配置SDMMC总线宽度 */
    if (config->enable_4bit) {
        host.flags |= SDMMC_HOST_FLAG_4BIT;
    } else {
        host.flags &= ~SDMMC_HOST_FLAG_4BIT;
    }
    
    /* 配置高速模�?*/
    if (config->enable_high_speed) {
        host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    } else {
        host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    }
    
    /* 配置DMA模式 */
    if (config->enable_dma) {
        host.flags |= SDMMC_HOST_FLAG_DMA_ENABLED;
    } else {
        host.flags &= ~SDMMC_HOST_FLAG_DMA_ENABLED;
    }
    
    /* 初始化SDMMC主机 */
    err = sdmmc_host_init();
    if (err != ESP_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 配置SDMMC槽位 */
    slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    /* 初始化SDMMC槽位 */
    err = sdmmc_host_init_slot(host.slot, &slot_config);
    if (err != ESP_OK) {
        sdmmc_host_deinit();
        return ERROR_DRIVER_INIT_FAILED;
    }
#endif
    
    /* 探测并初始化SD�?*/
    dev->card = malloc(sizeof(sdmmc_card_t));
    if (dev->card == NULL) {
#if SDIO_USE_SPI_MODE
        sdspi_host_deinit();
#else
        sdmmc_host_deinit();
#endif
        return ERROR_OUT_OF_MEMORY;
    }
    
    err = sdmmc_card_init(&host, dev->card);
    if (err != ESP_OK) {
        free(dev->card);
        dev->card = NULL;
#if SDIO_USE_SPI_MODE
        sdspi_host_deinit();
#else
        sdmmc_host_deinit();
#endif
        return ERROR_DRIVER_INIT_FAILED;
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 释放卡资�?*/
    if (dev->card != NULL) {
        free(dev->card);
        dev->card = NULL;
    }
    
#if SDIO_USE_SPI_MODE
    /* 去初始化SPI主机 */
    sdspi_host_deinit();
#else
    /* 去初始化SDMMC主机 */
    sdmmc_host_deinit();
#endif
    
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || card_inserted == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    *card_inserted = (dev->card != NULL);
    
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL || block_count == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    if (dev->card == NULL) {
        dev->status = SDIO_STATUS_NO_CARD;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_NO_CARD);
        }
        
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 读取数据 */
    err = sdmmc_read_sectors(dev->card, data, block_addr, block_count);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = SDIO_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_READ_FAILED;
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL || block_count == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    if (dev->card == NULL) {
        dev->status = SDIO_STATUS_NO_CARD;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_NO_CARD);
        }
        
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 写入数据 */
    err = sdmmc_write_sectors(dev->card, data, block_addr, block_count);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = SDIO_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_WRITE_FAILED;
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || start_block > end_block) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    if (dev->card == NULL) {
        dev->status = SDIO_STATUS_NO_CARD;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, SDIO_STATUS_NO_CARD);
        }
        
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 更新状�?*/
    dev->status = SDIO_STATUS_BUSY;
    
    /* 擦除�?*/
    err = sdmmc_erase_sectors(dev->card, start_block, end_block - start_block + 1);
    
    if (err != ESP_OK) {
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    
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
    /* ESP32 SDMMC API不支持运行时更改总线宽度，返回不支持 */
    return ERROR_NOT_SUPPORTED;
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
    /* ESP32 SDMMC API不支持运行时更改频率模式，返回不支持 */
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
    /* ESP32 SDMMC API不支持写保护检测，返回不支�?*/
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
    /* ESP32 SDMMC API不支持获取写保护状态，返回不支�?*/
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || block_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    if (dev->card == NULL) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 获取块大�?*/
    *block_size = dev->card->csd.sector_size;
    
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
    /* ESP32 SDMMC API不支持设置块大小，返回不支持 */
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
    esp32_sdio_device_t *dev = (esp32_sdio_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || resp == NULL || resp_len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查卡是否存在 */
    if (dev->card == NULL) {
        return ERROR_DEVICE_NOT_READY;
    }
    
    /* 执行命令 */
    err = sdmmc_send_cmd(dev->card, cmd, arg, resp);
    
    if (err != ESP_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    return DRIVER_OK;
}

