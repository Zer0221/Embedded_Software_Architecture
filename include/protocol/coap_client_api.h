/**
 * @file coap_client_api.h
 * @brief CoAP客户端接口定义
 *
 * 该头文件定义了CoAP (Constrained Application Protocol) 客户端的统一接口，
 * 适用于资源受限的物联网设备通信
 */

#ifndef COAP_CLIENT_API_H
#define COAP_CLIENT_API_H

#include <stdint.h>
#include <stdbool.h>
#include "network_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CoAP消息类型 */
typedef enum {
    COAP_TYPE_CON = 0,        /**< 可靠消息 */
    COAP_TYPE_NON = 1,        /**< 不可靠消息 */
    COAP_TYPE_ACK = 2,        /**< 确认消息 */
    COAP_TYPE_RST = 3         /**< 重置消息 */
} coap_message_type_t;

/* CoAP方法码 */
typedef enum {
    COAP_METHOD_GET = 1,      /**< GET方法 */
    COAP_METHOD_POST = 2,     /**< POST方法 */
    COAP_METHOD_PUT = 3,      /**< PUT方法 */
    COAP_METHOD_DELETE = 4    /**< DELETE方法 */
} coap_method_t;

/* CoAP内容格式 */
typedef enum {
    COAP_CONTENT_TEXT_PLAIN = 0,             /**< text/plain */
    COAP_CONTENT_LINK_FORMAT = 40,           /**< application/link-format */
    COAP_CONTENT_XML = 41,                   /**< application/xml */
    COAP_CONTENT_OCTET_STREAM = 42,          /**< application/octet-stream */
    COAP_CONTENT_EXI = 47,                   /**< application/exi */
    COAP_CONTENT_JSON = 50,                  /**< application/json */
    COAP_CONTENT_CBOR = 60                   /**< application/cbor */
} coap_content_format_t;

/* CoAP选项类型 */
typedef enum {
    COAP_OPTION_IF_MATCH = 1,               /**< If-Match */
    COAP_OPTION_URI_HOST = 3,               /**< Uri-Host */
    COAP_OPTION_ETAG = 4,                   /**< ETag */
    COAP_OPTION_IF_NONE_MATCH = 5,          /**< If-None-Match */
    COAP_OPTION_OBSERVE = 6,                /**< Observe */
    COAP_OPTION_URI_PORT = 7,               /**< Uri-Port */
    COAP_OPTION_LOCATION_PATH = 8,          /**< Location-Path */
    COAP_OPTION_URI_PATH = 11,              /**< Uri-Path */
    COAP_OPTION_CONTENT_FORMAT = 12,        /**< Content-Format */
    COAP_OPTION_MAX_AGE = 14,               /**< Max-Age */
    COAP_OPTION_URI_QUERY = 15,             /**< Uri-Query */
    COAP_OPTION_ACCEPT = 17,                /**< Accept */
    COAP_OPTION_LOCATION_QUERY = 20,        /**< Location-Query */
    COAP_OPTION_PROXY_URI = 35,             /**< Proxy-Uri */
    COAP_OPTION_PROXY_SCHEME = 39,          /**< Proxy-Scheme */
    COAP_OPTION_SIZE1 = 60                  /**< Size1 */
} coap_option_t;

