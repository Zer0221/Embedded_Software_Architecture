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
#include "driver_api.h"

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
    spi_channel_t channel;        /**< SPI通道 */
    spi_mode_t mode;              /**< SPI模式 */
    spi_bit_order_t bit_order;    /**< 位序 */
    spi_data_width_t data_width;  /**< 数据宽度 */
    spi_cs_mode_t cs_mode;        /**< 片选模式 */
    uint32_t clock_hz;            /**< 时钟频率（Hz） */
    uint8_t cs_pin;               /**< 软件片选引脚（当cs_mode为SPI_CS_MODE_SOFTWARE时有效） */
} spi_config_t;

/* SPI设备句柄 */
typedef driver_handle_t spi_handle_t;

/**
 * @brief 初始化SPI
 * 
 * @param config SPI配置参数
 * @param handle SPI设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int spi_init(const spi_config_t *config, spi_handle_t *handle);

/**
 * @brief 去初始化SPI
 * 
 * @param handle SPI设备句柄
 * @return int 0表示成功，非0表示失败
 */
int spi_deinit(spi_handle_t handle);

/**
 * @brief SPI传输数据（全双工）
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲区
 * @param len 数据长度（字节数）
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错误
 */
int spi_transfer(spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, 
                uint32_t len, uint32_t timeout_ms);

/**
 * @brief SPI只发送数据
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param len 数据长度（字节数）
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功发送的字节数，负值表示错误
 */
int spi_transmit(spi_handle_t handle, const uint8_t *tx_data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief SPI只接收数据
 * 
 * @param handle SPI设备句柄
 * @param rx_data 接收数据缓冲区
 * @param len 数据长度（字节数）
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错误
 */
int spi_receive(spi_handle_t handle, uint8_t *rx_data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 软件控制SPI片选信号
 * 
 * @param handle SPI设备句柄
 * @param state 片选状态，0表示选中，1表示取消选中
 * @return int 0表示成功，非0表示失败
 */
int spi_cs_control(spi_handle_t handle, uint8_t state);

#endif /* SPI_API_H */
