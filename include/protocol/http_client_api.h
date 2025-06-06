/**
 * @file http_client_api.h
 * @brief HTTP/HTTPS客户端接口定义
 *
 * 该头文件定义了HTTP/HTTPS客户端的统一接口，支持常见的HTTP方法、TLS加密和HTTP头部管理
 */

#ifndef HTTP_CLIENT_API_H
#define HTTP_CLIENT_API_H

#include <stdint.h>
#include <stdbool.h>
#include "network_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HTTP方法 */
typedef enum {
    HTTP_METHOD_GET,           /**< GET方法 */
    HTTP_METHOD_POST,          /**< POST方法 */
    HTTP_METHOD_PUT,           /**< PUT方法 */
    HTTP_METHOD_DELETE,        /**< DELETE方法 */
    HTTP_METHOD_HEAD,          /**< HEAD方法 */
    HTTP_METHOD_PATCH,         /**< PATCH方法 */
    HTTP_METHOD_OPTIONS        /**< OPTIONS方法 */
} http_method_t;

/* HTTP内容类型 */
typedef enum {
    HTTP_CONTENT_TYPE_NONE,                       /**< 无内容类型 */
    HTTP_CONTENT_TYPE_TEXT_PLAIN,                 /**< text/plain */
    HTTP_CONTENT_TYPE_TEXT_HTML,                  /**< text/html */
    HTTP_CONTENT_TYPE_APPLICATION_JSON,           /**< application/json */
    HTTP_CONTENT_TYPE_APPLICATION_XML,            /**< application/xml */
    HTTP_CONTENT_TYPE_APPLICATION_FORM,           /**< application/x-www-form-urlencoded */
    HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM,   /**< application/octet-stream */
    HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA,        /**< multipart/form-data */
    HTTP_CONTENT_TYPE_IMAGE_JPEG,                 /**< image/jpeg */
    HTTP_CONTENT_TYPE_IMAGE_PNG,                  /**< image/png */
    HTTP_CONTENT_TYPE_CUSTOM                      /**< 自定义内容类型 */
} http_content_type_t;

/* HTTP客户端句柄 */
typedef void* http_client_handle_t;

/* HTTP头部结构 */
typedef struct {
    char key[64];              /**< 头部字段名 */
    char value[256];           /**< 头部字段值 */
} http_header_t;

/* HTTP授权类型 */
typedef enum {
    HTTP_AUTH_TYPE_NONE,       /**< 无授权 */
    HTTP_AUTH_TYPE_BASIC,      /**< 基本授权 */
    HTTP_AUTH_TYPE_BEARER,     /**< Bearer授权 */
    HTTP_AUTH_TYPE_DIGEST,     /**< 摘要授权 */
    HTTP_AUTH_TYPE_CUSTOM      /**< 自定义授权 */
} http_auth_type_t;

/* HTTP授权信息 */
typedef struct {
    http_auth_type_t type;     /**< 授权类型 */
    union {
        struct {
            char username[64]; /**< 用户名 */
            char password[64]; /**< 密码 */
        } basic;
        struct {
            char token[256];   /**< 令牌 */
        } bearer;
        struct {
            char digest[512];  /**< 摘要授权信息 */
        } digest;
        struct {
            char data[512];    /**< 自定义授权数据 */
        } custom;
    } credentials;
} http_auth_info_t;

/* SSL/TLS验证级别 */
typedef enum {
    HTTP_SSL_VERIFY_NONE,      /**< 不验证 */
    HTTP_SSL_VERIFY_OPTIONAL,  /**< 可选验证 */
    HTTP_SSL_VERIFY_REQUIRED   /**< 必须验证 */
} http_ssl_verify_t;

/* HTTP客户端配置 */
typedef struct {
    bool use_global_ca_store;   /**< 是否使用全局CA存储 */
    const char* cert_pem;       /**< 客户端证书（PEM格式） */
    const char* client_key_pem; /**< 客户端私钥（PEM格式） */
    const char* ca_cert_pem;    /**< CA证书（PEM格式） */
    http_ssl_verify_t ssl_verify;/**< SSL验证级别 */
    uint16_t timeout_ms;        /**< 超时时间(毫秒) */
    bool keep_alive;            /**< 是否保持连接 */
    uint8_t max_redirection;    /**< 最大重定向次数 */
    bool auto_redirect;         /**< 是否自动重定向 */
    bool use_proxy;             /**< 是否使用代理 */
    struct {
        char host[128];         /**< 代理主机 */
        uint16_t port;          /**< 代理端口 */
        char username[64];      /**< 代理用户名 */
        char password[64];      /**< 代理密码 */
    } proxy;
    uint8_t max_retries;        /**< 最大重试次数 */
    char user_agent[128];       /**< 用户代理 */
} http_client_config_t;

/* HTTP响应 */
typedef struct {
    uint16_t status_code;       /**< 状态码 */
    char* content;              /**< 响应内容 */
    uint32_t content_length;    /**< 内容长度 */
    http_header_t* headers;     /**< 响应头部 */
    uint16_t header_count;      /**< 头部数量 */
    bool is_chunked;            /**< 是否分块传输 */
} http_response_t;

