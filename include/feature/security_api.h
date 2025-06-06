/**
 * @file security_api.h
 * @brief 安全管理接口定义
 *
 * 该头文件定义了安全相关的统一接口，包括加密、认证等
 */

#ifndef SECURITY_API_H
#define SECURITY_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"

/* 安全操作状态 */
typedef enum {
    SECURITY_STATUS_IDLE,       /**< 空闲状态 */
    SECURITY_STATUS_BUSY,       /**< 忙状态 */
    SECURITY_STATUS_COMPLETE,   /**< 操作完成 */
    SECURITY_STATUS_ERROR,      /**< 操作错误 */
    SECURITY_STATUS_TIMEOUT     /**< 操作超时 */
} security_status_t;

/* 加密算法类型 */
typedef enum {
    SECURITY_ALGO_NONE = 0,     /**< 无加密 */
    SECURITY_ALGO_AES,          /**< AES加密 */
    SECURITY_ALGO_DES,          /**< DES加密 */
    SECURITY_ALGO_3DES,         /**< 3DES加密 */
    SECURITY_ALGO_RSA,          /**< RSA加密 */
    SECURITY_ALGO_ECC,          /**< ECC加密 */
    SECURITY_ALGO_CHACHA20,     /**< ChaCha20加密 */
    SECURITY_ALGO_CUSTOM        /**< 自定义加密算法 */
} security_algo_t;

/* 哈希算法类型 */
typedef enum {
    SECURITY_HASH_NONE = 0,     /**< 无哈希 */
    SECURITY_HASH_MD5,          /**< MD5哈希 */
    SECURITY_HASH_SHA1,         /**< SHA1哈希 */
    SECURITY_HASH_SHA224,       /**< SHA224哈希 */
    SECURITY_HASH_SHA256,       /**< SHA256哈希 */
    SECURITY_HASH_SHA384,       /**< SHA384哈希 */
    SECURITY_HASH_SHA512,       /**< SHA512哈希 */
    SECURITY_HASH_CRC32,        /**< CRC32校验 */
    SECURITY_HASH_CUSTOM        /**< 自定义哈希算法 */
} security_hash_t;

/* 密钥类型 */
typedef enum {
    SECURITY_KEY_SYMMETRIC,     /**< 对称密钥 */
    SECURITY_KEY_PRIVATE,       /**< 私钥 */
    SECURITY_KEY_PUBLIC,        /**< 公钥 */
    SECURITY_KEY_CERTIFICATE    /**< 证书 */
} security_key_type_t;

/* 加密模式 */
typedef enum {
    SECURITY_MODE_ECB,          /**< ECB模式 */
    SECURITY_MODE_CBC,          /**< CBC模式 */
    SECURITY_MODE_CTR,          /**< CTR模式 */
    SECURITY_MODE_GCM,          /**< GCM模式 */
    SECURITY_MODE_CCM           /**< CCM模式 */
} security_mode_t;

/* 安全回调函数类型 */
typedef void (*security_callback_t)(void *arg, security_status_t status);

/* 安全设备句柄 */
typedef driver_handle_t security_handle_t;

/**
 * @brief 安全操作配置
 */
typedef struct {
    security_algo_t algo;       /**< 加密算法 */
    security_mode_t mode;       /**< 加密模式 */
    uint16_t key_size;          /**< 密钥大小（位） */
    uint8_t *iv;                /**< 初始化向量 */
    uint16_t iv_len;            /**< 初始化向量长度 */
    uint8_t *aad;               /**< 附加认证数据 */
    uint16_t aad_len;           /**< 附加认证数据长度 */
} security_config_t;

/**
 * @brief 固件验证结果
 */
typedef enum {
    FIRMWARE_VALID,             /**< 固件有效 */
    FIRMWARE_INVALID_SIGNATURE, /**< 签名无效 */
    FIRMWARE_CORRUPTED,         /**< 固件损坏 */
    FIRMWARE_VERSION_ERROR,     /**< 版本错误 */
    FIRMWARE_PLATFORM_MISMATCH  /**< 平台不匹配 */
} firmware_validation_result_t;

