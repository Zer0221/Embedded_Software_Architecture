/**
 * @file esp32_spi.c
 * @brief ESP32平台SPI驱动实现
 *
 * 该文件实现了ESP32平台的SPI驱动接口
 */

#include "base/spi_api.h"
#include "common/error_api.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

/* 日志标签 */
static const char *TAG = "ESP32_SPI";

/* SPI设备句柄结构�?*/
typedef struct {
    spi_host_device_t host;       /* ESP32 SPI主机设备 */
    spi_device_handle_t device;   /* ESP32 SPI设备句柄 */
    spi_config_t config;          /* SPI配置参数 */
    bool initialized;             /* 初始化标�?*/
    uint8_t cs_pin;               /* 软件片选引�?*/
} esp32_spi_handle_t;

/* SPI设备句柄数组 */
static esp32_spi_handle_t g_spi_handles[SPI_CHANNEL_MAX];

/**
 * @brief 将抽象通道转换为ESP32 SPI主机设备
 * 
 * @param channel 抽象通道
 * @return spi_host_device_t ESP32 SPI主机设备
 */
static spi_host_device_t convert_channel(spi_channel_t channel)
{
    switch (channel) {
        case SPI_CHANNEL_0:
            return SPI2_HOST;  /* ESP32 HSPI */
        case SPI_CHANNEL_1:
            return SPI3_HOST;  /* ESP32 VSPI */
        default:
            return SPI2_HOST;
    }
}

/**
 * @brief 将抽象模式转换为ESP32模式
 * 
 * @param mode 抽象模式
 * @return uint8_t ESP32模式
 */
static uint8_t convert_mode(spi_mode_t mode)
{
    switch (mode) {
        case SPI_MODE_0:
            return 0;
        case SPI_MODE_1:
            return 1;
        case SPI_MODE_2:
            return 2;
        case SPI_MODE_3:
            return 3;
        default:
            return 0;
    }
}