/* CoAP响应码 */
typedef enum {
    COAP_RESPONSE_CODE_CREATED = 0x41,                 /**< 2.01 Created */
    COAP_RESPONSE_CODE_DELETED = 0x42,                 /**< 2.02 Deleted */
    COAP_RESPONSE_CODE_VALID = 0x43,                   /**< 2.03 Valid */
    COAP_RESPONSE_CODE_CHANGED = 0x44,                 /**< 2.04 Changed */
    COAP_RESPONSE_CODE_CONTENT = 0x45,                 /**< 2.05 Content */
    COAP_RESPONSE_CODE_BAD_REQUEST = 0x80,             /**< 4.00 Bad Request */
    COAP_RESPONSE_CODE_UNAUTHORIZED = 0x81,            /**< 4.01 Unauthorized */
    COAP_RESPONSE_CODE_BAD_OPTION = 0x82,              /**< 4.02 Bad Option */
    COAP_RESPONSE_CODE_FORBIDDEN = 0x83,               /**< 4.03 Forbidden */
    COAP_RESPONSE_CODE_NOT_FOUND = 0x84,               /**< 4.04 Not Found */
    COAP_RESPONSE_CODE_METHOD_NOT_ALLOWED = 0x85,      /**< 4.05 Method Not Allowed */
    COAP_RESPONSE_CODE_NOT_ACCEPTABLE = 0x86,          /**< 4.06 Not Acceptable */
    COAP_RESPONSE_CODE_PRECONDITION_FAILED = 0x8C,     /**< 4.12 Precondition Failed */
    COAP_RESPONSE_CODE_REQUEST_ENTITY_TOO_LARGE = 0x8D,/**< 4.13 Request Entity Too Large */
    COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT = 0x8F,/**< 4.15 Unsupported Content-Format */
    COAP_RESPONSE_CODE_INTERNAL_SERVER_ERROR = 0xA0,   /**< 5.00 Internal Server Error */
    COAP_RESPONSE_CODE_NOT_IMPLEMENTED = 0xA1,         /**< 5.01 Not Implemented */
    COAP_RESPONSE_CODE_BAD_GATEWAY = 0xA2,             /**< 5.02 Bad Gateway */
    COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE = 0xA3,     /**< 5.03 Service Unavailable */
    COAP_RESPONSE_CODE_GATEWAY_TIMEOUT = 0xA4,         /**< 5.04 Gateway Timeout */
    COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED = 0xA5   /**< 5.05 Proxying Not Supported */
} coap_response_code_t;

/* CoAP客户端句柄 */
typedef void* coap_client_handle_t;

/* CoAP消息选项 */
typedef struct {
    coap_option_t option;      /**< 选项类型 */
    uint16_t len;              /**< 选项长度 */
    uint8_t* data;             /**< 选项数据 */
} coap_option_data_t;

/* CoAP消息 */
typedef struct {
    coap_message_type_t type;  /**< 消息类型 */
    coap_method_t method;      /**< 方法码 */
    coap_response_code_t code; /**< 响应码 */
    uint16_t message_id;       /**< 消息ID */
    uint8_t* token;            /**< 令牌 */
    uint8_t token_len;         /**< 令牌长度 */
    coap_option_data_t* options;/**< 选项数组 */
    uint8_t option_count;      /**< 选项数量 */
    uint8_t* payload;          /**< 负载 */
    uint16_t payload_len;      /**< 负载长度 */
} coap_message_t;

/* CoAP客户端配置 */
typedef struct {
    char host[128];            /**< 主机地址 */
    uint16_t port;             /**< 端口 */
    bool use_dtls;             /**< 是否使用DTLS */
    uint16_t ack_timeout_ms;   /**< 确认超时(毫秒) */
    uint8_t max_retransmit;    /**< 最大重传次数 */
    bool non_confirmable;      /**< 是否使用不可靠消息 */
    struct {
        const char* psk;       /**< 预共享密钥 */
        uint16_t psk_len;      /**< 预共享密钥长度 */
        const char* identity;  /**< 身份 */
        uint16_t identity_len; /**< 身份长度 */
    } dtls;
} coap_client_config_t;

/* CoAP事件类型 */
typedef enum {
    COAP_EVENT_CONNECTED,      /**< 已连接 */
    COAP_EVENT_DISCONNECTED,   /**< 已断开连接 */
    COAP_EVENT_DATA,           /**< 收到数据 */
    COAP_EVENT_TIMEOUT,        /**< 超时 */
    COAP_EVENT_ERROR           /**< 错误 */
} coap_event_t;

