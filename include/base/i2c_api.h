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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @return api_status_t 操作状态
 */
api_status_t i2c_init(const i2c_config_t *config, i2c_handle_t *handle);

/**
 * @brief 反初始化I2C总线
 * 
 * @param handle I2C设备句柄
 * @return api_status_t 操作状态
 */
api_status_t i2c_deinit(i2c_handle_t handle);

/**
 * @brief 从I2C设备读取数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_read(i2c_handle_t handle, uint16_t dev_addr, uint8_t *data, uint16_t len, uint8_t flags, uint32_t timeout_ms);

/**
 * @brief 向I2C设备写入数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲区
 * @param len 写入长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_write(i2c_handle_t handle, uint16_t dev_addr, const uint8_t *data, uint16_t len, uint8_t flags, uint32_t timeout_ms);

/**
 * @brief 从I2C设备寄存器读取数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param reg_size 寄存器地址大小(字节)
 * @param data 数据缓冲区
 * @param len 读取长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_read_reg(i2c_handle_t handle, uint16_t dev_addr, uint16_t reg_addr, uint8_t reg_size, uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 向I2C设备寄存器写入数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param reg_size 寄存器地址大小(字节)
 * @param data 数据缓冲区
 * @param len 写入长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_write_reg(i2c_handle_t handle, uint16_t dev_addr, uint16_t reg_addr, uint8_t reg_size, const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 执行I2C传输
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param tx_data 发送数据缓冲区
 * @param tx_len 发送数据长度
 * @param rx_data 接收数据缓冲区
 * @param rx_len 接收数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_transfer(i2c_handle_t handle, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len, uint32_t timeout_ms);

/**
 * @brief 检测I2C设备是否存在
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t i2c_is_device_ready(i2c_handle_t handle, uint16_t dev_addr, uint32_t timeout_ms);

/**
 * @brief 扫描I2C总线上的设备
 * 
 * @param handle I2C设备句柄
 * @param addr_list 地址列表缓冲区
 * @param list_size 列表大小
 * @param found_count 找到的设备数量
 * @return api_status_t 操作状态
 */
api_status_t i2c_scan_devices(i2c_handle_t handle, uint16_t *addr_list, uint8_t list_size, uint8_t *found_count);

#ifdef __cplusplus
}
#endif

#endif /* I2C_API_H */