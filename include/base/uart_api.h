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
#include "common/driver_api.h"

/* UART通道ID定义 */
typedef enum {
    UART_CHANNEL_0 = 0,    /**< UART通道0 */
    UART_CHANNEL_1,        /**< UART通道1 */
    UART_CHANNEL_2,        /**< UART通道2 */
    UART_CHANNEL_3,        /**< UART通道3 */
    UART_CHANNEL_MAX       /**< UART通道最大数量 */
} uart_channel_id_t;

/* UART波特率定义 */
typedef enum {
    UART_BAUDRATE_1200     = 1200,     /**< 1200波特率 */
    UART_BAUDRATE_2400     = 2400,     /**< 2400波特率 */
    UART_BAUDRATE_4800     = 4800,     /**< 4800波特率 */
    UART_BAUDRATE_9600     = 9600,     /**< 9600波特率 */
    UART_BAUDRATE_19200    = 19200,    /**< 19200波特率 */
    UART_BAUDRATE_38400    = 38400,    /**< 38400波特率 */
    UART_BAUDRATE_57600    = 57600,    /**< 57600波特率 */
    UART_BAUDRATE_115200   = 115200,   /**< 115200波特率 */
    UART_BAUDRATE_230400   = 230400,   /**< 230400波特率 */
    UART_BAUDRATE_460800   = 460800,   /**< 460800波特率 */
    UART_BAUDRATE_921600   = 921600,   /**< 921600波特率 */
    UART_BAUDRATE_1000000  = 1000000,  /**< 1000000波特率 */
    UART_BAUDRATE_2000000  = 2000000,  /**< 2000000波特率 */
    UART_BAUDRATE_3000000  = 3000000   /**< 3000000波特率 */
} uart_baudrate_t;

/* UART数据位数 */
typedef enum {
    UART_DATABITS_5 = 5,   /**< 5位数据位 */
    UART_DATABITS_6 = 6,   /**< 6位数据位 */
    UART_DATABITS_7 = 7,   /**< 7位数据位 */
    UART_DATABITS_8 = 8,   /**< 8位数据位 */
    UART_DATABITS_9 = 9    /**< 9位数据位 */
} uart_databits_t;

/* UART停止位 */
typedef enum {
    UART_STOPBITS_0_5 = 0, /**< 0.5位停止位 */
    UART_STOPBITS_1,       /**< 1位停止位 */
    UART_STOPBITS_1_5,     /**< 1.5位停止位 */
    UART_STOPBITS_2        /**< 2位停止位 */
} uart_stopbits_t;

/* UART校验位 */
typedef enum {
    UART_PARITY_NONE = 0,  /**< 无校验 */
    UART_PARITY_ODD,       /**< 奇校验 */
    UART_PARITY_EVEN,      /**< 偶校验 */
    UART_PARITY_MARK,      /**< 标记校验 */
    UART_PARITY_SPACE      /**< 空白校验 */
} uart_parity_t;

/* UART流控制 */
typedef enum {
    UART_FLOWCTRL_NONE = 0,    /**< 无流控 */
    UART_FLOWCTRL_RTS,         /**< RTS流控 */
    UART_FLOWCTRL_CTS,         /**< CTS流控 */
    UART_FLOWCTRL_RTS_CTS      /**< RTS/CTS流控 */
} uart_flowctrl_t;

/* UART配置结构体 */
typedef struct {
    uart_channel_id_t  channel;    /**< UART通道 */
    uart_baudrate_t    baudrate;   /**< 波特率 */
    uart_databits_t    databits;   /**< 数据位 */
    uart_stopbits_t    stopbits;   /**< 停止位 */
    uart_parity_t      parity;     /**< 校验位 */
    uart_flowctrl_t    flowctrl;   /**< 流控制 */
    bool               use_dma;    /**< 是否使用DMA */
    uint32_t           rx_buf_size; /**< 接收缓冲区大小 */
    uint32_t           tx_buf_size; /**< 发送缓冲区大小 */
} uart_config_t;

/* UART事件类型 */
typedef enum {
    UART_EVENT_RX_CHAR,         /**< 接收到字符 */
    UART_EVENT_RX_DATA,         /**< 接收到数据块 */
    UART_EVENT_TX_DONE,         /**< 发送完成 */
    UART_EVENT_ERROR_OVERRUN,   /**< 溢出错误 */
    UART_EVENT_ERROR_PARITY,    /**< 校验错误 */
    UART_EVENT_ERROR_FRAMING,   /**< 帧错误 */
    UART_EVENT_BREAK            /**< 接收到中断信号 */
} uart_event_type_t;

/* UART事件结构体 */
typedef struct {
    uart_event_type_t type;     /**< 事件类型 */
    union {
        uint8_t  rx_char;       /**< 接收到的字符 */
        struct {
            uint8_t  *data;     /**< 数据指针 */
            uint32_t size;      /**< 数据大小 */
        } rx_data;
    } data;
} uart_event_t;

/* UART事件回调函数类型 */
typedef void (*uart_event_callback_t)(uart_event_t *event, void *user_data);

/**
 * @brief 初始化UART设备
 *
 * @param config 配置参数
 * @return 成功返回UART句柄，失败返回NULL
 */
driver_handle_t uart_init(uart_config_t *config);

/**
 * @brief 关闭UART设备
 *
 * @param handle UART句柄
 * @return 成功返回0，失败返回负数错误码
 */
int uart_deinit(driver_handle_t handle);

/**
 * @brief 发送数据
 *
 * @param handle UART句柄
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return 成功返回发送的字节数，失败返回负数错误码
 */
int uart_send(driver_handle_t handle, const uint8_t *data, uint32_t size);

/**
 * @brief 接收数据
 *
 * @param handle UART句柄
 * @param data 数据缓冲区
 * @param size 要接收的字节数
 * @param timeout_ms 超时时间(毫秒)，0表示非阻塞，UINT32_MAX表示永久阻塞
 * @return 成功返回接收的字节数，失败返回负数错误码
 */
int uart_receive(driver_handle_t handle, uint8_t *data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief 注册UART事件回调函数
 *
 * @param handle UART句柄
 * @param callback 回调函数
 * @param user_data 用户数据，会在回调中传递
 * @return 成功返回0，失败返回负数错误码
 */
int uart_register_event_callback(driver_handle_t handle, uart_event_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* UART_API_H */