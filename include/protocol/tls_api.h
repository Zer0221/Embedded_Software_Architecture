/**
 * @file tls_api.h
 * @brief TLS/SSL安全通信接口定义
 *
 * 该头文件定义了TLS/SSL安全通信的统一接口，提供证书管理、加密通信等功能
 */

#ifndef TLS_API_H
#define TLS_API_H

#include <stdint.h>
#include <stdbool.h>
#include "network_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TLS句柄 */
typedef void* tls_handle_t;

/* TLS版本 */
typedef enum {
    TLS_VER_SSL_3_0 = 0,   /**< SSL 3.0 */
    TLS_VER_TLS_1_0,       /**< TLS 1.0 */
    TLS_VER_TLS_1_1,       /**< TLS 1.1 */
    TLS_VER_TLS_1_2,       /**< TLS 1.2 */
    TLS_VER_TLS_1_3,       /**< TLS 1.3 */
    TLS_VER_DTLS_1_0,      /**< DTLS 1.0 */
    TLS_VER_DTLS_1_2,      /**< DTLS 1.2 */
    TLS_VER_DTLS_1_3       /**< DTLS 1.3 */
} tls_version_t;

/* TLS证书格式 */
typedef enum {
    TLS_CERT_FORMAT_PEM,   /**< PEM格式 */
    TLS_CERT_FORMAT_DER,   /**< DER格式 */
    TLS_CERT_FORMAT_RAW    /**< 原始格式 */
} tls_cert_format_t;

/* TLS密钥类型 */
typedef enum {
    TLS_KEY_TYPE_RSA,      /**< RSA密钥 */
    TLS_KEY_TYPE_ECC,      /**< ECC密钥 */
    TLS_KEY_TYPE_ED25519,  /**< ED25519密钥 */
    TLS_KEY_TYPE_PSK       /**< 预共享密钥 */
} tls_key_type_t;

/* TLS验证模式 */
typedef enum {
    TLS_VERIFY_NONE,       /**< 不验证 */
    TLS_VERIFY_OPTIONAL,   /**< 可选验证 */
    TLS_VERIFY_REQUIRED    /**< 必须验证 */
} tls_verify_mode_t;

/* TLS加密套件 */
typedef enum {
    TLS_CIPHER_AUTO,                     /**< 自动选择 */
    TLS_CIPHER_TLS_AES_128_GCM_SHA256,   /**< TLS_AES_128_GCM_SHA256 */
    TLS_CIPHER_TLS_AES_256_GCM_SHA384,   /**< TLS_AES_256_GCM_SHA384 */
    TLS_CIPHER_TLS_CHACHA20_POLY1305_SHA256, /**< TLS_CHACHA20_POLY1305_SHA256 */
    TLS_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, /**< TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 */
    TLS_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,   /**< TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 */
    TLS_CIPHER_TLS_PSK_WITH_AES_128_GCM_SHA256,         /**< TLS_PSK_WITH_AES_128_GCM_SHA256 */
    TLS_CIPHER_CUSTOM                    /**< 自定义加密套件 */
} tls_cipher_suite_t;

/* TLS配置 */
typedef struct {
    tls_version_t min_version;       /**< 最低TLS版本 */
    tls_version_t max_version;       /**< 最高TLS版本 */
    const char* ca_cert;             /**< CA证书 */
    uint32_t ca_cert_len;            /**< CA证书长度 */
    tls_cert_format_t ca_cert_format;/**< CA证书格式 */
    const char* client_cert;         /**< 客户端证书 */
    uint32_t client_cert_len;        /**< 客户端证书长度 */
    tls_cert_format_t client_cert_format; /**< 客户端证书格式 */
    const char* client_key;          /**< 客户端私钥 */
    uint32_t client_key_len;         /**< 客户端私钥长度 */
    tls_key_type_t client_key_type;  /**< 客户端私钥类型 */
    const char* client_key_password; /**< 客户端私钥密码 */
    tls_verify_mode_t verify_mode;   /**< 验证模式 */
    tls_cipher_suite_t cipher_suite; /**< 加密套件 */
    const char* custom_cipher_list;  /**< 自定义加密套件列表 */
    bool verify_host;                /**< 是否验证主机名 */
    char* hostname;                  /**< 主机名(用于SNI) */
    bool use_secure_element;         /**< 是否使用安全元件 */
    uint16_t timeout_ms;             /**< 超时时间(毫秒) */
    bool alpn_enabled;               /**< 是否启用ALPN */
    const char** alpn_protocols;     /**< ALPN协议列表 */
    uint8_t alpn_protocol_count;     /**< ALPN协议数量 */
    bool session_tickets_enabled;    /**< 是否启用会话票据 */
    bool renegotiation_enabled;      /**< 是否启用重新协商 */
} tls_config_t;

