/**
 * @file protocol_converter_api.h
 * @brief 协议转换器接口抽象层定义
 *
 * 该头文件定义了协议转换器的统一抽象接口，用于处理UART-CAN通讯协议转换
 */

#ifndef PROTOCOL_CONVERTER_API_H
#define PROTOCOL_CONVERTER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "uart_api.h"
#include "can_api.h"

/* 协议转换器句柄 */
typedef driver_handle_t protocol_converter_handle_t;

/* 帧类型枚举 */
typedef enum {
    FRAME_TYPE_COMMAND,       /**< 命令帧 */
    FRAME_TYPE_RESPONSE,      /**< 响应帧 */
    FRAME_TYPE_NOTIFICATION,  /**< 通知帧 */
    FRAME_TYPE_ERROR,         /**< 错误帧 */
    FRAME_TYPE_DATA,          /**< 数据帧 */
    FRAME_TYPE_HEARTBEAT,     /**< 心跳帧 */
    FRAME_TYPE_CUSTOM         /**< 自定义帧 */
} frame_type_t;

/* 转换器错误枚举 */
typedef enum {
    CONVERTER_ERROR_NONE,             /**< 无错误 */
    CONVERTER_ERROR_INVALID_FRAME,    /**< 无效帧 */
    CONVERTER_ERROR_CHECKSUM,         /**< 校验和错误 */
    CONVERTER_ERROR_TIMEOUT,          /**< 超时错误 */
    CONVERTER_ERROR_BUSY,             /**< 设备忙 */
    CONVERTER_ERROR_NOT_SUPPORTED,    /**< 不支持的命令 */
    CONVERTER_ERROR_BUFFER_OVERFLOW,  /**< 缓冲区溢出 */
    CONVERTER_ERROR_UNKNOWN           /**< 未知错误 */
} converter_error_t;

/* 帧结构体 */
typedef struct {
    frame_type_t type;        /**< 帧类型 */
    uint8_t command;          /**< 命令字 */
    uint8_t subcommand;       /**< 子命令字 */
    uint16_t length;          /**< 数据长度 */
    uint8_t *data;            /**< 数据指针 */
    uint16_t checksum;        /**< 校验和 */
    uint8_t source_addr;      /**< 源地址 */
    uint8_t dest_addr;        /**< 目标地址 */
    uint8_t sequence;         /**< 序列号 */
} protocol_frame_t;

/* 协议转换器配置参数 */
typedef struct {
    uart_handle_t uart_handle;        /**< UART句柄 */
    uint32_t baud_rate;               /**< 波特率 */
    uint16_t buffer_size;             /**< 缓冲区大小 */
    uint16_t timeout_ms;              /**< 超时时间(毫秒) */
    uint8_t device_addr;              /**< 设备地址 */
    bool use_checksum;                /**< 是否使用校验和 */
    uint8_t retries;                  /**< 重试次数 */
    bool use_escape_chars;            /**< 是否使用转义字符 */
} protocol_converter_config_t;

/* 帧回调函数类型 */
typedef void (*protocol_frame_callback_t)(protocol_frame_t *frame, void *user_data);

/**
 * @brief 初始化协议转换器
 * 
 * @param config 转换器配置参数
 * @param handle 转换器句柄指针
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_init(const protocol_converter_config_t *config, 
                           protocol_converter_handle_t *handle);

/**
 * @brief 注册帧接收回调
 * 
 * @param handle 转换器句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_register_callback(protocol_converter_handle_t handle, 
                                        protocol_frame_callback_t callback, 
                                        void *user_data);

/**
 * @brief 发送协议帧
 * 
 * @param handle 转换器句柄
 * @param frame 帧结构体指针
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_send_frame(protocol_converter_handle_t handle, 
                                 const protocol_frame_t *frame);

/**
 * @brief 接收协议帧
 * 
 * @param handle 转换器句柄
 * @param frame 帧结构体指针
 * @param timeout_ms 超时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_receive_frame(protocol_converter_handle_t handle, 
                                    protocol_frame_t *frame, 
                                    uint16_t timeout_ms);

/**
 * @brief 发送命令并等待响应
 * 
 * @param handle 转换器句柄
 * @param cmd_frame 命令帧指针
 * @param resp_frame 响应帧指针
 * @param timeout_ms 超时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_send_command(protocol_converter_handle_t handle, 
                                   const protocol_frame_t *cmd_frame, 
                                   protocol_frame_t *resp_frame, 
                                   uint16_t timeout_ms);

/**
 * @brief 构建协议帧
 * 
 * @param frame 帧结构体指针
 * @param type 帧类型
 * @param command 命令字
 * @param subcommand 子命令字
 * @param data 数据指针
 * @param length 数据长度
 * @param source_addr 源地址
 * @param dest_addr 目标地址
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_build_frame(protocol_frame_t *frame, 
                                  frame_type_t type, 
                                  uint8_t command, 
                                  uint8_t subcommand, 
                                  const uint8_t *data, 
                                  uint16_t length, 
                                  uint8_t source_addr, 
                                  uint8_t dest_addr);

/**
 * @brief 解析协议帧
 * 
 * @param raw_data 原始数据
 * @param data_len 数据长度
 * @param frame 帧结构体指针
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_parse_frame(const uint8_t *raw_data, 
                                  uint16_t data_len, 
                                  protocol_frame_t *frame);

/**
 * @brief 计算帧校验和
 * 
 * @param frame 帧结构体指针
 * @return uint16_t 计算出的校验和
 */
uint16_t protocol_converter_calculate_checksum(const protocol_frame_t *frame);

/**
 * @brief 验证帧校验和
 * 
 * @param frame 帧结构体指针
 * @return bool true表示校验正确，false表示校验错误
 */
bool protocol_converter_verify_checksum(const protocol_frame_t *frame);

/**
 * @brief 转换器错误处理
 * 
 * @param handle 转换器句柄
 * @param error 错误码
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_handle_error(protocol_converter_handle_t handle, 
                                   converter_error_t error);

/**
 * @brief 设置帧超时
 * 
 * @param handle 转换器句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_set_timeout(protocol_converter_handle_t handle, 
                                  uint16_t timeout_ms);

/**
 * @brief 清空转换器缓冲区
 * 
 * @param handle 转换器句柄
 * @return int 0表示成功，非0表示失败
 */
int protocol_converter_flush(protocol_converter_handle_t handle);

#endif /* PROTOCOL_CONVERTER_API_H */
