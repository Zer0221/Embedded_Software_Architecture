/**
 * @file ota_api.h
 * @brief OTA (空中升级) 功能抽象接口
 *
 * 该头文件定义了OTA功能的统一接口，支持固件下载、验证、安装和回滚
 */

#ifndef OTA_API_H
#define OTA_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OTA句柄 */
typedef void* ota_handle_t;

/* OTA状态 */
typedef enum {
    OTA_STATE_IDLE,           /**< 空闲状态 */
    OTA_STATE_DOWNLOADING,    /**< 正在下载 */
    OTA_STATE_DOWNLOAD_DONE,  /**< 下载完成 */
    OTA_STATE_VERIFYING,      /**< 正在验证 */
    OTA_STATE_VERIFIED,       /**< 验证完成 */
    OTA_STATE_INSTALLING,     /**< 正在安装 */
    OTA_STATE_INSTALLED,      /**< 安装完成 */
    OTA_STATE_REBOOTING,      /**< 正在重启 */
    OTA_STATE_ROLLBACKING,    /**< 正在回滚 */
    OTA_STATE_ABORTED,        /**< 已终止 */
    OTA_STATE_ERROR           /**< 错误状态 */
} ota_state_t;

/* OTA错误码 */
typedef enum {
    OTA_ERR_NONE = 0,             /**< 无错误 */
    OTA_ERR_INVALID_PARAM,        /**< 无效参数 */
    OTA_ERR_NO_MEMORY,            /**< 内存不足 */
    OTA_ERR_FLASH_ERROR,          /**< Flash操作错误 */
    OTA_ERR_DOWNLOAD_FAILED,      /**< 下载失败 */
    OTA_ERR_VERIFICATION_FAILED,  /**< 验证失败 */
    OTA_ERR_INSTALLATION_FAILED,  /**< 安装失败 */
    OTA_ERR_ROLLBACK_FAILED,      /**< 回滚失败 */
    OTA_ERR_TIMEOUT,              /**< 超时 */
    OTA_ERR_SERVER_ERROR,         /**< 服务器错误 */
    OTA_ERR_NETWORK_ERROR,        /**< 网络错误 */
    OTA_ERR_INVALID_IMAGE,        /**< 无效镜像 */
    OTA_ERR_VERSION_DOWNGRADE,    /**< 版本降级 */
    OTA_ERR_NOT_SUPPORTED         /**< 不支持的操作 */
} ota_err_t;

/* OTA源类型 */
typedef enum {
    OTA_SOURCE_HTTP,          /**< HTTP/HTTPS源 */
    OTA_SOURCE_MQTT,          /**< MQTT源 */
    OTA_SOURCE_LOCAL,         /**< 本地文件源 */
    OTA_SOURCE_CUSTOM         /**< 自定义源 */
} ota_source_type_t;

/* OTA认证类型 */
typedef enum {
    OTA_AUTH_NONE,            /**< 无认证 */
    OTA_AUTH_BASIC,           /**< HTTP基本认证 */
    OTA_AUTH_TOKEN,           /**< 令牌认证 */
    OTA_AUTH_CERT             /**< 证书认证 */
} ota_auth_type_t;

/* OTA验证类型 */
typedef enum {
    OTA_VERIFY_NONE,          /**< 无验证 */
    OTA_VERIFY_MD5,           /**< MD5验证 */
    OTA_VERIFY_SHA1,          /**< SHA1验证 */
    OTA_VERIFY_SHA256,        /**< SHA256验证 */
    OTA_VERIFY_RSA,           /**< RSA签名验证 */
    OTA_VERIFY_ECDSA          /**< ECDSA签名验证 */
} ota_verify_type_t;

/* OTA认证信息 */
typedef struct {
    ota_auth_type_t type;     /**< 认证类型 */
    union {
        struct {
            char username[32];/**< 用户名 */
            char password[32];/**< 密码 */
        } basic;
        struct {
            char token[128];  /**< 令牌 */
        } token;
        struct {
            const char* cert; /**< 证书 */
            uint32_t cert_len;/**< 证书长度 */
            const char* key;  /**< 密钥 */
            uint32_t key_len; /**< 密钥长度 */
            const char* ca;   /**< CA证书 */
            uint32_t ca_len;  /**< CA证书长度 */
        } cert;
    } auth;
} ota_auth_info_t;