/**
 * @brief 初始化TLS引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int tls_init(void);

/**
 * @brief 反初始化TLS引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int tls_deinit(void);

/**
 * @brief 创建TLS会话
 * 
 * @param config TLS配置
 * @param handle 返回的TLS句柄
 * @return int 成功返回0，失败返回错误码
 */
int tls_create(const tls_config_t* config, tls_handle_t* handle);

/**
 * @brief 释放TLS会话
 * 
 * @param handle TLS句柄
 * @return int 成功返回0，失败返回错误码
 */
int tls_destroy(tls_handle_t handle);

/**
 * @brief 连接TLS服务器
 * 
 * @param handle TLS句柄
 * @param hostname 主机名
 * @param port 端口
 * @return int 成功返回0，失败返回错误码
 */
int tls_connect(tls_handle_t handle, const char* hostname, uint16_t port);

/**
 * @brief 基于已有的网络连接创建TLS连接
 * 
 * @param handle TLS句柄
 * @param net_handle 网络句柄
 * @return int 成功返回0，失败返回错误码
 */
int tls_connect_over(tls_handle_t handle, network_handle_t net_handle);

/**
 * @brief 断开TLS连接
 * 
 * @param handle TLS句柄
 * @return int 成功返回0，失败返回错误码
 */
int tls_disconnect(tls_handle_t handle);

/**
 * @brief 通过TLS发送数据
 * 
 * @param handle TLS句柄
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param sent_len 返回已发送长度
 * @return int 成功返回0，失败返回错误码
 */
int tls_send(tls_handle_t handle, const void* data, size_t len, size_t* sent_len);

/**
 * @brief 通过TLS接收数据
 * 
 * @param handle TLS句柄
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param received_len 返回已接收长度
 * @return int 成功返回0，失败返回错误码
 */
int tls_receive(tls_handle_t handle, void* data, size_t len, size_t* received_len);

/**
 * @brief 获取TLS连接状态
 * 
 * @param handle TLS句柄
 * @param connected 返回连接状态
 * @return int 成功返回0，失败返回错误码
 */
int tls_is_connected(tls_handle_t handle, bool* connected);

/**
 * @brief 设置TLS超时时间
 * 
 * @param handle TLS句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return int 成功返回0，失败返回错误码
 */
int tls_set_timeout(tls_handle_t handle, uint16_t timeout_ms);

/**
 * @brief 获取TLS会话信息
 * 
 * @param handle TLS句柄
 * @param version 返回TLS版本
 * @param cipher 返回加密套件
 * @param cipher_len 加密套件缓冲区长度
 * @return int 成功返回0，失败返回错误码
 */
int tls_get_session_info(tls_handle_t handle, tls_version_t* version, char* cipher, size_t cipher_len);

/**
 * @brief 验证证书
 * 
 * @param cert 证书数据
 * @param cert_len 证书长度
 * @param format 证书格式
 * @param ca_cert CA证书数据
 * @param ca_cert_len CA证书长度
 * @param ca_format CA证书格式
 * @return int 成功返回0，失败返回错误码
 */
int tls_verify_certificate(const void* cert, size_t cert_len, tls_cert_format_t format,
                         const void* ca_cert, size_t ca_cert_len, tls_cert_format_t ca_format);

/**
 * @brief 添加CA证书到全局存储
 * 
 * @param cert 证书数据
 * @param cert_len 证书长度
 * @param format 证书格式
 * @return int 成功返回0，失败返回错误码
 */
int tls_add_global_ca(const void* cert, size_t cert_len, tls_cert_format_t format);

/**
 * @brief 清空全局CA证书存储
 * 
 * @return int 成功返回0，失败返回错误码
 */
int tls_clear_global_ca(void);

/**
 * @brief 设置PSK (预共享密钥)
 * 
 * @param handle TLS句柄
 * @param psk 预共享密钥
 * @param psk_len 预共享密钥长度
 * @param identity 身份
 * @param identity_len 身份长度
 * @return int 成功返回0，失败返回错误码
 */
int tls_set_psk(tls_handle_t handle, const uint8_t* psk, size_t psk_len, 
              const char* identity, size_t identity_len);

/**
 * @brief 生成随机数
 * 
 * @param buf 缓冲区
 * @param len 长度
 * @return int 成功返回0，失败返回错误码
 */
int tls_random(void* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* TLS_API_H */
