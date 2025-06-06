/**
 * @file https_server_api.h
 * @brief HTTPS服务器接口抽象层定义
 *
 * 该头文件定义了HTTPS服务器的统一抽象接口，使上层应用能够提供安全的Web服务
 */

#ifndef HTTPS_SERVER_API_H
#define HTTPS_SERVER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "network_api.h"
#include "tls_api.h"

/* HTTPS服务器句柄 */
typedef driver_handle_t https_server_handle_t;

/* HTTP方法类型 */
typedef enum {
    HTTP_METHOD_GET,      /**< GET方法 */
    HTTP_METHOD_POST,     /**< POST方法 */
    HTTP_METHOD_PUT,      /**< PUT方法 */
    HTTP_METHOD_DELETE,   /**< DELETE方法 */
    HTTP_METHOD_HEAD,     /**< HEAD方法 */
    HTTP_METHOD_OPTIONS,  /**< OPTIONS方法 */
    HTTP_METHOD_PATCH     /**< PATCH方法 */
} http_method_t;

/* HTTP响应状态码 */
typedef enum {
    HTTP_STATUS_OK                    = 200,  /**< 200 OK */
    HTTP_STATUS_CREATED               = 201,  /**< 201 Created */
    HTTP_STATUS_ACCEPTED              = 202,  /**< 202 Accepted */
    HTTP_STATUS_NO_CONTENT            = 204,  /**< 204 No Content */
    HTTP_STATUS_MOVED_PERMANENTLY     = 301,  /**< 301 Moved Permanently */
    HTTP_STATUS_FOUND                 = 302,  /**< 302 Found */
    HTTP_STATUS_SEE_OTHER             = 303,  /**< 303 See Other */
    HTTP_STATUS_NOT_MODIFIED          = 304,  /**< 304 Not Modified */
    HTTP_STATUS_BAD_REQUEST           = 400,  /**< 400 Bad Request */
    HTTP_STATUS_UNAUTHORIZED          = 401,  /**< 401 Unauthorized */
    HTTP_STATUS_FORBIDDEN             = 403,  /**< 403 Forbidden */
    HTTP_STATUS_NOT_FOUND             = 404,  /**< 404 Not Found */
    HTTP_STATUS_METHOD_NOT_ALLOWED    = 405,  /**< 405 Method Not Allowed */
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,  /**< 500 Internal Server Error */
    HTTP_STATUS_NOT_IMPLEMENTED       = 501,  /**< 501 Not Implemented */
    HTTP_STATUS_BAD_GATEWAY           = 502,  /**< 502 Bad Gateway */
    HTTP_STATUS_SERVICE_UNAVAILABLE   = 503   /**< 503 Service Unavailable */
} http_status_t;

/* HTTP请求结构体 */
typedef struct {
    http_method_t method;         /**< HTTP方法 */
    char *uri;                    /**< 请求URI */
    char *version;                /**< HTTP版本 */
    char *query;                  /**< 查询字符串 */
    char *body;                   /**< 请求体 */
    uint32_t body_length;         /**< 请求体长度 */
    void *headers;                /**< 请求头 */
    uint16_t header_count;        /**< 请求头数量 */
} http_request_t;

/* HTTP响应结构体 */
typedef struct {
    http_status_t status_code;    /**< 状态码 */
    char *content_type;           /**< 内容类型 */
    char *body;                   /**< 响应体 */
    uint32_t body_length;         /**< 响应体长度 */
    void *headers;                /**< 响应头 */
    uint16_t header_count;        /**< 响应头数量 */
} http_response_t;

/* HTTPS服务器配置参数 */
typedef struct {
    uint16_t port;                /**< 服务器端口 */
    uint16_t max_connections;     /**< 最大连接数 */
    uint16_t timeout_ms;          /**< 连接超时时间(毫秒) */
    uint16_t max_request_size;    /**< 最大请求大小 */
    tls_config_t *tls_config;     /**< TLS配置 */
} https_server_config_t;

/* URI处理回调函数类型 */
typedef int (*https_uri_handler_t)(http_request_t *request, http_response_t *response, void *user_data);

/**
 * @brief 初始化HTTPS服务器
 * 
 * @param config 服务器配置参数
 * @param handle 服务器句柄指针
 * @return int 0表示成功，非0表示失败
 */
int https_server_init(const https_server_config_t *config, https_server_handle_t *handle);

/**
 * @brief 启动HTTPS服务器
 * 
 * @param handle 服务器句柄
 * @return int 0表示成功，非0表示失败
 */
int https_server_start(https_server_handle_t handle);

/**
 * @brief 停止HTTPS服务器
 * 
 * @param handle 服务器句柄
 * @return int 0表示成功，非0表示失败
 */
int https_server_stop(https_server_handle_t handle);

/**
 * @brief 注册URI处理函数
 * 
 * @param handle 服务器句柄
 * @param uri URI字符串
 * @param method HTTP方法
 * @param handler 处理函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int https_server_register_uri_handler(https_server_handle_t handle, 
                                     const char *uri, 
                                     http_method_t method, 
                                     https_uri_handler_t handler, 
                                     void *user_data);

/**
 * @brief 取消注册URI处理函数
 * 
 * @param handle 服务器句柄
 * @param uri URI字符串
 * @param method HTTP方法
 * @return int 0表示成功，非0表示失败
 */
int https_server_unregister_uri_handler(https_server_handle_t handle, 
                                       const char *uri, 
                                       http_method_t method);

/**
 * @brief 设置基础身份验证
 * 
 * @param handle 服务器句柄
 * @param username 用户名
 * @param password 密码
 * @return int 0表示成功，非0表示失败
 */
int https_server_set_basic_auth(https_server_handle_t handle, 
                               const char *username, 
                               const char *password);

/**
 * @brief 发送HTTP响应
 * 
 * @param request 请求结构体指针
 * @param response 响应结构体指针
 * @return int 0表示成功，非0表示失败
 */
int https_server_send_response(http_request_t *request, http_response_t *response);

/**
 * @brief 获取HTTP请求头
 * 
 * @param request 请求结构体指针
 * @param header_name 头名称
 * @param value 值缓冲区
 * @param value_len 缓冲区长度
 * @return int 0表示成功，非0表示失败
 */
int https_server_get_header(http_request_t *request, 
                           const char *header_name, 
                           char *value, 
                           size_t value_len);

/**
 * @brief 添加HTTP响应头
 * 
 * @param response 响应结构体指针
 * @param header_name 头名称
 * @param header_value 头值
 * @return int 0表示成功，非0表示失败
 */
int https_server_add_header(http_response_t *response, 
                           const char *header_name, 
                           const char *header_value);

/**
 * @brief 解析HTTP请求查询参数
 * 
 * @param request 请求结构体指针
 * @param param_name 参数名
 * @param value 值缓冲区
 * @param value_len 缓冲区长度
 * @return int 0表示成功，非0表示失败
 */
int https_server_get_query_param(http_request_t *request, 
                                const char *param_name, 
                                char *value, 
                                size_t value_len);

#endif /* HTTPS_SERVER_API_H */
