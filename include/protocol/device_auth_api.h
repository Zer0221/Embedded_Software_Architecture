/**
 * @file device_auth_api.h
 * @brief 设备认证和激活接口抽象层定义
 *
 * 该头文件定义了设备认证和激活的统一抽象接口，包括设备SN管理、证书管理、JWT验证等功能
 */

#ifndef DEVICE_AUTH_API_H
#define DEVICE_AUTH_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "tls_api.h"

/* 设备认证句柄 */
typedef driver_handle_t device_auth_handle_t;

/* 设备状态枚举 */
typedef enum {
    DEVICE_STATE_UNACTIVATED,     /**< 未激活 */
    DEVICE_STATE_ACTIVATING,      /**< 激活中 */
    DEVICE_STATE_ACTIVATED,       /**< 已激活 */
    DEVICE_STATE_REVOKED,         /**< 已撤销 */
    DEVICE_STATE_ERROR            /**< 错误状态 */
} device_state_t;

/* JWT令牌结构 */
typedef struct {
    char *token;                  /**< 令牌字符串 */
    uint32_t expiry;              /**< 过期时间戳 */
    char *issuer;                 /**< 签发者 */
    char *subject;                /**< 主题 */
    char *audience;               /**< 受众 */
} jwt_token_t;

/* 设备信息结构 */
typedef struct {
    char serial_number[32];       /**< 设备序列号 */
    char model[32];               /**< 设备型号 */
    char firmware_version[32];    /**< 固件版本 */
    char hardware_version[32];    /**< 硬件版本 */
    device_state_t state;         /**< 设备状态 */
    uint32_t activation_time;     /**< 激活时间 */
    char activation_code[64];     /**< 激活码 */
} device_info_t;

/* 设备认证配置参数 */
typedef struct {
    bool use_secure_storage;      /**< 是否使用安全存储 */
    char *cert_path;              /**< 证书存储路径 */
    char *key_path;               /**< 密钥存储路径 */
    char *ca_path;                /**< CA证书路径 */
    char *auth_server_url;        /**< 认证服务器URL */
    uint16_t timeout_ms;          /**< 认证超时时间(毫秒) */
} device_auth_config_t;

/**
 * @brief 初始化设备认证模块
 * 
 * @param config 认证配置参数
 * @param handle 认证句柄指针
 * @return int 0表示成功，非0表示失败
 */
int device_auth_init(const device_auth_config_t *config, device_auth_handle_t *handle);

/**
 * @brief 获取设备信息
 * 
 * @param handle 认证句柄
 * @param info 设备信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int device_auth_get_device_info(device_auth_handle_t handle, device_info_t *info);

/**
 * @brief 设置设备序列号
 * 
 * @param handle 认证句柄
 * @param serial_number 序列号字符串
 * @return int 0表示成功，非0表示失败
 */
int device_auth_set_serial_number(device_auth_handle_t handle, const char *serial_number);

/**
 * @brief 生成设备公私钥对
 * 
 * @param handle 认证句柄
 * @param key_type 密钥类型
 * @param key_size 密钥大小
 * @return int 0表示成功，非0表示失败
 */
int device_auth_generate_key_pair(device_auth_handle_t handle, 
                                 tls_key_type_t key_type, 
                                 uint16_t key_size);

/**
 * @brief 生成证书签名请求(CSR)
 * 
 * @param handle 认证句柄
 * @param csr_buffer CSR缓冲区
 * @param buffer_size 缓冲区大小
 * @param actual_size 实际生成的CSR大小
 * @return int 0表示成功，非0表示失败
 */
int device_auth_generate_csr(device_auth_handle_t handle, 
                            char *csr_buffer, 
                            size_t buffer_size, 
                            size_t *actual_size);

/**
 * @brief 导入设备证书
 * 
 * @param handle 认证句柄
 * @param cert_data 证书数据
 * @param cert_len 证书长度
 * @return int 0表示成功，非0表示失败
 */
int device_auth_import_certificate(device_auth_handle_t handle, 
                                  const char *cert_data, 
                                  size_t cert_len);

/**
 * @brief 激活设备
 * 
 * @param handle 认证句柄
 * @param activation_code 激活码
 * @return int 0表示成功，非0表示失败
 */
int device_auth_activate(device_auth_handle_t handle, const char *activation_code);

/**
 * @brief 验证JWT令牌
 * 
 * @param handle 认证句柄
 * @param token 令牌字符串
 * @param token_info 令牌信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int device_auth_verify_jwt(device_auth_handle_t handle, 
                          const char *token, 
                          jwt_token_t *token_info);

/**
 * @brief 生成JWT令牌
 * 
 * @param handle 认证句柄
 * @param token_info 令牌信息结构体指针
 * @param token_buffer 令牌缓冲区
 * @param buffer_size 缓冲区大小
 * @param actual_size 实际生成的令牌大小
 * @return int 0表示成功，非0表示失败
 */
int device_auth_generate_jwt(device_auth_handle_t handle, 
                            const jwt_token_t *token_info, 
                            char *token_buffer, 
                            size_t buffer_size, 
                            size_t *actual_size);

/**
 * @brief 检查设备激活状态
 * 
 * @param handle 认证句柄
 * @param state 设备状态指针
 * @return int 0表示成功，非0表示失败
 */
int device_auth_check_state(device_auth_handle_t handle, device_state_t *state);

/**
 * @brief 撤销设备激活
 * 
 * @param handle 认证句柄
 * @return int 0表示成功，非0表示失败
 */
int device_auth_revoke(device_auth_handle_t handle);

#endif /* DEVICE_AUTH_API_H */