/**
 * @brief 初始化SPI
 * 
 * @param config SPI配置参数
 * @param handle SPI设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int spi_init(const spi_config_t *config, spi_handle_t *handle)
{
    esp32_spi_handle_t *esp32_handle;
    spi_bus_config_t bus_config;
    spi_device_interface_config_t dev_config;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= SPI_CHANNEL_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已初始化 */
    esp32_handle = &g_spi_handles[config->channel];
    if (esp32_handle->initialized) {
        return ERROR_BUSY;
    }
    
    /* 配置ESP32 SPI总线参数 */
    memset(&bus_config, 0, sizeof(bus_config));
    
    /* 根据不同SPI通道设置默认引脚 */
    if (config->channel == SPI_CHANNEL_0) {
        /* SPI2_HOST (HSPI) 默认引脚 */
        bus_config.mosi_io_num = 13;
        bus_config.miso_io_num = 12;
        bus_config.sclk_io_num = 14;
        bus_config.quadwp_io_num = -1;
        bus_config.quadhd_io_num = -1;
    } else {
        /* SPI3_HOST (VSPI) 默认引脚 */
        bus_config.mosi_io_num = 23;
        bus_config.miso_io_num = 19;
        bus_config.sclk_io_num = 18;
        bus_config.quadwp_io_num = -1;
        bus_config.quadhd_io_num = -1;
    }
    
    bus_config.max_transfer_sz = 4096;
    
    /* 初始化SPI总线 */
    ret = spi_bus_initialize(convert_channel(config->channel), &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialize failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 配置ESP32 SPI设备参数 */
    memset(&dev_config, 0, sizeof(dev_config));
    
    dev_config.mode = convert_mode(config->mode);
    dev_config.clock_speed_hz = config->clock_hz;
    
    /* 设置数据位序 */
    if (config->bit_order == SPI_BIT_ORDER_LSB_FIRST) {
        dev_config.flags |= SPI_DEVICE_BIT_LSBFIRST;
    }
    
    /* 根据数据宽度设置参数 */
    switch (config->data_width) {
        case SPI_DATA_WIDTH_8BIT:
            dev_config.command_bits = 0;
            dev_config.address_bits = 0;
            dev_config.dummy_bits = 0;
            break;
        case SPI_DATA_WIDTH_16BIT:
            /* ESP32不直接支�?6位传输，通过软件处理 */
            dev_config.command_bits = 0;
            dev_config.address_bits = 0;
            dev_config.dummy_bits = 0;
            break;
        case SPI_DATA_WIDTH_32BIT:
            /* ESP32不直接支�?2位传输，通过软件处理 */
            dev_config.command_bits = 0;
            dev_config.address_bits = 0;
            dev_config.dummy_bits = 0;
            break;
        default:
            spi_bus_free(convert_channel(config->channel));
            return ERROR_INVALID_PARAM;
    }
    
    /* 设置片选模�?*/
    if (config->cs_mode == SPI_CS_MODE_HARDWARE) {
        /* 硬件片选模式，默认引脚 */
        dev_config.spics_io_num = (config->channel == SPI_CHANNEL_0) ? 15 : 5;
        dev_config.cs_ena_pretrans = 0;
        dev_config.cs_ena_posttrans = 0;
    } else {
        /* 软件片选模�?*/
        dev_config.spics_io_num = -1;
        
        /* 配置软件片选引�?*/
        if (config->cs_pin < 255) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << config->cs_pin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            ret = gpio_config(&io_conf);
            if (ret != ESP_OK) {
                spi_bus_free(convert_channel(config->channel));
                ESP_LOGE(TAG, "GPIO config failed: %d", ret);
                return ERROR_HARDWARE;
            }
            
            /* 默认保持片选为非激活状�?*/
            gpio_set_level(config->cs_pin, 1);
        }
    }
    
    /* 预设传输队列长度�? */
    dev_config.queue_size = 4;
    
    /* 添加SPI设备 */
    ret = spi_bus_add_device(convert_channel(config->channel), &dev_config, &esp32_handle->device);
    if (ret != ESP_OK) {
        spi_bus_free(convert_channel(config->channel));
        ESP_LOGE(TAG, "SPI bus add device failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 更新句柄信息 */
    esp32_handle->host = convert_channel(config->channel);
    esp32_handle->initialized = true;
    esp32_handle->cs_pin = config->cs_pin;
    memcpy(&esp32_handle->config, config, sizeof(spi_config_t));
    
    /* 返回句柄 */
    *handle = (spi_handle_t)esp32_handle;
    
    return ERROR_NONE;
}

/**
 * @brief 去初始化SPI
 * 
 * @param handle SPI设备句柄
 * @return int 0表示成功，非0表示失败
 */
int spi_deinit(spi_handle_t handle)
{
    esp32_spi_handle_t *esp32_handle = (esp32_spi_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 移除SPI设备 */
    ret = spi_bus_remove_device(esp32_handle->device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus remove device failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 释放SPI总线 */
    ret = spi_bus_free(esp32_handle->host);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus free failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 清除句柄信息 */
    esp32_handle->initialized = false;
    
    return ERROR_NONE;
}

/**
 * @brief SPI传输数据（全双工�?
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲�?
 * @param len 数据长度（字节数�?
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错�?
 */
int spi_transfer(spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, 
                uint32_t len, uint32_t timeout_ms)
{
    esp32_spi_handle_t *esp32_handle = (esp32_spi_handle_t *)handle;
    spi_transaction_t transaction;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL || len == 0 || (tx_data == NULL && rx_data == NULL)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 配置SPI传输参数 */
    memset(&transaction, 0, sizeof(transaction));
    
    /* 设置数据缓冲�?*/
    transaction.length = len * 8; /* 位数 */
    transaction.tx_buffer = tx_data;
    transaction.rx_buffer = rx_data;
    
    /* 如果是软件片选模式，需要手动控制片选信�?*/
    if (esp32_handle->config.cs_mode == SPI_CS_MODE_SOFTWARE && esp32_handle->cs_pin < 255) {
        /* 激活片�?*/
        gpio_set_level(esp32_handle->cs_pin, 0);
    }
    
    /* 进行SPI传输 */
    ret = spi_device_transmit(esp32_handle->device, &transaction);
    
    /* 如果是软件片选模式，需要手动控制片选信�?*/
    if (esp32_handle->config.cs_mode == SPI_CS_MODE_SOFTWARE && esp32_handle->cs_pin < 255) {
        /* 取消激活片�?*/
        gpio_set_level(esp32_handle->cs_pin, 1);
    }
    
    /* 检查传输结�?*/
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return len;
}

/**
 * @brief SPI只发送数�?
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param len 数据长度（字节数�?
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功发送的字节数，负值表示错�?
 */
int spi_transmit(spi_handle_t handle, const uint8_t *tx_data, uint32_t len, uint32_t timeout_ms)
{
    /* 调用全双工传输，但不接收数据 */
    return spi_transfer(handle, tx_data, NULL, len, timeout_ms);
}

/**
 * @brief SPI只接收数�?
 * 
 * @param handle SPI设备句柄
 * @param rx_data 接收数据缓冲�?
 * @param len 数据长度（字节数�?
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错�?
 */
int spi_receive(spi_handle_t handle, uint8_t *rx_data, uint32_t len, uint32_t timeout_ms)
{
    /* 发送空数据以产生时钟，同时接收数据 */
    uint8_t *tx_dummy = NULL;
    esp32_spi_handle_t *esp32_handle = (esp32_spi_handle_t *)handle;
    int ret;
    
    /* 参数检�?*/
    if (handle == NULL || rx_data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 为发送分配空缓冲�?*/
    tx_dummy = (uint8_t *)malloc(len);
    if (tx_dummy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for dummy TX buffer");
        return ERROR_NO_MEMORY;
    }
    
    /* 填充0xFF，通常SPI总线空闲状�?*/
    memset(tx_dummy, 0xFF, len);
    
    /* 调用全双工传�?*/
    ret = spi_transfer(handle, tx_dummy, rx_data, len, timeout_ms);
    
    /* 释放临时缓冲�?*/
    free(tx_dummy);
    
    return ret;
}

/**
 * @brief 软件控制SPI片选信�?
 * 
 * @param handle SPI设备句柄
 * @param state 片选状态，0表示选中�?表示取消选中
 * @return int 0表示成功，非0表示失败
 */
int spi_cs_control(spi_handle_t handle, uint8_t state)
{
    esp32_spi_handle_t *esp32_handle = (esp32_spi_handle_t *)handle;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否为软件片选模�?*/
    if (esp32_handle->config.cs_mode != SPI_CS_MODE_SOFTWARE) {
        ESP_LOGW(TAG, "CS control is only available in software CS mode");
        return ERROR_NOT_SUPPORTED;
    }
    
    /* 检查片选引脚是否有�?*/
    if (esp32_handle->cs_pin >= 255) {
        ESP_LOGW(TAG, "Invalid CS pin");
        return ERROR_INVALID_PARAM;
    }
    
    /* 控制片选引�?*/
    gpio_set_level(esp32_handle->cs_pin, state ? 1 : 0);
    
    return ERROR_NONE;
}