/**
 * @brief 初始化安全模块
 * 
 * @param callback 安全操作完成回调函数
 * @param arg 传递给回调函数的参数
 * @param handle 安全设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int security_init(security_callback_t callback, void *arg, security_handle_t *handle);

/**
 * @brief 去初始化安全模块
 * 
 * @param handle 安全设备句柄
 * @return int 0表示成功，非0表示失败
 */
int security_deinit(security_handle_t handle);

/**
 * @brief 生成随机数
 * 
 * @param handle 安全设备句柄
 * @param buffer 随机数缓冲区
 * @param len 随机数长度
 * @return int 0表示成功，非0表示失败
 */
int security_generate_random(security_handle_t handle, uint8_t *buffer, uint32_t len);

/**
 * @brief 生成密钥
 * 
 * @param handle 安全设备句柄
 * @param key_type 密钥类型
 * @param algo 算法类型
 * @param key_size 密钥大小（位）
 * @param key_buffer 密钥缓冲区
 * @param key_len 密钥缓冲区长度
 * @return int 0表示成功，非0表示失败
 */
int security_generate_key(security_handle_t handle, security_key_type_t key_type, 
                         security_algo_t algo, uint16_t key_size, 
                         uint8_t *key_buffer, uint32_t *key_len);

/**
 * @brief 导入密钥
 * 
 * @param handle 安全设备句柄
 * @param key_type 密钥类型
 * @param key_id 密钥ID
 * @param key_data 密钥数据
 * @param key_len 密钥长度
 * @return int 0表示成功，非0表示失败
 */
int security_import_key(security_handle_t handle, security_key_type_t key_type, 
                       uint32_t key_id, const uint8_t *key_data, uint32_t key_len);

/**
 * @brief 导出密钥
 * 
 * @param handle 安全设备句柄
 * @param key_id 密钥ID
 * @param key_data 密钥数据缓冲区
 * @param key_len 输入为缓冲区大小，输出为实际密钥长度
 * @return int 0表示成功，非0表示失败
 */
int security_export_key(security_handle_t handle, uint32_t key_id, 
                       uint8_t *key_data, uint32_t *key_len);

/**
 * @brief 删除密钥
 * 
 * @param handle 安全设备句柄
 * @param key_id 密钥ID
 * @return int 0表示成功，非0表示失败
 */
int security_delete_key(security_handle_t handle, uint32_t key_id);

/**
 * @brief 加密数据
 * 
 * @param handle 安全设备句柄
 * @param config 加密配置
 * @param key_id 密钥ID
 * @param input 明文数据
 * @param input_len 明文长度
 * @param output 密文缓冲区
 * @param output_len 输入为缓冲区大小，输出为实际密文长度
 * @return int 0表示成功，非0表示失败
 */
int security_encrypt(security_handle_t handle, const security_config_t *config, 
                    uint32_t key_id, const uint8_t *input, uint32_t input_len,
                    uint8_t *output, uint32_t *output_len);

/**
 * @brief 解密数据
 * 
 * @param handle 安全设备句柄
 * @param config 解密配置
 * @param key_id 密钥ID
 * @param input 密文数据
 * @param input_len 密文长度
 * @param output 明文缓冲区
 * @param output_len 输入为缓冲区大小，输出为实际明文长度
 * @return int 0表示成功，非0表示失败
 */
int security_decrypt(security_handle_t handle, const security_config_t *config, 
                    uint32_t key_id, const uint8_t *input, uint32_t input_len,
                    uint8_t *output, uint32_t *output_len);

/**
 * @brief 计算哈希值
 * 
 * @param handle 安全设备句柄
 * @param hash_type 哈希算法类型
 * @param input 输入数据
 * @param input_len 输入数据长度
 * @param output 哈希值缓冲区
 * @param output_len 输入为缓冲区大小，输出为实际哈希值长度
 * @return int 0表示成功，非0表示失败
 */
