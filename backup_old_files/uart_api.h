/**
 * @file uart_api.h
 * @brief UART接口抽象层定义
 *
 * 该头文件定义了UART接口的统一抽象，使上层应用能够与底层UART硬件实现解耦
 */

#ifndef UART_API_H
#define UART_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* UART通道ID定义 */
typedef enum {
    UART_CHANNEL_0 = 0,    /**< UART通道0 */
    UART_CHANNEL_1,        /**< UART通道1 */
    UART_CHANNEL_2,        /**< UART通道2 */
    UART_CHANNEL_3,        /**< UART通道3 */
    UART_CHANNEL_MAX       /**< UART通道最大数量 */
} uart_channel_t;

/* UART波特率定义 */
typedef enum {
    UART_BAUDRATE_1200    = 1200,    /**< 1200 bps */
    UART_BAUDRATE_2400    = 2400,    /**< 2400 bps */
    UART_BAUDRATE_4800    = 4800,    /**< 4800 bps */
    UART_BAUDRATE_9600    = 9600,    /**< 9600 bps */
    UART_BAUDRATE_19200   = 19200,   /**< 19200 bps */
    UART_BAUDRATE_38400   = 38400,   /**< 38400 bps */
    UART_BAUDRATE_57600   = 57600,   /**< 57600 bps */
    UART_BAUDRATE_115200  = 115200,  /**< 115200 bps */
    UART_BAUDRATE_230400  = 230400,  /**< 230400 bps */
    UART_BAUDRATE_460800  = 460800,  /**< 460800 bps */
    UART_BAUDRATE_921600  = 921600,  /**< 921600 bps */
    UART_BAUDRATE_CUSTOM  = 0        /**< 自定义波特率 */
} uart_baudrate_t;

/* UART数据位定义 */
typedef enum {
    UART_DATA_BITS_5 = 0,   /**< 5位数据位 */
    UART_DATA_BITS_6,       /**< 6位数据位 */
    UART_DATA_BITS_7,       /**< 7位数据位 */
    UART_DATA_BITS_8,       /**< 8位数据位 */
    UART_DATA_BITS_9        /**< 9位数据位 */
} uart_data_bits_t;

/* UART停止位定义 */
typedef enum {
    UART_STOP_BITS_1 = 0,   /**< 1位停止位 */
    UART_STOP_BITS_1_5,     /**< 1.5位停止位 */
    UART_STOP_BITS_2        /**< 2位停止位 */
} uart_stop_bits_t;

/* UART奇偶校验定义 */
typedef enum {
    UART_PARITY_NONE = 0,   /**< 无校验 */
    UART_PARITY_ODD,        /**< 奇校验 */
    UART_PARITY_EVEN        /**< 偶校验 */
} uart_parity_t;

/* UART硬件流控定义 */
typedef enum {
    UART_FLOW_CONTROL_NONE = 0,     /**< 无流控 */
    UART_FLOW_CONTROL_RTS,          /**< RTS流控 */
    UART_FLOW_CONTROL_CTS,          /**< CTS流控 */
    UART_FLOW_CONTROL_RTS_CTS       /**< RTS/CTS流控 */
} uart_flow_control_t;

/* UART配置参数 */
typedef struct {
    uart_channel_t channel;          /**< UART通道 */
    uart_baudrate_t baudrate;        /**< 波特率 */
    uart_data_bits_t data_bits;      /**< 数据位 */
    uart_stop_bits_t stop_bits;      /**< 停止位 */
    uart_parity_t parity;            /**< 校验位 */
    uart_flow_control_t flow_control;/**< 硬件流控 */
    uint32_t custom_baudrate;        /**< 自定义波特率，当baudrate为UART_BAUDRATE_CUSTOM时有效 */
} uart_config_t;

/* UART接收回调函数类型 */
typedef void (*uart_rx_callback_t)(uint8_t *data, uint32_t size, void *user_data);

/* UART设备句柄 */
typedef driver_handle_t uart_handle_t;

/**
 * @brief 初始化UART
 * 
 * @param config UART配置参数
 * @param handle UART设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int uart_init(const uart_config_t *config, uart_handle_t *handle);

/**
 * @brief 去初始化UART
 * 
 * @param handle UART设备句柄
 * @return int 0表示成功，非0表示失败
 */
int uart_deinit(uart_handle_t handle);

/**
 * @brief UART发送数据
 * 
 * @param handle UART设备句柄
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功发送的字节数，负值表示错误
 */
int uart_transmit(uart_handle_t handle, const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief UART接收数据
 * 
 * @param handle UART设备句柄
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错误
 */
int uart_receive(uart_handle_t handle, uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 注册UART接收回调函数（中断模式）
 * 
 * @param handle UART设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int uart_register_rx_callback(uart_handle_t handle, uart_rx_callback_t callback, void *user_data);

/**
 * @brief 获取UART接收缓冲区中可读取的数据长度
 * 
 * @param handle UART设备句柄
 * @return int 可读取的数据长度，负值表示错误
 */
int uart_get_rx_data_size(uart_handle_t handle);

/**
 * @brief 清空UART接收缓冲区
 * 
 * @param handle UART设备句柄
 * @return int 0表示成功，非0表示失败
 */
int uart_flush_rx_buffer(uart_handle_t handle);

#endif /* UART_API_H */
