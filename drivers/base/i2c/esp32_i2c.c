/**
 * @file esp32_i2c.c
 * @brief ESP32平台I2C驱动实现
 *
 * 该文件实现了ESP32平台的I2C驱动接口
 */

#include "base/i2c_api.h"
#include "common/error_api.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

/* 日志标签 */
static const char *TAG = "ESP32_I2C";

/* I2C设备句柄结构�?*/
typedef struct {
    i2c_port_t port;        /* ESP32 I2C端口�?*/
    i2c_config_t config;    /* I2C配置参数 */
    bool initialized;       /* 初始化标�?*/
} esp32_i2c_handle_t;

/* I2C设备句柄数组 */
static esp32_i2c_handle_t g_i2c_handles[I2C_CHANNEL_MAX];

/**
 * @brief 将抽象速率转换为ESP32速率
 * 
 * @param speed 抽象速率
 * @return uint32_t ESP32速率
 */
static uint32_t convert_speed(i2c_speed_t speed)
{
    switch (speed) {
        case I2C_SPEED_STANDARD:
            return 100000;  /* 100kHz */
        case I2C_SPEED_FAST:
            return 400000;  /* 400kHz */
        case I2C_SPEED_FAST_PLUS:
            return 1000000; /* 1MHz */
        case I2C_SPEED_HIGH:
            return 3400000; /* 3.4MHz, 实际可能受ESP32硬件限制 */
        default:
            return 100000;  /* 默认100kHz */
    }
}