/* OTA配置 */
typedef struct {
    ota_source_type_t source_type;     /**< 源类型 */
    union {
        struct {
            char url[256];              /**< URL */
            ota_auth_info_t auth;       /**< 认证信息 */
            bool use_ssl;               /**< 是否使用SSL */
            uint16_t port;              /**< 端口 */
            uint16_t timeout_ms;        /**< 超时时间(毫秒) */
            uint16_t retry_count;       /**< 重试次数 */
        } http;
        struct {
            char broker[128];           /**< 代理地址 */
            uint16_t port;              /**< 端口 */
            char topic[128];            /**< 主题 */
            char client_id[64];         /**< 客户端ID */
            ota_auth_info_t auth;       /**< 认证信息 */
            bool use_ssl;               /**< 是否使用SSL */
            uint16_t timeout_ms;        /**< 超时时间(毫秒) */
            uint16_t retry_count;       /**< 重试次数 */
        } mqtt;
        struct {
            char path[256];             /**< 文件路径 */
        } local;
        void* custom;                   /**< 自定义源配置 */
    } source;
    
    ota_verify_type_t verify_type;      /**< 验证类型 */
    union {
        char md5[33];                   /**< MD5校验和 */
        char sha1[41];                  /**< SHA1校验和 */
        char sha256[65];                /**< SHA256校验和 */
        struct {
            const char* pub_key;        /**< 公钥 */
            uint32_t pub_key_len;       /**< 公钥长度 */
            const char* signature;      /**< 签名 */
            uint32_t sig_len;           /**< 签名长度 */
        } rsa;
        struct {
            const char* pub_key;        /**< 公钥 */
            uint32_t pub_key_len;       /**< 公钥长度 */
            const char* signature;      /**< 签名 */
            uint32_t sig_len;           /**< 签名长度 */
        } ecdsa;
    } verify;
    
    uint32_t partition_size;            /**< 分区大小(字节) */
    bool auto_reboot;                   /**< 是否自动重启 */
    bool rollback_enabled;              /**< 是否启用回滚 */
    uint16_t rollback_timeout;          /**< 回滚超时(秒) */
} ota_config_t;

/* OTA进度回调 */
typedef void (*ota_progress_cb_t)(ota_handle_t handle, ota_state_t state, uint8_t progress, void* user_data);

/* OTA事件回调 */
typedef void (*ota_event_cb_t)(ota_handle_t handle, ota_state_t state, ota_err_t error, void* user_data);

/**
 * @brief 初始化OTA模块
 * 
 * @param config OTA配置
 * @param progress_cb 进度回调函数
 * @param event_cb 事件回调函数
 * @param user_data 用户数据
 * @param handle 返回的OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_init(const ota_config_t* config, ota_progress_cb_t progress_cb, ota_event_cb_t event_cb, void* user_data, ota_handle_t* handle);

/**
 * @brief 反初始化OTA模块
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_deinit(ota_handle_t handle);

/**
 * @brief 开始OTA升级
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_begin(ota_handle_t handle);

/**
 * @brief 终止OTA升级
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_abort(ota_handle_t handle);

/**
 * @brief 验证已下载的固件
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_verify(ota_handle_t handle);

/**
 * @brief 安装已下载的固件
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_install(ota_handle_t handle);

/**
 * @brief 回滚到之前的固件
 * 
 * @param handle OTA句柄
 * @return int 成功返回0，失败返回错误码
 */
int ota_rollback(ota_handle_t handle);

/**
 * @brief 获取当前OTA状态
 * 
 * @param handle OTA句柄
 * @param state 返回的状态
 * @return int 成功返回0，失败返回错误码
 */
int ota_get_state(ota_handle_t handle, ota_state_t* state);

/**
 * @brief 获取当前OTA进度
 * 
 * @param handle OTA句柄
 * @param progress 返回的进度(0-100)
 * @return int 成功返回0，失败返回错误码
 */
int ota_get_progress(ota_handle_t handle, uint8_t* progress);

/**
 * @brief 获取当前运行的固件版本
 * 
 * @param version 版本字符串缓冲区
 * @param max_len 缓冲区最大长度
 * @return int 成功返回0，失败返回错误码
 */
int ota_get_current_version(char* version, uint16_t max_len);

/**
 * @brief 设置启动确认标志
 * 
 * 在更新后的第一次启动中调用，确认启动成功，防止系统自动回滚
 * 
 * @return int 成功返回0，失败返回错误码
 */
int ota_set_boot_confirmed(void);

/**
 * @brief 写入自定义OTA数据
 * 
 * 仅用于自定义源类型
 * 
 * @param handle OTA句柄
 * @param data 数据
 * @param len 数据长度
 * @return int 成功返回写入的字节数，失败返回错误码
 */
int ota_write(ota_handle_t handle, const void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* OTA_API_H */
