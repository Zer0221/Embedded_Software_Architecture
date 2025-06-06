/**
 * @file i2c_api.h
 * @brief I2C接口抽象层定义
 *
 * 该头文件定义了I2C接口的统一抽象，使上层应用能够与底层I2C硬件实现解耦
 */

#ifndef I2C_API_H
#define I2C_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* I2C通道ID定义 */
typedef enum {
    I2C_CHANNEL_0 = 0,    /**< I2C通道0 */
    I2C_CHANNEL_1,        /**< I2C通道1 */
    I2C_CHANNEL_2,        /**< I2C通道2 */
    I2C_CHANNEL_MAX       /**< I2C通道最大数量 */
} i2c_channel_t;

/* I2C总线速率定义 */
typedef enum {
    I2C_SPEED_STANDARD = 0,  /**< 标准模式: 100kHz */
    I2C_SPEED_FAST,          /**< 快速模式: 400kHz */
    I2C_SPEED_FAST_PLUS,     /**< 快速模式+: 1MHz */
    I2C_SPEED_HIGH           /**< 高速模式: 3.4MHz */
} i2c_speed_t;

/* I2C配置参数 */
typedef struct {
    i2c_channel_t channel;   /**< I2C通道 */
    i2c_speed_t speed;       /**< I2C速率 */
    bool addr_10bit;         /**< 是否使用10位地址模式 */
} i2c_config_t;

/* I2C传输标志 */
typedef enum {
    I2C_FLAG_NONE      = 0x00,     /**< 无标志 */
    I2C_FLAG_STOP      = 0x01,     /**< 传输后发送停止位 */
    I2C_FLAG_NO_START  = 0x02,     /**< 传输不发送起始位 */
    I2C_FLAG_IGNORE_NAK = 0x04,    /**< 忽略NAK响应 */
    I2C_FLAG_10BIT_ADDR = 0x08,    /**< 使用10位地址 */
} i2c_flag_t;

/* I2C设备句柄 */
typedef driver_handle_t i2c_handle_t;

/**
 * @brief 初始化I2C总线
 * 
 * @param config I2C配置参数
 * @param handle I2C设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int i2c_init(const i2c_config_t *config, i2c_handle_t *handle);

/**
 * @brief 去初始化I2C总线
 * 
 * @param handle I2C设备句柄
 * @return int 0表示成功，非0表示失败
 */
int i2c_deinit(i2c_handle_t handle);

/**
 * @brief I2C主机发送数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错误
 */
int i2c_master_transmit(i2c_handle_t handle, uint16_t dev_addr, const uint8_t *data, 
                      uint32_t len, uint32_t flags, uint32_t timeout_ms);

/**
 * @brief I2C主机接收数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错误
 */
int i2c_master_receive(i2c_handle_t handle, uint16_t dev_addr, uint8_t *data, 
                     uint32_t len, uint32_t flags, uint32_t timeout_ms);

/**
 * @brief I2C内存写入
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小（1/2字节）
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功写入的字节数，负值表示错误
 */
int i2c_mem_write(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
                uint8_t mem_addr_size, const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief I2C内存读取
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小（1/2字节）
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功读取的字节数，负值表示错误
 */
int i2c_mem_read(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
               uint8_t mem_addr_size, uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 检测I2C设备是否存在
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param retries 重试次数
 * @param timeout_ms 超时时间（毫秒）
 * @return int 0表示设备存在，非0表示设备不存在或错误
 */
int i2c_is_device_ready(i2c_handle_t handle, uint16_t dev_addr, uint32_t retries, uint32_t timeout_ms);

#endif /* I2C_API_H */