/* HTTP事件类型 */
typedef enum {
    HTTP_EVENT_ON_CONNECTED,    /**< 已连接 */
    HTTP_EVENT_HEADERS_SENT,    /**< 头部已发送 */
    HTTP_EVENT_ON_HEADER,       /**< 接收到头部 */
    HTTP_EVENT_ON_DATA,         /**< 接收到数据 */
    HTTP_EVENT_ON_FINISH,       /**< 完成 */
    HTTP_EVENT_DISCONNECTED,    /**< 已断开连接 */
    HTTP_EVENT_ERROR            /**< 错误 */
} http_event_t;

/* HTTP事件处理回调 */
typedef int (*http_event_handler_t)(http_client_handle_t client, http_event_t event, void* data, int data_len, void* user_data);

/**
 * @brief 初始化HTTP客户端
 * 
 * @param config 配置参数
 * @param handle 返回的客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int http_client_init(const http_client_config_t* config, http_client_handle_t* handle);

/**
 * @brief 释放HTTP客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int http_client_cleanup(http_client_handle_t handle);

/**
 * @brief 设置HTTP请求URL
 * 
 * @param handle 客户端句柄
 * @param url URL地址
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_url(http_client_handle_t handle, const char* url);

/**
 * @brief 设置HTTP请求方法
 * 
 * @param handle 客户端句柄
 * @param method 请求方法
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_method(http_client_handle_t handle, http_method_t method);

/**
 * @brief 设置HTTP请求头部
 * 
 * @param handle 客户端句柄
 * @param key 头部字段名
 * @param value 头部字段值
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_header(http_client_handle_t handle, const char* key, const char* value);

/**
 * @brief 设置HTTP授权信息
 * 
 * @param handle 客户端句柄
 * @param auth_info 授权信息
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_auth(http_client_handle_t handle, const http_auth_info_t* auth_info);

/**
 * @brief 设置HTTP请求内容类型
 * 
 * @param handle 客户端句柄
 * @param content_type 内容类型
 * @param custom_type 自定义类型字符串(当content_type为HTTP_CONTENT_TYPE_CUSTOM时有效)
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_content_type(http_client_handle_t handle, http_content_type_t content_type, const char* custom_type);

/**
 * @brief 设置HTTP请求超时时间
 * 
 * @param handle 客户端句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_timeout(http_client_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 设置HTTP请求正文
 * 
 * @param handle 客户端句柄
 * @param data 数据
 * @param len 数据长度
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_post_data(http_client_handle_t handle, const void* data, size_t len);

/**
 * @brief 设置HTTP事件处理回调
 * 
 * @param handle 客户端句柄
 * @param event_handler 事件处理回调
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int http_client_set_event_handler(http_client_handle_t handle, http_event_handler_t event_handler, void* user_data);

/**
 * @brief 执行HTTP请求
 * 
 * @param handle 客户端句柄
 * @param response 响应
 * @return int 成功返回0，失败返回错误码
 */
int http_client_perform(http_client_handle_t handle, http_response_t* response);

/**
 * @brief 重置HTTP客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int http_client_reset(http_client_handle_t handle);

/**
 * @brief 释放HTTP响应资源
 * 
 * @param response 响应
 * @return int 成功返回0，失败返回错误码
 */
int http_client_free_response(http_response_t* response);

/**
 * @brief HTTP GET简化接口
 * 
 * @param url URL地址
 * @param response 响应
 * @param config 配置参数，NULL表示使用默认配置
 * @return int 成功返回0，失败返回错误码
 */
int http_client_get(const char* url, http_response_t* response, const http_client_config_t* config);

/**
 * @brief HTTP POST简化接口
 * 
 * @param url URL地址
 * @param data 数据
 * @param len 数据长度
 * @param content_type 内容类型
 * @param response 响应
 * @param config 配置参数，NULL表示使用默认配置
 * @return int 成功返回0，失败返回错误码
 */
int http_client_post(const char* url, const void* data, size_t len, http_content_type_t content_type, 
                     http_response_t* response, const http_client_config_t* config);

/**
 * @brief 下载文件
 * 
 * @param url URL地址
 * @param path 保存路径
 * @param progress_cb 进度回调
 * @param user_data 用户数据
 * @param config 配置参数，NULL表示使用默认配置
 * @return int 成功返回0，失败返回错误码
 */
int http_client_download(const char* url, const char* path, 
                         void (*progress_cb)(size_t downloaded, size_t total, void* user_data), 
                         void* user_data, const http_client_config_t* config);

/**
 * @brief 上传文件
 * 
 * @param url URL地址
 * @param path 文件路径
 * @param field_name 表单字段名
 * @param progress_cb 进度回调
 * @param user_data 用户数据
 * @param response 响应
 * @param config 配置参数，NULL表示使用默认配置
 * @return int 成功返回0，失败返回错误码
 */
int http_client_upload(const char* url, const char* path, const char* field_name,
                       void (*progress_cb)(size_t uploaded, size_t total, void* user_data),
                       void* user_data, http_response_t* response, const http_client_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_CLIENT_API_H */
