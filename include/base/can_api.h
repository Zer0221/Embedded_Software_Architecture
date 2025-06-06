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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* CAN过滤器配置 */
typedef struct {
    uint32_t id;             /**< 过滤器ID */
    uint32_t mask;           /**< 过滤器掩码 */
    can_id_type_t id_type;   /**< ID类型 (标准或扩展) */
    bool enable;             /**< 过滤器使能状态 */
} can_filter_t;

/* CAN消息结构体 */
typedef struct {
    uint32_t id;             /**< 消息ID */
    can_id_type_t id_type;   /**< ID类型 (标准或扩展) */
    can_frame_type_t frame_type; /**< 帧类型 (数据或远程) */
    uint8_t dlc;             /**< 数据长度码 (0-8) */
    uint8_t data[8];         /**< 数据字段 */
    uint32_t timestamp;      /**< 时间戳 */
} can_message_t;

/* CAN总线统计信息 */
typedef struct {
    uint32_t tx_frames;      /**< 发送的帧数 */
    uint32_t rx_frames;      /**< 接收的帧数 */
    uint32_t tx_errors;      /**< 发送错误数 */
    uint32_t rx_errors;      /**< 接收错误数 */
    uint32_t bus_off_count;  /**< 总线关闭计数 */
    uint32_t error_warnings; /**< 错误警告计数 */
    uint32_t overrun_count;  /**< 溢出计数 */
} can_statistics_t;

/* CAN控制器配置 */
typedef struct {
    can_mode_t mode;         /**< CAN模式 */
    can_baudrate_t baudrate; /**< 波特率 */
    bool auto_retransmit;    /**< 自动重传 */
    bool auto_bus_off_recovery; /**< 自动总线关闭恢复 */
    uint8_t tx_fifo_size;    /**< 发送FIFO大小 */
    uint8_t rx_fifo_size;    /**< 接收FIFO大小 */
} can_config_t;

/* CAN总线状态 */
typedef struct {
    bool tx_busy;            /**< 发送忙状态 */
    bool rx_busy;            /**< 接收忙状态 */
    bool bus_off;            /**< 总线关闭状态 */
    bool error_warning;      /**< 错误警告状态 */
    bool error_passive;      /**< 错误被动状态 */
} can_bus_status_t;

/* CAN接收回调函数类型 */
typedef void (*can_rx_callback_t)(can_message_t *message, void *user_data);

/* CAN事件回调函数类型 */
typedef void (*can_event_callback_t)(uint32_t events, void *user_data);

/**
 * @brief 初始化CAN控制器
 *
 * @param controller CAN控制器索引
 * @param config CAN控制器配置
 * @return api_status_t 操作状态
 */
api_status_t can_init(uint8_t controller, const can_config_t *config);

/**
 * @brief 反初始化CAN控制器
 *
 * @param controller CAN控制器索引
 * @return api_status_t 操作状态
 */
api_status_t can_deinit(uint8_t controller);

/**
 * @brief 配置CAN过滤器
 *
 * @param controller CAN控制器索引
 * @param filter_idx 过滤器索引
 * @param filter 过滤器配置
 * @return api_status_t 操作状态
 */
api_status_t can_set_filter(uint8_t controller, uint8_t filter_idx, const can_filter_t *filter);

/**
 * @brief 发送CAN消息
 *
 * @param controller CAN控制器索引
 * @param message 要发送的消息
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t can_send(uint8_t controller, const can_message_t *message, uint32_t timeout_ms);

/**
 * @brief 接收CAN消息
 *
 * @param controller CAN控制器索引
 * @param message 接收到的消息
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t can_receive(uint8_t controller, can_message_t *message, uint32_t timeout_ms);

/**
 * @brief 注册CAN接收回调函数
 *
 * @param controller CAN控制器索引
 * @param callback 接收回调函数
 * @param user_data 用户数据指针
 * @return api_status_t 操作状态
 */
api_status_t can_register_rx_callback(uint8_t controller, can_rx_callback_t callback, void *user_data);

/**
 * @brief 注册CAN事件回调函数
 *
 * @param controller CAN控制器索引
 * @param callback 事件回调函数
 * @param user_data 用户数据指针
 * @return api_status_t 操作状态
 */
api_status_t can_register_event_callback(uint8_t controller, can_event_callback_t callback, void *user_data);

/**
 * @brief 获取CAN总线状态
 *
 * @param controller CAN控制器索引
 * @param status 总线状态
 * @return api_status_t 操作状态
 */
api_status_t can_get_bus_status(uint8_t controller, can_bus_status_t *status);

/**
 * @brief 获取CAN统计信息
 *
 * @param controller CAN控制器索引
 * @param statistics 统计信息
 * @return api_status_t 操作状态
 */
api_status_t can_get_statistics(uint8_t controller, can_statistics_t *statistics);

/**
 * @brief 清除CAN统计信息
 *
 * @param controller CAN控制器索引
 * @return api_status_t 操作状态
 */
api_status_t can_clear_statistics(uint8_t controller);

/**
 * @brief 启动CAN控制器
 *
 * @param controller CAN控制器索引
 * @return api_status_t 操作状态
 */
api_status_t can_start(uint8_t controller);

/**
 * @brief 停止CAN控制器
 *
 * @param controller CAN控制器索引
 * @return api_status_t 操作状态
 */
api_status_t can_stop(uint8_t controller);

/**
 * @brief 设置CAN控制器模式
 *
 * @param controller CAN控制器索引
 * @param mode CAN模式
 * @return api_status_t 操作状态
 */
api_status_t can_set_mode(uint8_t controller, can_mode_t mode);

/**
 * @brief 获取CAN控制器模式
 *
 * @param controller CAN控制器索引
 * @param mode CAN模式
 * @return api_status_t 操作状态
 */
api_status_t can_get_mode(uint8_t controller, can_mode_t *mode);

#ifdef __cplusplus
}
#endif

#endif /* CAN_API_H */