/* CoAP事件回调函数类型 */
typedef void (*coap_event_callback_t)(coap_client_handle_t handle, coap_event_t event, coap_message_t* message, void* user_data);

/**
 * @brief 初始化CoAP客户端
 * 
 * @param config 配置参数
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @param handle 返回的客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_init(const coap_client_config_t* config, coap_event_callback_t callback, void* user_data, coap_client_handle_t* handle);

/**
 * @brief 释放CoAP客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_destroy(coap_client_handle_t handle);

/**
 * @brief 连接CoAP服务器
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_connect(coap_client_handle_t handle);

/**
 * @brief 断开CoAP连接
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_disconnect(coap_client_handle_t handle);

/**
 * @brief 创建CoAP消息
 * 
 * @param type 消息类型
 * @param method 方法码
 * @param message 返回的消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_create_message(coap_message_type_t type, coap_method_t method, coap_message_t* message);

/**
 * @brief 添加CoAP选项
 * 
 * @param message 消息
 * @param option 选项类型
 * @param data 选项数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_add_option(coap_message_t* message, coap_option_t option, const uint8_t* data, uint16_t len);

/**
 * @brief 添加URI路径选项
 * 
 * @param message 消息
 * @param path URI路径
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_add_uri_path(coap_message_t* message, const char* path);

/**
 * @brief 添加URI查询选项
 * 
 * @param message 消息
 * @param query URI查询
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_add_uri_query(coap_message_t* message, const char* query);

/**
 * @brief 设置CoAP消息负载
 * 
 * @param message 消息
 * @param payload 负载
 * @param len 负载长度
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_set_payload(coap_message_t* message, const uint8_t* payload, uint16_t len);

/**
 * @brief 发送CoAP消息
 * 
 * @param handle 客户端句柄
 * @param message 消息
 * @param token 返回的令牌
 * @param token_len 令牌长度
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_send(coap_client_handle_t handle, const coap_message_t* message, uint8_t* token, uint8_t* token_len);

/**
 * @brief 接收CoAP消息
 * 
 * @param handle 客户端句柄
 * @param message 返回的消息
 * @param timeout_ms 超时时间(毫秒)
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_receive(coap_client_handle_t handle, coap_message_t* message, uint32_t timeout_ms);

/**
 * @brief 订阅CoAP资源
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param token 返回的令牌
 * @param token_len 令牌长度
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_subscribe(coap_client_handle_t handle, const char* path, uint8_t* token, uint8_t* token_len);

/**
 * @brief 取消订阅CoAP资源
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param token 令牌
 * @param token_len 令牌长度
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_unsubscribe(coap_client_handle_t handle, const char* path, const uint8_t* token, uint8_t token_len);

/**
 * @brief 发送简单的GET请求
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param response 返回的响应消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_get(coap_client_handle_t handle, const char* path, coap_message_t* response);

/**
 * @brief 发送简单的POST请求
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param payload 负载
 * @param len 负载长度
 * @param format 内容格式
 * @param response 返回的响应消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_post(coap_client_handle_t handle, const char* path, const uint8_t* payload, uint16_t len, coap_content_format_t format, coap_message_t* response);

/**
 * @brief 发送简单的PUT请求
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param payload 负载
 * @param len 负载长度
 * @param format 内容格式
 * @param response 返回的响应消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_put(coap_client_handle_t handle, const char* path, const uint8_t* payload, uint16_t len, coap_content_format_t format, coap_message_t* response);

/**
 * @brief 发送简单的DELETE请求
 * 
 * @param handle 客户端句柄
 * @param path 资源路径
 * @param response 返回的响应消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_delete(coap_client_handle_t handle, const char* path, coap_message_t* response);

/**
 * @brief 释放CoAP消息资源
 * 
 * @param message 消息
 * @return int 成功返回0，失败返回错误码
 */
int coap_client_free_message(coap_message_t* message);

#ifdef __cplusplus
}
#endif

#endif /* COAP_CLIENT_API_H */
