/**
 * @file access_control_api.h
 * @brief 访问控制接口抽象层定义
 *
 * 该头文件定义了访问控制的统一抽象接口，用于管理和验证各种凭证的访问权限
 */

#ifndef ACCESS_CONTROL_API_H
#define ACCESS_CONTROL_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "device_auth_api.h"

/* 访问控制句柄 */
typedef driver_handle_t access_control_handle_t;

/* 凭证类型枚举 */
typedef enum {
    CREDENTIAL_TYPE_CARD,         /**< 卡片凭证 */
    CREDENTIAL_TYPE_PASSWORD,     /**< 密码凭证 */
    CREDENTIAL_TYPE_BIOMETRIC,    /**< 生物特征凭证 */
    CREDENTIAL_TYPE_QR_CODE,      /**< 二维码凭证 */
    CREDENTIAL_TYPE_MOBILE,       /**< 手机凭证 */
    CREDENTIAL_TYPE_REMOTE,       /**< 远程授权凭证 */
    CREDENTIAL_TYPE_TEMPORARY,    /**< 临时凭证 */
    CREDENTIAL_TYPE_CUSTOM        /**< 自定义凭证 */
} credential_type_t;

/* 验证方式枚举 */
typedef enum {
    VERIFY_MODE_SINGLE,           /**< 单一凭证验证 */
    VERIFY_MODE_DUAL,             /**< 双重凭证验证 */
    VERIFY_MODE_MULTI             /**< 多重凭证验证 */
} verify_mode_t;

/* 验证结果枚举 */
typedef enum {
    VERIFY_RESULT_SUCCESS,        /**< 验证成功 */
    VERIFY_RESULT_FAILED,         /**< 验证失败 */
    VERIFY_RESULT_TIMEOUT,        /**< 验证超时 */
    VERIFY_RESULT_INVALID,        /**< 无效凭证 */
    VERIFY_RESULT_EXPIRED,        /**< 凭证过期 */
    VERIFY_RESULT_BLACKLISTED,    /**< 凭证被列入黑名单 */
    VERIFY_RESULT_NO_PERMISSION,  /**< 无权限 */
    VERIFY_RESULT_ERROR           /**< 验证错误 */
} verify_result_t;

/* 凭证信息结构体 */
typedef struct {
    credential_type_t type;       /**< 凭证类型 */
    char id[64];                  /**< 凭证ID */
    char holder_id[64];           /**< 持有者ID */
    char holder_name[64];         /**< 持有者姓名 */
    uint32_t issue_time;          /**< 签发时间 */
    uint32_t expire_time;         /**< 过期时间 */
    uint8_t permission_level;     /**< 权限级别 */
    char access_zones[256];       /**< 可访问区域,以逗号分隔 */
    uint32_t time_restrictions;   /**< 时间限制标志 */
    bool is_active;               /**< 是否激活 */
    bool is_blacklisted;          /**< 是否被列入黑名单 */
} credential_info_t;

/* 访问记录结构体 */
typedef struct {
    uint32_t record_id;           /**< 记录ID */
    char credential_id[64];       /**< 凭证ID */
    credential_type_t type;       /**< 凭证类型 */
    char holder_id[64];           /**< 持有者ID */
    char holder_name[64];         /**< 持有者姓名 */
    uint32_t timestamp;           /**< 时间戳 */
    char zone_id[32];             /**< 区域ID */
    verify_result_t result;       /**< 验证结果 */
    char device_id[32];           /**< 设备ID */
    char extra_info[128];         /**< 额外信息 */
} access_record_t;

/* 访问控制配置结构体 */
typedef struct {
    verify_mode_t verify_mode;                /**< 验证方式 */
    uint16_t verify_timeout_ms;               /**< 验证超时时间 */
    char database_path[128];                  /**< 凭证数据库路径 */
    char record_path[128];                    /**< 记录存储路径 */
    bool enable_remote_auth;                  /**< 启用远程认证 */
    char auth_server_url[128];                /**< 认证服务器URL */
    jwt_token_t *auth_token;                  /**< 认证令牌 */
    bool cache_credentials;                   /**< 缓存凭证 */
    uint16_t cache_timeout_min;               /**< 缓存超时时间(分钟) */
    bool offline_operation;                   /**< 离线操作模式 */
    uint32_t max_records;                     /**< 最大记录数量 */
    bool auto_sync;                           /**< 自动同步 */
} access_control_config_t;

/* 验证结果回调函数类型 */
typedef void (*verify_result_callback_t)(verify_result_t result, 
                                        const credential_info_t *credential, 
                                        void *user_data);

