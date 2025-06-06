/**
 * @file can_api.h
 * @brief CAN总线接口抽象层定义
 *
 * 该头文件定义了CAN总线的统一抽象接口，提供了CAN总线通信、过滤器配置和状态监控功能
 */

#ifndef CAN_API_H
#define CAN_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* CAN操作状态 */
typedef enum {
    CAN_STATUS_IDLE,         /**< 空闲状态 */
    CAN_STATUS_BUSY,         /**< 忙状态 */
    CAN_STATUS_COMPLETE,     /**< 操作完成 */
    CAN_STATUS_ERROR,        /**< 操作错误 */
    CAN_STATUS_TIMEOUT       /**< 操作超时 */
} can_status_t;

/* CAN工作模式 */
typedef enum {
    CAN_MODE_NORMAL,         /**< 正常模式 */
    CAN_MODE_LOOPBACK,       /**< 回环模式 */
    CAN_MODE_SILENT,         /**< 静默模式 */
    CAN_MODE_SILENT_LOOPBACK /**< 静默回环模式 */
} can_mode_t;

/* CAN标识符类型 */
typedef enum {
    CAN_ID_STANDARD,         /**< 标准帧ID (11位) */
    CAN_ID_EXTENDED          /**< 扩展帧ID (29位) */
} can_id_type_t;

/* CAN帧类型 */
typedef enum {
    CAN_FRAME_DATA,          /**< 数据帧 */
    CAN_FRAME_REMOTE         /**< 远程帧 */
} can_frame_type_t;

/* CAN总线波特率 */
typedef enum {
    CAN_BAUDRATE_10K,        /**< 10 kbit/s */
    CAN_BAUDRATE_20K,        /**< 20 kbit/s */
    CAN_BAUDRATE_50K,        /**< 50 kbit/s */
    CAN_BAUDRATE_100K,       /**< 100 kbit/s */
    CAN_BAUDRATE_125K,       /**< 125 kbit/s */
    CAN_BAUDRATE_250K,       /**< 250 kbit/s */
    CAN_BAUDRATE_500K,       /**< 500 kbit/s */
    CAN_BAUDRATE_800K,       /**< 800 kbit/s */
    CAN_BAUDRATE_1M          /**< 1 Mbit/s */
} can_baudrate_t;

/* CAN过滤器类型 */
typedef enum {
    CAN_FILTER_MASK,         /**< 掩码模式过滤器 */
    CAN_FILTER_LIST          /**< 列表模式过滤器 */
} can_filter_type_t;

/* CAN过滤器配置 */
typedef struct {
    uint8_t filter_num;         /**< 过滤器编号 */
    can_filter_type_t type;     /**< 过滤器类型 */
    can_id_type_t id_type;      /**< 标识符类型 */
    uint32_t id;                /**< 标识符/掩码值 */
    uint32_t mask;              /**< 掩码/列表中第二个ID值 */
    bool active;                /**< 过滤器启用状态 */
} can_filter_t;

/* CAN消息帧 */
typedef struct {
    uint32_t id;                /**< 报文ID */
    can_id_type_t id_type;      /**< ID类型 */
    can_frame_type_t frame_type; /**< 帧类型 */
    uint8_t dlc;                /**< 数据长度 */
    uint8_t data[8];            /**< 数据缓冲区 */
    uint32_t timestamp;         /**< 时间戳 */
} can_message_t;

/* CAN统计信息 */
typedef struct {
    uint32_t rx_count;          /**< 接收计数 */
    uint32_t tx_count;          /**< 发送计数 */
    uint32_t error_count;       /**< 错误计数 */
    uint32_t overrun_count;     /**< 溢出计数 */
    uint8_t tx_error_counter;   /**< 发送错误计数器 */
    uint8_t rx_error_counter;   /**< 接收错误计数器 */
    bool bus_off;               /**< 总线关闭状态 */
    bool passive_error;         /**< 被动错误状态 */
} can_stats_t;

