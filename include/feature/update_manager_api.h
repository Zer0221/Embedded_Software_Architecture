/**
 * @file update_manager_api.h
 * @brief 更新管理接口抽象层定义
 *
 * 该头文件定义了系统更新管理的统一抽象接口，包括OTA和USB更新，以及版本回滚功能
 */

#ifndef UPDATE_MANAGER_API_H
#define UPDATE_MANAGER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 更新管理器句柄 */
typedef driver_handle_t update_manager_handle_t;

/* 更新类型枚举 */
typedef enum {
    UPDATE_TYPE_OTA,          /**< OTA更新 */
    UPDATE_TYPE_USB,          /**< USB更新 */
    UPDATE_TYPE_LOCAL         /**< 本地更新 */
} update_type_t;

/* 更新状态枚举 */
typedef enum {
    UPDATE_STATE_IDLE,                /**< 空闲状态 */
    UPDATE_STATE_CHECKING,            /**< 检查更新 */
    UPDATE_STATE_DOWNLOADING,         /**< 下载中 */
    UPDATE_STATE_VERIFYING,           /**< 验证中 */
    UPDATE_STATE_READY_TO_UPDATE,     /**< 准备更新 */
    UPDATE_STATE_UPDATING,            /**< 更新中 */
    UPDATE_STATE_REBOOTING,           /**< 重启中 */
    UPDATE_STATE_ROLLED_BACK,         /**< 已回滚 */
    UPDATE_STATE_COMPLETED,           /**< 完成 */
    UPDATE_STATE_FAILED               /**< 失败 */
} update_state_t;

/* 更新错误枚举 */
typedef enum {
    UPDATE_ERROR_NONE,                /**< 无错误 */
    UPDATE_ERROR_INVALID_PACKAGE,     /**< 无效更新包 */
    UPDATE_ERROR_DOWNLOAD_FAILED,     /**< 下载失败 */
    UPDATE_ERROR_VERIFICATION_FAILED, /**< 验证失败 */
    UPDATE_ERROR_FLASH_WRITE_FAILED,  /**< Flash写入失败 */
    UPDATE_ERROR_STORAGE_FULL,        /**< 存储空间不足 */
    UPDATE_ERROR_NO_BACKUP,           /**< 无备份可回滚 */
    UPDATE_ERROR_LOW_BATTERY,         /**< 电量不足 */
    UPDATE_ERROR_CONNECTION_LOST,     /**< 连接断开 */
    UPDATE_ERROR_INCOMPATIBLE,        /**< 版本不兼容 */
    UPDATE_ERROR_TIMEOUT,             /**< 超时 */
    UPDATE_ERROR_UNKNOWN              /**< 未知错误 */
} update_error_t;

/* 版本信息结构体 */
typedef struct {
    char version[32];         /**< 版本字符串 */
    uint32_t build_number;    /**< 构建编号 */
    uint32_t timestamp;       /**< 构建时间戳 */
    char build_type[16];      /**< 构建类型 */
    char target_hardware[32]; /**< 目标硬件 */
    char release_notes[256];  /**< 发布说明 */
    uint32_t min_space_required; /**< 最小所需空间(字节) */
} version_info_t;

/* 更新包信息结构体 */
typedef struct {
    char filename[64];        /**< 文件名 */
    uint32_t size;            /**< 文件大小 */
    version_info_t version;   /**< 版本信息 */
    char checksum[32];        /**< 校验和 */
    bool is_encrypted;        /**< 是否加密 */
    bool is_differential;     /**< 是否差分更新 */
    update_type_t type;       /**< 更新类型 */
} update_package_info_t;

/* 更新进度结构体 */
typedef struct {
    update_state_t state;     /**< 更新状态 */
    uint8_t progress;         /**< 进度百分比 */
    update_error_t error;     /**< 错误类型 */
    char error_message[128];  /**< 错误信息 */
    uint32_t bytes_processed; /**< 已处理字节数 */
    uint32_t total_bytes;     /**< 总字节数 */
    uint32_t time_remaining;  /**< 预计剩余时间(秒) */
} update_progress_t;