/**
 * @brief 初始化I2C总线
 * 
 * @param config I2C配置参数
 * @param handle I2C设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int i2c_init(const i2c_config_t *config, i2c_handle_t *handle)
{
    esp32_i2c_handle_t *esp32_handle;
    i2c_config_t esp32_i2c_config;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= I2C_CHANNEL_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已初始化 */
    esp32_handle = &g_i2c_handles[config->channel];
    if (esp32_handle->initialized) {
        return ERROR_BUSY;
    }
    
    /* 配置ESP32 I2C参数 */
    memset(&esp32_i2c_config, 0, sizeof(esp32_i2c_config));
    
    /* 由于ESP32需要指定GPIO引脚，这里使用默认引�?
       可以通过扩展配置结构体来支持自定义引�?*/
    if (config->channel == I2C_CHANNEL_0) {
        /* I2C_0默认引脚 */
        esp32_i2c_config.sda_io_num = 21;
        esp32_i2c_config.scl_io_num = 22;
    } else {
        /* I2C_1默认引脚 */
        esp32_i2c_config.sda_io_num = 18;
        esp32_i2c_config.scl_io_num = 19;
    }
    
    esp32_i2c_config.mode = I2C_MODE_MASTER;
    esp32_i2c_config.master.clk_speed = convert_speed(config->speed);
    esp32_i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    esp32_i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    
    /* 安装I2C驱动 */
    ret = i2c_param_config(config->channel, &esp32_i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    ret = i2c_driver_install(config->channel, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 更新句柄信息 */
    esp32_handle->port = config->channel;
    esp32_handle->initialized = true;
    memcpy(&esp32_handle->config, config, sizeof(i2c_config_t));
    
    /* 返回句柄 */
    *handle = (i2c_handle_t)esp32_handle;
    
    return ERROR_NONE;
}

/**
 * @brief 去初始化I2C总线
 * 
 * @param handle I2C设备句柄
 * @return int 0表示成功，非0表示失败
 */
int i2c_deinit(i2c_handle_t handle)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 卸载I2C驱动 */
    ret = i2c_driver_delete(esp32_handle->port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver delete failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 清除句柄信息 */
    esp32_handle->initialized = false;
    
    return ERROR_NONE;
}

/**
 * @brief I2C主机发送数�?
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错�?
 */
int i2c_master_transmit(i2c_handle_t handle, uint16_t dev_addr, const uint8_t *data, 
                      uint32_t len, uint32_t flags, uint32_t timeout_ms)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    i2c_cmd_handle_t cmd;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 创建I2C命令链接 */
    cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 如果需要，发送起始位 */
    if (!(flags & I2C_FLAG_NO_START)) {
        i2c_master_start(cmd);
        if (flags & I2C_FLAG_10BIT_ADDR) {
            /* 10位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr >> 7) | 0xF0, true);
            i2c_master_write_byte(cmd, dev_addr & 0xFF, true);
        } else {
            /* 7位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
        }
    }
    
    /* 发送数�?*/
    i2c_master_write(cmd, data, len, !(flags & I2C_FLAG_IGNORE_NAK));
    
    /* 如果需要，发送停止位 */
    if (flags & I2C_FLAG_STOP) {
        i2c_master_stop(cmd);
    }
    
    /* 执行I2C传输 */
    ret = i2c_master_cmd_begin(esp32_handle->port, cmd, timeout_ms / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    /* 检查结�?*/
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C transmit failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return len;
}

/**
 * @brief I2C主机接收数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错�?
 */
int i2c_master_receive(i2c_handle_t handle, uint16_t dev_addr, uint8_t *data, 
                     uint32_t len, uint32_t flags, uint32_t timeout_ms)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    i2c_cmd_handle_t cmd;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 创建I2C命令链接 */
    cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 如果需要，发送起始位 */
    if (!(flags & I2C_FLAG_NO_START)) {
        i2c_master_start(cmd);
        if (flags & I2C_FLAG_10BIT_ADDR) {
            /* 10位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr >> 7) | 0xF0, true);
            i2c_master_write_byte(cmd, dev_addr & 0xFF, true);
            i2c_master_start(cmd);  /* 重复起始�?*/
            i2c_master_write_byte(cmd, ((dev_addr >> 7) | 0xF0) | 0x01, true);
        } else {
            /* 7位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
        }
    }
    
    /* 接收数据 */
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    
    /* 如果需要，发送停止位 */
    if (flags & I2C_FLAG_STOP) {
        i2c_master_stop(cmd);
    }
    
    /* 执行I2C传输 */
    ret = i2c_master_cmd_begin(esp32_handle->port, cmd, timeout_ms / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    /* 检查结�?*/
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C receive failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return len;
}

/**
 * @brief I2C内存写入
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小�?/2字节�?
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功写入的字节数，负值表示错�?
 */
int i2c_mem_write(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
                uint8_t mem_addr_size, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    i2c_cmd_handle_t cmd;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0 || 
        (mem_addr_size != 1 && mem_addr_size != 2)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 创建I2C命令链接 */
    cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 发送起始位和设备地址 */
    i2c_master_start(cmd);
    
    if (esp32_handle->config.addr_10bit) {
        /* 10位地址模式 */
        i2c_master_write_byte(cmd, (dev_addr >> 7) | 0xF0, true);
        i2c_master_write_byte(cmd, dev_addr & 0xFF, true);
    } else {
        /* 7位地址模式 */
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    }
    
    /* 发送内存地址 */
    if (mem_addr_size == 2) {
        i2c_master_write_byte(cmd, (mem_addr >> 8) & 0xFF, true);
    }
    i2c_master_write_byte(cmd, mem_addr & 0xFF, true);
    
    /* 发送数�?*/
    i2c_master_write(cmd, data, len, true);
    
    /* 发送停止位 */
    i2c_master_stop(cmd);
    
    /* 执行I2C传输 */
    ret = i2c_master_cmd_begin(esp32_handle->port, cmd, timeout_ms / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    /* 检查结�?*/
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C memory write failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return len;
}

/**
 * @brief I2C内存读取
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小�?/2字节�?
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功读取的字节数，负值表示错�?
 */
int i2c_mem_read(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
               uint8_t mem_addr_size, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    i2c_cmd_handle_t cmd;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0 || 
        (mem_addr_size != 1 && mem_addr_size != 2)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 创建I2C命令链接 */
    cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 发送起始位和设备地址 */
    i2c_master_start(cmd);
    
    if (esp32_handle->config.addr_10bit) {
        /* 10位地址模式 */
        i2c_master_write_byte(cmd, (dev_addr >> 7) | 0xF0, true);
        i2c_master_write_byte(cmd, dev_addr & 0xFF, true);
    } else {
        /* 7位地址模式 */
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    }
    
    /* 发送内存地址 */
    if (mem_addr_size == 2) {
        i2c_master_write_byte(cmd, (mem_addr >> 8) & 0xFF, true);
    }
    i2c_master_write_byte(cmd, mem_addr & 0xFF, true);
    
    /* 发送重复起始位和设备地址（读模式�?*/
    i2c_master_start(cmd);
    
    if (esp32_handle->config.addr_10bit) {
        /* 10位地址模式 */
        i2c_master_write_byte(cmd, ((dev_addr >> 7) | 0xF0) | 0x01, true);
    } else {
        /* 7位地址模式 */
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    }
    
    /* 接收数据 */
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    
    /* 发送停止位 */
    i2c_master_stop(cmd);
    
    /* 执行I2C传输 */
    ret = i2c_master_cmd_begin(esp32_handle->port, cmd, timeout_ms / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    /* 检查结�?*/
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C memory read failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return len;
}

/**
 * @brief 检测I2C设备是否存在
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param retries 重试次数
 * @param timeout_ms 超时时间（毫秒）
 * @return int 0表示设备存在，非0表示设备不存在或错误
 */
int i2c_is_device_ready(i2c_handle_t handle, uint16_t dev_addr, uint32_t retries, uint32_t timeout_ms)
{
    esp32_i2c_handle_t *esp32_handle = (esp32_i2c_handle_t *)handle;
    i2c_cmd_handle_t cmd;
    esp_err_t ret;
    uint32_t retry_count = 0;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 重试循环 */
    while (retry_count <= retries) {
        /* 创建I2C命令链接 */
        cmd = i2c_cmd_link_create();
        if (cmd == NULL) {
            return ERROR_NO_MEMORY;
        }
        
        /* 发送起始位和设备地址 */
        i2c_master_start(cmd);
        
        if (esp32_handle->config.addr_10bit) {
            /* 10位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr >> 7) | 0xF0, true);
            i2c_master_write_byte(cmd, dev_addr & 0xFF, true);
        } else {
            /* 7位地址模式 */
            i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
        }
        
        /* 发送停止位 */
        i2c_master_stop(cmd);
        
        /* 执行I2C传输 */
        ret = i2c_master_cmd_begin(esp32_handle->port, cmd, timeout_ms / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        
        /* 如果成功，设备存�?*/
        if (ret == ESP_OK) {
            return ERROR_NONE;
        }
        
        retry_count++;
    }
    
    /* 所有重试都失败，设备不存在 */
    return ERROR_TIMEOUT;
}