/* CAN配置 */
typedef struct {
    can_mode_t mode;            /**< CAN工作模式 */
    can_baudrate_t baudrate;    /**< 总线波特率 */
    bool auto_retransmit;       /**< 自动重传使能 */
    bool auto_bus_off_recovery; /**< 自动恢复使能 */
    bool rx_fifo_locked_mode;   /**< 接收FIFO锁定模式 */
    bool tx_fifo_priority;      /**< 发送FIFO优先级由ID决定 */
} can_config_t;

/* CAN回调函数类型 */
typedef void (*can_callback_t)(void *arg, can_status_t status, can_message_t *msg);

/* CAN设备句柄 */
typedef driver_handle_t can_handle_t;

/**
 * @brief 初始化CAN设备
 * 
 * @param config CAN配置参数
 * @param callback CAN事件回调函数
 * @param arg 传递给回调函数的参数
 * @param handle CAN设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int can_init(const can_config_t *config, can_callback_t callback, void *arg, can_handle_t *handle);

/**
 * @brief 去初始化CAN设备
 * 
 * @param handle CAN设备句柄
 * @return int 0表示成功，非0表示失败
 */
int can_deinit(can_handle_t handle);

/**
 * @brief 配置CAN过滤器
 * 
 * @param handle CAN设备句柄
 * @param filter 过滤器配置
 * @return int 0表示成功，非0表示失败
 */
int can_config_filter(can_handle_t handle, const can_filter_t *filter);

/**
 * @brief 启用CAN设备
 * 
 * @param handle CAN设备句柄
 * @return int 0表示成功，非0表示失败
 */
int can_start(can_handle_t handle);

/**
 * @brief 停止CAN设备
 * 
 * @param handle CAN设备句柄
 * @return int 0表示成功，非0表示失败
 */
int can_stop(can_handle_t handle);

/**
 * @brief 发送CAN消息
 * 
 * @param handle CAN设备句柄
 * @param msg 要发送的消息
 * @param timeout_ms 超时时间(毫秒)，0表示不等待，UINT32_MAX表示一直等待
 * @return int 0表示成功，非0表示失败
 */
int can_transmit(can_handle_t handle, const can_message_t *msg, uint32_t timeout_ms);

/**
 * @brief 接收CAN消息
 * 
 * @param handle CAN设备句柄
 * @param msg 接收消息的缓冲区
 * @param timeout_ms 超时时间(毫秒)，0表示不等待，UINT32_MAX表示一直等待
 * @return int 0表示成功，非0表示失败
 */
int can_receive(can_handle_t handle, can_message_t *msg, uint32_t timeout_ms);

/**
 * @brief 获取CAN统计信息
 * 
 * @param handle CAN设备句柄
 * @param stats 统计信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int can_get_stats(can_handle_t handle, can_stats_t *stats);

/**
 * @brief 清除CAN统计信息
 * 
 * @param handle CAN设备句柄
 * @return int 0表示成功，非0表示失败
 */
int can_clear_stats(can_handle_t handle);

/**
 * @brief 获取CAN错误状态
 * 
 * @param handle CAN设备句柄
 * @param bus_off 总线关闭状态指针
 * @param passive_error 被动错误状态指针
 * @return int 0表示成功，非0表示失败
 */
int can_get_error_status(can_handle_t handle, bool *bus_off, bool *passive_error);

/**
 * @brief 重置CAN错误计数器
 * 
 * @param handle CAN设备句柄
 * @return int 0表示成功，非0表示失败
 */
int can_reset_error_counters(can_handle_t handle);

/**
 * @brief 设置CAN工作模式
 * 
 * @param handle CAN设备句柄
 * @param mode 工作模式
 * @return int 0表示成功，非0表示失败
 */
int can_set_mode(can_handle_t handle, can_mode_t mode);

/**
 * @brief 设置CAN接收回调函数
 * 
 * @param handle CAN设备句柄
 * @param callback 回调函数
 * @param arg 传递给回调函数的参数
 * @return int 0表示成功，非0表示失败
 */
int can_set_rx_callback(can_handle_t handle, can_callback_t callback, void *arg);

/**
 * @brief 获取CAN设备状态
 * 
 * @param handle CAN设备句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int can_get_status(can_handle_t handle, can_status_t *status);

#endif /* CAN_API_H */