/* 更新管理器配置参数 */
typedef struct {
    char *update_server_url;      /**< 更新服务器URL */
    char *download_dir;           /**< 下载目录 */
    char *backup_dir;             /**< 备份目录 */
    uint32_t max_retries;         /**< 最大重试次数 */
    uint16_t timeout_ms;          /**< 超时时间(毫秒) */
    bool auto_reboot;             /**< 更新后自动重启 */
    bool auto_backup;             /**< 更新前自动备份 */
    uint8_t max_backups;          /**< 最大备份数量 */
    bool verify_signature;        /**< 是否验证签名 */
    char *public_key_path;        /**< 公钥路径 */
} update_manager_config_t;

/* 更新回调函数类型 */
typedef void (*update_callback_t)(update_progress_t *progress, void *user_data);

/**
 * @brief 初始化更新管理器
 * 
 * @param config 管理器配置参数
 * @param handle 管理器句柄指针
 * @return int 0表示成功，非0表示失败
 */
int update_manager_init(const update_manager_config_t *config, update_manager_handle_t *handle);

/**
 * @brief 注册更新回调
 * 
 * @param handle 管理器句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int update_manager_register_callback(update_manager_handle_t handle, 
                                    update_callback_t callback, 
                                    void *user_data);

/**
 * @brief 检查更新
 * 
 * @param handle 管理器句柄
 * @param current_version 当前版本
 * @param has_update 是否有更新
 * @param update_info 更新信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int update_manager_check_update(update_manager_handle_t handle, 
                               const char *current_version, 
                               bool *has_update, 
                               update_package_info_t *update_info);

/**
 * @brief 开始OTA更新
 * 
 * @param handle 管理器句柄
 * @param package_url 更新包URL
 * @return int 0表示成功，非0表示失败
 */
int update_manager_start_ota_update(update_manager_handle_t handle, const char *package_url);

/**
 * @brief 开始USB更新
 * 
 * @param handle 管理器句柄
 * @param usb_path USB设备路径
 * @param package_name 更新包名称
 * @return int 0表示成功，非0表示失败
 */
int update_manager_start_usb_update(update_manager_handle_t handle, 
                                   const char *usb_path, 
                                   const char *package_name);

/**
 * @brief 开始本地更新
 * 
 * @param handle 管理器句柄
 * @param package_path 更新包路径
 * @return int 0表示成功，非0表示失败
 */
int update_manager_start_local_update(update_manager_handle_t handle, const char *package_path);

/**
 * @brief 取消更新
 * 
 * @param handle 管理器句柄
 * @return int 0表示成功，非0表示失败
 */
int update_manager_cancel_update(update_manager_handle_t handle);

/**
 * @brief 获取更新进度
 * 
 * @param handle 管理器句柄
 * @param progress 进度结构体指针
 * @return int 0表示成功，非0表示失败
 */
int update_manager_get_progress(update_manager_handle_t handle, update_progress_t *progress);

/**
 * @brief 应用更新
 * 
 * @param handle 管理器句柄
 * @param reboot 是否重启
 * @return int 0表示成功，非0表示失败
 */
int update_manager_apply_update(update_manager_handle_t handle, bool reboot);

/**
 * @brief 回滚更新
 * 
 * @param handle 管理器句柄
 * @param version 目标版本，NULL表示回滚到上一个版本
 * @return int 0表示成功，非0表示失败
 */
int update_manager_rollback(update_manager_handle_t handle, const char *version);

/**
 * @brief 获取备份版本列表
 * 
 * @param handle 管理器句柄
 * @param versions 版本信息数组
 * @param array_size 数组大小
 * @param version_count 版本数量
 * @return int 0表示成功，非0表示失败
 */
int update_manager_get_backup_versions(update_manager_handle_t handle, 
                                      version_info_t *versions, 
                                      size_t array_size, 
                                      size_t *version_count);

/**
 * @brief 获取当前版本信息
 * 
 * @param handle 管理器句柄
 * @param version 版本信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int update_manager_get_current_version(update_manager_handle_t handle, version_info_t *version);

/**
 * @brief 验证更新包
 * 
 * @param handle 管理器句柄
 * @param package_path 更新包路径
 * @param is_valid 是否有效
 * @return int 0表示成功，非0表示失败
 */
int update_manager_verify_package(update_manager_handle_t handle, 
                                 const char *package_path, 
                                 bool *is_valid);

/**
 * @brief 清理更新缓存
 * 
 * @param handle 管理器句柄
 * @return int 0表示成功，非0表示失败
 */
int update_manager_clean_cache(update_manager_handle_t handle);

#endif /* UPDATE_MANAGER_API_H */
