/**
 * @file device_manager_api.h
 * @brief 设备管理接口抽象层定义
 *
 * 该头文件定义了设备管理的统一抽象接口，用于管理摄像头、读卡器、扫描器等外部设备
 */

#ifndef DEVICE_MANAGER_API_H
#define DEVICE_MANAGER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 设备类型枚举 */
typedef enum {
    DEVICE_TYPE_CAMERA,           /**< 摄像头 */
    DEVICE_TYPE_CARD_READER,      /**< 读卡器 */
    DEVICE_TYPE_QR_SCANNER,       /**< 二维码扫描器 */
    DEVICE_TYPE_FACE_CHIP,        /**< 人脸识别芯片 */
    DEVICE_TYPE_TOUCH_PANEL,      /**< 触控面板 */
    DEVICE_TYPE_FINGERPRINT,      /**< 指纹识别器 */
    DEVICE_TYPE_CUSTOM            /**< 自定义设备 */
} device_type_t;

/* 设备状态枚举 */
typedef enum {
    DEVICE_STATUS_DISCONNECTED,   /**< 设备断开连接 */
    DEVICE_STATUS_CONNECTING,     /**< 设备正在连接 */
    DEVICE_STATUS_CONNECTED,      /**< 设备已连接 */
    DEVICE_STATUS_ERROR,          /**< 设备错误 */
    DEVICE_STATUS_BUSY,           /**< 设备忙 */
    DEVICE_STATUS_IDLE            /**< 设备空闲 */
} device_status_t;

/* 设备句柄 */
typedef driver_handle_t device_handle_t;

/* 设备信息结构体 */
typedef struct {
    char name[32];                /**< 设备名称 */
    char model[32];               /**< 设备型号 */
    char version[16];             /**< 设备版本 */
    device_type_t type;           /**< 设备类型 */
    device_status_t status;       /**< 设备状态 */
    uint8_t address;              /**< 设备地址/ID */
    char connection_info[64];     /**< 连接信息 */
} device_info_t;

/* 设备配置结构体 */
typedef struct {
    device_type_t type;           /**< 设备类型 */
    void *device_config;          /**< 设备特定配置，不同设备类型有不同结构 */
    uint16_t timeout_ms;          /**< 通信超时时间 */
    uint8_t retries;              /**< 重试次数 */
    bool auto_reconnect;          /**< 自动重连标志 */
} device_config_t;

/* 设备状态变化回调函数类型 */
typedef void (*device_status_callback_t)(device_handle_t handle, 
                                        device_status_t status, 
                                        void *user_data);

/**
 * @brief 初始化设备管理器
 * 
 * @return int 0表示成功，非0表示失败
 */
int device_manager_init(void);

/**
 * @brief 添加设备
 * 
 * @param config 设备配置
 * @param handle 返回的设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_add_device(const device_config_t *config, device_handle_t *handle);

/**
 * @brief 移除设备
 * 
 * @param handle 设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_remove_device(device_handle_t handle);

/**
 * @brief 连接设备
 * 
 * @param handle 设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_connect_device(device_handle_t handle);

/**
 * @brief 断开设备连接
 * 
 * @param handle 设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_disconnect_device(device_handle_t handle);

/**
 * @brief 获取设备信息
 * 
 * @param handle 设备句柄
 * @param info 设备信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int device_manager_get_device_info(device_handle_t handle, device_info_t *info);

/**
 * @brief 发送命令到设备
 * 
 * @param handle 设备句柄
 * @param command 命令缓冲区
 * @param cmd_length 命令长度
 * @param response 响应缓冲区
 * @param resp_size 响应缓冲区大小
 * @param actual_size 实际接收的响应大小
 * @return int 0表示成功，非0表示失败
 */
int device_manager_send_command(device_handle_t handle, 
                               const void *command, 
                               size_t cmd_length, 
                               void *response, 
                               size_t resp_size, 
                               size_t *actual_size);

/**
 * @brief 注册设备状态回调
 * 
 * @param handle 设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int device_manager_register_status_callback(device_handle_t handle, 
                                          device_status_callback_t callback, 
                                          void *user_data);

/**
 * @brief 配置设备参数
 * 
 * @param handle 设备句柄
 * @param param_name 参数名
 * @param param_value 参数值
 * @return int 0表示成功，非0表示失败
 */
int device_manager_set_device_param(device_handle_t handle, 
                                   const char *param_name, 
                                   const void *param_value, 
                                   size_t value_size);

/**
 * @brief 获取设备参数
 * 
 * @param handle 设备句柄
 * @param param_name 参数名
 * @param param_value 参数值缓冲区
 * @param value_size 缓冲区大小
 * @param actual_size 实际参数大小
 * @return int 0表示成功，非0表示失败
 */
int device_manager_get_device_param(device_handle_t handle, 
                                   const char *param_name, 
                                   void *param_value, 
                                   size_t value_size, 
                                   size_t *actual_size);

/**
 * @brief 启动设备自检
 * 
 * @param handle 设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_self_test(device_handle_t handle);

/**
 * @brief 获取设备列表
 * 
 * @param devices 设备信息数组
 * @param max_devices 数组最大容量
 * @param actual_count 实际设备数量
 * @return int 0表示成功，非0表示失败
 */
int device_manager_get_device_list(device_info_t *devices, 
                                  size_t max_devices, 
                                  size_t *actual_count);

/**
 * @brief 根据类型查找设备
 * 
 * @param type 设备类型
 * @param handles 设备句柄数组
 * @param max_handles 数组最大容量
 * @param actual_count 实际找到的设备数量
 * @return int 0表示成功，非0表示失败
 */
int device_manager_find_devices_by_type(device_type_t type, 
                                       device_handle_t *handles, 
                                       size_t max_handles, 
                                       size_t *actual_count);

/**
 * @brief 根据名称查找设备
 * 
 * @param name 设备名称
 * @param handle 返回的设备句柄
 * @return int 0表示成功，非0表示失败
 */
int device_manager_find_device_by_name(const char *name, device_handle_t *handle);

#endif /* DEVICE_MANAGER_API_H */
