/**
 * @file websocket_api.h
 * @brief WebSocket客户端接口定义
 *
 * 该头文件定义了WebSocket客户端的统一接口，支持双向实时通信
 */

#ifndef WEBSOCKET_API_H
#define WEBSOCKET_API_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol/network_api.h"
#include "common/error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket消息类型 */
typedef enum {
    WS_MESSAGE_TYPE_TEXT = 0,       /**< 文本消息 */
    WS_MESSAGE_TYPE_BINARY,         /**< 二进制消息 */
    WS_MESSAGE_TYPE_PING,           /**< Ping消息 */
    WS_MESSAGE_TYPE_PONG,           /**< Pong消息 */
    WS_MESSAGE_TYPE_CLOSE,          /**< 关闭消息 */
    WS_MESSAGE_TYPE_CONTINUE        /**< 延续消息 */
} ws_message_type_t;

/* WebSocket事件类型 */
typedef enum {
    WS_EVENT_CONNECTED,             /**< 已连接 */
    WS_EVENT_DISCONNECTED,          /**< 已断开连接 */
    WS_EVENT_TEXT_DATA,             /**< 收到文本数据 */
    WS_EVENT_BINARY_DATA,           /**< 收到二进制数据 */
    WS_EVENT_PING,                  /**< 收到Ping */
    WS_EVENT_PONG,                  /**< 收到Pong */
    WS_EVENT_ERROR                  /**< 错误 */
} ws_event_t;

/* WebSocket关闭状态码 */
typedef enum {
    WS_CLOSE_NORMAL = 1000,                 /**< 正常关闭 */
    WS_CLOSE_GOING_AWAY = 1001,             /**< 终端离开 */
    WS_CLOSE_PROTOCOL_ERROR = 1002,         /**< 协议错误 */
    WS_CLOSE_UNSUPPORTED_DATA = 1003,       /**< 不支持的数据 */
    WS_CLOSE_NO_STATUS = 1005,              /**< 无状态码 */
    WS_CLOSE_ABNORMAL = 1006,               /**< 异常关闭 */
    WS_CLOSE_INVALID_PAYLOAD = 1007,        /**< 无效数据 */
    WS_CLOSE_POLICY_VIOLATION = 1008,       /**< 策略违规 */
    WS_CLOSE_MESSAGE_TOO_BIG = 1009,        /**< 消息过大 */
    WS_CLOSE_EXTENSION_REQUIRED = 1010,     /**< 需要扩展 */
    WS_CLOSE_UNEXPECTED_CONDITION = 1011,   /**< 意外情况 */
    WS_CLOSE_TLS_HANDSHAKE_FAILED = 1015    /**< TLS握手失败 */
} ws_close_code_t;

/* WebSocket客户端句柄 */
typedef void* ws_handle_t;

/* WebSocket配置 */
typedef struct {
    char url[256];                 /**< WebSocket URL */
    char** protocols;              /**< 支持的协议 */
    uint8_t protocol_count;        /**< 协议数量 */
    char** headers;                /**< 额外的HTTP头部 */
    uint8_t header_count;          /**< 头部数量 */
    uint32_t ping_interval_ms;     /**< Ping间隔(毫秒) */
    uint32_t timeout_ms;           /**< 超时(毫秒) */
    uint32_t max_message_size;     /**< 最大消息大小 */
    bool auto_reconnect;           /**< 是否自动重连 */
    uint8_t reconnect_max_retry;   /**< 最大重试次数 */
    uint16_t reconnect_interval_ms;/**< 重连间隔(毫秒) */
    bool use_tls;                  /**< 是否使用TLS */
    struct {
        const char* ca_cert;       /**< CA证书 */
        const char* client_cert;   /**< 客户端证书 */
        const char* client_key;    /**< 客户端密钥 */
        bool skip_cert_verify;     /**< 是否跳过证书验证 */
    } tls;
} ws_config_t;

/* WebSocket消息 */
typedef struct {
    ws_message_type_t type;        /**< 消息类型 */
    uint8_t* data;                 /**< 消息数据 */
    uint32_t len;                  /**< 数据长度 */
    bool fin;                      /**< 是否最后一帧 */
} ws_message_t;

/* WebSocket事件回调函数类型 */
typedef void (*ws_event_callback_t)(ws_handle_t handle, ws_event_t event, ws_message_t* msg, void* user_data);

/**
 * @brief 初始化WebSocket客户端
 * 
 * @param config 配置参数
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @param handle 返回的客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int ws_init(const ws_config_t* config, ws_event_callback_t callback, void* user_data, ws_handle_t* handle);

/**
 * @brief 释放WebSocket客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int ws_deinit(ws_handle_t handle);

/**
 * @brief 连接WebSocket服务器
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int ws_connect(ws_handle_t handle);

/**
 * @brief 断开WebSocket连接
 * 
 * @param handle 客户端句柄
 * @param code 关闭状态码
 * @param reason 关闭原因
 * @return int 成功返回0，失败返回错误码
 */
int ws_disconnect(ws_handle_t handle, ws_close_code_t code, const char* reason);

/**
 * @brief 发送文本消息
 * 
 * @param handle 客户端句柄
 * @param text 文本数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int ws_send_text(ws_handle_t handle, const char* text, uint32_t len);

/**
 * @brief 发送二进制消息
 * 
 * @param handle 客户端句柄
 * @param data 二进制数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int ws_send_binary(ws_handle_t handle, const uint8_t* data, uint32_t len);

/**
 * @brief 发送Ping消息
 * 
 * @param handle 客户端句柄
 * @param data 可选数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int ws_send_ping(ws_handle_t handle, const uint8_t* data, uint32_t len);

/**
 * @brief 发送Pong消息
 * 
 * @param handle 客户端句柄
 * @param data 可选数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int ws_send_pong(ws_handle_t handle, const uint8_t* data, uint32_t len);

/**
 * @brief 接收消息
 * 
 * @param handle 客户端句柄
 * @param msg 返回的消息
 * @param timeout_ms 超时时间(毫秒)
 * @return int 成功返回0，失败返回错误码
 */
int ws_receive(ws_handle_t handle, ws_message_t* msg, uint32_t timeout_ms);

/**
 * @brief 检查WebSocket连接状态
 * 
 * @param handle 客户端句柄
 * @param connected 返回的连接状态
 * @return int 成功返回0，失败返回错误码
 */
int ws_is_connected(ws_handle_t handle, bool* connected);

/**
 * @brief 设置WebSocket选项
 * 
 * @param handle 客户端句柄
 * @param option 选项名
 * @param value 选项值
 * @return int 成功返回0，失败返回错误码
 */
int ws_set_option(ws_handle_t handle, const char* option, const void* value);

/**
 * @brief 获取WebSocket选项
 * 
 * @param handle 客户端句柄
 * @param option 选项名
 * @param value 返回的选项值
 * @return int 成功返回0，失败返回错误码
 */
int ws_get_option(ws_handle_t handle, const char* option, void* value);

/**
 * @brief 释放消息资源
 * 
 * @param msg 消息
 * @return int 成功返回0，失败返回错误码
 */
int ws_free_message(ws_message_t* msg);

/**
 * @brief 启动自动Ping
 * 
 * @param handle 客户端句柄
 * @param interval_ms Ping间隔(毫秒)
 * @return int 成功返回0，失败返回错误码
 */
int ws_start_auto_ping(ws_handle_t handle, uint32_t interval_ms);

/**
 * @brief 停止自动Ping
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int ws_stop_auto_ping(ws_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* WEBSOCKET_API_H */