/**
 * @brief 初始化访问控制模块
 * 
 * @param config 访问控制配置
 * @param handle 返回的访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_init(const access_control_config_t *config, 
                       access_control_handle_t *handle);

/**
 * @brief 注册验证结果回调
 * 
 * @param handle 访问控制句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int access_control_register_callback(access_control_handle_t handle, 
                                    verify_result_callback_t callback, 
                                    void *user_data);

/**
 * @brief 添加凭证
 * 
 * @param handle 访问控制句柄
 * @param credential 凭证信息
 * @return int 0表示成功，非0表示失败
 */
int access_control_add_credential(access_control_handle_t handle, 
                                 const credential_info_t *credential);

/**
 * @brief 删除凭证
 * 
 * @param handle 访问控制句柄
 * @param credential_id 凭证ID
 * @return int 0表示成功，非0表示失败
 */
int access_control_delete_credential(access_control_handle_t handle, 
                                    const char *credential_id);

/**
 * @brief 查找凭证
 * 
 * @param handle 访问控制句柄
 * @param credential_id 凭证ID
 * @param credential 返回的凭证信息
 * @return int 0表示成功，非0表示失败
 */
int access_control_find_credential(access_control_handle_t handle, 
                                  const char *credential_id, 
                                  credential_info_t *credential);

/**
 * @brief 更新凭证
 * 
 * @param handle 访问控制句柄
 * @param credential 凭证信息
 * @return int 0表示成功，非0表示失败
 */
int access_control_update_credential(access_control_handle_t handle, 
                                    const credential_info_t *credential);

/**
 * @brief 验证凭证
 * 
 * @param handle 访问控制句柄
 * @param type 凭证类型
 * @param credential_data 凭证数据
 * @param data_length 数据长度
 * @param zone_id 区域ID
 * @param result 返回的验证结果
 * @param credential_info 返回的凭证信息(可选,NULL表示不需要)
 * @return int 0表示成功，非0表示失败
 */
int access_control_verify_credential(access_control_handle_t handle, 
                                    credential_type_t type, 
                                    const void *credential_data, 
                                    size_t data_length, 
                                    const char *zone_id, 
                                    verify_result_t *result, 
                                    credential_info_t *credential_info);

/**
 * @brief 添加访问记录
 * 
 * @param handle 访问控制句柄
 * @param record 访问记录
 * @return int 0表示成功，非0表示失败
 */
int access_control_add_record(access_control_handle_t handle, 
                             const access_record_t *record);

/**
 * @brief 查询访问记录
 * 
 * @param handle 访问控制句柄
 * @param filter 过滤条件(NULL表示所有记录)
 * @param records 记录数组
 * @param max_records 数组最大容量
 * @param actual_count 实际记录数量
 * @return int 0表示成功，非0表示失败
 */
int access_control_query_records(access_control_handle_t handle, 
                                const char *filter, 
                                access_record_t *records, 
                                size_t max_records, 
                                size_t *actual_count);

/**
 * @brief 同步凭证数据库
 * 
 * @param handle 访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_sync_database(access_control_handle_t handle);

/**
 * @brief 同步访问记录
 * 
 * @param handle 访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_sync_records(access_control_handle_t handle);

/**
 * @brief 设置访问控制参数
 * 
 * @param handle 访问控制句柄
 * @param param_name 参数名
 * @param value 参数值
 * @param value_size 参数大小
 * @return int 0表示成功，非0表示失败
 */
int access_control_set_parameter(access_control_handle_t handle, 
                                const char *param_name, 
                                const void *value, 
                                size_t value_size);

/**
 * @brief 获取访问控制参数
 * 
 * @param handle 访问控制句柄
 * @param param_name 参数名
 * @param value 参数值缓冲区
 * @param value_size 缓冲区大小
 * @param actual_size 实际参数大小
 * @return int 0表示成功，非0表示失败
 */
int access_control_get_parameter(access_control_handle_t handle, 
                                const char *param_name, 
                                void *value, 
                                size_t value_size, 
                                size_t *actual_size);

/**
 * @brief 开始批量导入凭证
 * 
 * @param handle 访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_begin_batch_import(access_control_handle_t handle);

/**
 * @brief 结束批量导入凭证
 * 
 * @param handle 访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_end_batch_import(access_control_handle_t handle);

/**
 * @brief 清空凭证数据库
 * 
 * @param handle 访问控制句柄
 * @return int 0表示成功，非0表示失败
 */
int access_control_clear_database(access_control_handle_t handle);

#endif /* ACCESS_CONTROL_API_H */