int security_hash(security_handle_t handle, security_hash_t hash_type,
                 const uint8_t *input, uint32_t input_len,
                 uint8_t *output, uint32_t *output_len);

/**
 * @brief 数字签名
 * 
 * @param handle 安全设备句柄
 * @param key_id 私钥ID
 * @param input 待签名数据
 * @param input_len 待签名数据长度
 * @param signature 签名缓冲区
 * @param sig_len 输入为缓冲区大小，输出为实际签名长度
 * @return int 0表示成功，非0表示失败
 */
int security_sign(security_handle_t handle, uint32_t key_id,
                 const uint8_t *input, uint32_t input_len,
                 uint8_t *signature, uint32_t *sig_len);

/**
 * @brief 验证签名
 * 
 * @param handle 安全设备句柄
 * @param key_id 公钥ID
 * @param input 原始数据
 * @param input_len 原始数据长度
 * @param signature 签名数据
 * @param sig_len 签名数据长度
 * @return int 0表示签名有效，非0表示签名无效
 */
int security_verify(security_handle_t handle, uint32_t key_id,
                   const uint8_t *input, uint32_t input_len,
                   const uint8_t *signature, uint32_t sig_len);

/**
 * @brief 安全启动验证
 * 
 * 验证当前运行的固件是否合法
 * 
 * @param handle 安全设备句柄
 * @return int 0表示验证通过，非0表示验证失败
 */
int security_secure_boot_verify(security_handle_t handle);

/**
 * @brief 固件更新验证
 * 
 * 验证待更新的固件是否合法
 * 
 * @param handle 安全设备句柄
 * @param firmware_addr 固件地址
 * @param firmware_size 固件大小
 * @param signature 固件签名
 * @param sig_len 签名长度
 * @param result 验证结果
 * @return int 0表示操作成功，非0表示操作失败
 */
int security_verify_firmware(security_handle_t handle,
                            const uint8_t *firmware_addr, uint32_t firmware_size,
                            const uint8_t *signature, uint32_t sig_len,
                            firmware_validation_result_t *result);

/**
 * @brief 加密存储写入
 * 
 * 加密数据并写入安全存储区
 * 
 * @param handle 安全设备句柄
 * @param key 存储键名
 * @param data 数据
 * @param data_len 数据长度
 * @return int 0表示成功，非0表示失败
 */
int security_secure_storage_write(security_handle_t handle,
                                 const char *key,
                                 const uint8_t *data, uint32_t data_len);

/**
 * @brief 加密存储读取
 * 
 * 从安全存储区读取并解密数据
 * 
 * @param handle 安全设备句柄
 * @param key 存储键名
 * @param data 数据缓冲区
 * @param data_len 输入为缓冲区大小，输出为实际数据长度
 * @return int 0表示成功，非0表示失败
 */
int security_secure_storage_read(security_handle_t handle,
                                const char *key,
                                uint8_t *data, uint32_t *data_len);

/**
 * @brief 删除安全存储项
 * 
 * @param handle 安全设备句柄
 * @param key 存储键名
 * @return int 0表示成功，非0表示失败
 */
int security_secure_storage_delete(security_handle_t handle, const char *key);

/**
 * @brief 获取安全模块状态
 * 
 * @param handle 安全设备句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int security_get_status(security_handle_t handle, security_status_t *status);

/**
 * @brief 获取支持的算法
 * 
 * @param handle 安全设备句柄
 * @param algos 算法数组
 * @param count 输入为数组大小，输出为实际支持的算法数量
 * @return int 0表示成功，非0表示失败
 */
int security_get_supported_algorithms(security_handle_t handle, 
                                     security_algo_t *algos, uint32_t *count);

/**
 * @brief 获取支持的哈希算法
 * 
 * @param handle 安全设备句柄
 * @param hashes 哈希算法数组
 * @param count 输入为数组大小，输出为实际支持的算法数量
 * @return int 0表示成功，非0表示失败
 */
int security_get_supported_hashes(security_handle_t handle, 
                                 security_hash_t *hashes, uint32_t *count);

#endif /* SECURITY_API_H */
