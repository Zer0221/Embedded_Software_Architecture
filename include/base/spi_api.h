/**
 * @file spi_api.h
 * @brief SPI接口抽象层定义
 *
 * 该头文件定义了SPI接口的统一抽象，使上层应用能够与底层SPI硬件实现解耦
 */

#ifndef SPI_API_H
#define SPI_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPI通道ID定义 */
typedef enum {
    SPI_CHANNEL_0 = 0,    /**< SPI通道0 */
    SPI_CHANNEL_1,        /**< SPI通道1 */
    SPI_CHANNEL_2,        /**< SPI通道2 */
    SPI_CHANNEL_MAX       /**< SPI通道最大数量 */
} spi_channel_t;

/* SPI模式定义 */
typedef enum {
    SPI_MODE_0 = 0,    /**< CPOL=0, CPHA=0 */
    SPI_MODE_1,        /**< CPOL=0, CPHA=1 */
    SPI_MODE_2,        /**< CPOL=1, CPHA=0 */
    SPI_MODE_3         /**< CPOL=1, CPHA=1 */
} spi_mode_t;

/* SPI位序定义 */
typedef enum {
    SPI_BIT_ORDER_MSB_FIRST = 0,  /**< 高位在前 */
    SPI_BIT_ORDER_LSB_FIRST       /**< 低位在前 */
} spi_bit_order_t;

/* SPI数据宽度定义 */
typedef enum {
    SPI_DATA_WIDTH_8BIT = 0,   /**< 8位数据宽度 */
    SPI_DATA_WIDTH_16BIT,      /**< 16位数据宽度 */
    SPI_DATA_WIDTH_32BIT       /**< 32位数据宽度 */
} spi_data_width_t;

/* SPI片选模式定义 */
typedef enum {
    SPI_CS_MODE_HARDWARE = 0,  /**< 硬件片选 */
    SPI_CS_MODE_SOFTWARE       /**< 软件片选 */
} spi_cs_mode_t;

/* SPI配置参数 */
typedef struct {
    spi_channel_t channel;         /**< SPI通道 */
    spi_mode_t mode;               /**< SPI模式 */
    spi_bit_order_t bit_order;     /**< 位序 */
    spi_data_width_t data_width;   /**< 数据宽度 */
    spi_cs_mode_t cs_mode;         /**< 片选模式 */
    uint32_t clock_hz;             /**< 时钟频率(Hz) */
    uint8_t cs_pin;                /**< 片选引脚(软件片选时使用) */
} spi_config_t;

/* SPI设备句柄 */
typedef driver_handle_t spi_handle_t;

/**
 * @brief 初始化SPI总线
 * 
 * @param config SPI配置参数
 * @param handle SPI设备句柄指针
 * @return api_status_t 操作状态
 */
api_status_t spi_init(const spi_config_t *config, spi_handle_t *handle);

/**
 * @brief 反初始化SPI总线
 * 
 * @param handle SPI设备句柄
 * @return api_status_t 操作状态
 */
api_status_t spi_deinit(spi_handle_t handle);

/**
 * @brief 读取SPI数据
 * 
 * @param handle SPI设备句柄
 * @param data 数据缓冲区
 * @param len 读取长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t spi_read(spi_handle_t handle, void *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 写入SPI数据
 * 
 * @param handle SPI设备句柄
 * @param data 数据缓冲区
 * @param len 写入长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t spi_write(spi_handle_t handle, const void *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 传输SPI数据(全双工)
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲区
 * @param len 传输长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t spi_transfer(spi_handle_t handle, const void *tx_data, void *rx_data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 设置SPI片选状态
 * 
 * @param handle SPI设备句柄
 * @param state 片选状态(true: 使能, false: 禁用)
 * @return api_status_t 操作状态
 */
api_status_t spi_cs_set(spi_handle_t handle, bool state);

/**
 * @brief 设置SPI时钟频率
 * 
 * @param handle SPI设备句柄
 * @param clock_hz 时钟频率(Hz)
 * @return api_status_t 操作状态
 */
api_status_t spi_set_clock(spi_handle_t handle, uint32_t clock_hz);

/**
 * @brief 设置SPI模式
 * 
 * @param handle SPI设备句柄
 * @param mode SPI模式
 * @return api_status_t 操作状态
 */
api_status_t spi_set_mode(spi_handle_t handle, spi_mode_t mode);

/**
 * @brief 设置SPI数据宽度
 * 
 * @param handle SPI设备句柄
 * @param data_width 数据宽度
 * @return api_status_t 操作状态
 */
api_status_t spi_set_data_width(spi_handle_t handle, spi_data_width_t data_width);

#ifdef __cplusplus
}
#endif

#endif /* SPI_API_H */