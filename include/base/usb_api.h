/**
 * @file usb_api.h
 * @brief USB接口抽象层定义
 *
 * 该头文件定义了USB设备的统一抽象接口，提供了USB配置、数据传输和状态管理功能
 */

#ifndef USB_API_H
#define USB_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* USB操作状态 */
typedef enum {
    USB_STATUS_IDLE,        /**< 空闲状态 */
    USB_STATUS_BUSY,        /**< 忙状态 */
    USB_STATUS_COMPLETE,    /**< 操作完成 */
    USB_STATUS_ERROR,       /**< 操作错误 */
    USB_STATUS_TIMEOUT      /**< 操作超时 */
} usb_status_t;

/* USB端点类型 */
typedef enum {
    USB_ENDPOINT_CONTROL,   /**< 控制端点 */
    USB_ENDPOINT_ISOCHRONOUS, /**< 同步端点 */
    USB_ENDPOINT_BULK,      /**< 批量端点 */
    USB_ENDPOINT_INTERRUPT  /**< 中断端点 */
} usb_endpoint_type_t;

/* USB端点方向 */
typedef enum {
    USB_DIR_OUT = 0,       /**< 输出端点（主机到设备） */
    USB_DIR_IN = 0x80      /**< 输入端点（设备到主机） */
} usb_direction_t;

/* USB设备角色 */
typedef enum {
    USB_ROLE_DEVICE,        /**< 设备模式 */
    USB_ROLE_HOST,          /**< 主机模式 */
    USB_ROLE_OTG            /**< OTG模式 */
} usb_role_t;

/* USB速度 */
typedef enum {
    USB_SPEED_LOW,          /**< 低速模式 (1.5 Mbit/s) */
    USB_SPEED_FULL,         /**< 全速模式 (12 Mbit/s) */
    USB_SPEED_HIGH,         /**< 高速模式 (480 Mbit/s) */
    USB_SPEED_SUPER         /**< 超高速模式 (5 Gbit/s) */
} usb_speed_t;

/* USB设备状态 */
typedef enum {
    USB_DEVICE_DISCONNECTED,/**< 设备断开连接 */
    USB_DEVICE_CONNECTED,   /**< 设备已连接 */
    USB_DEVICE_SUSPENDED,   /**< 设备已挂起 */
    USB_DEVICE_RESUMED,     /**< 设备已恢复 */
    USB_DEVICE_CONFIGURED,  /**< 设备已配置 */
    USB_DEVICE_ERROR        /**< 设备错误 */
} usb_device_state_t;

/* USB主机状态 */
typedef enum {
    USB_HOST_IDLE,          /**< 主机空闲 */
    USB_HOST_DEVICE_CONNECTED, /**< 主机检测到设备连接 */
    USB_HOST_DEVICE_DISCONNECTED, /**< 主机检测到设备断开 */
    USB_HOST_ENUMERATING,   /**< 主机正在枚举设备 */
    USB_HOST_READY,         /**< 主机就绪 */
    USB_HOST_ERROR          /**< 主机错误 */
} usb_host_state_t;

/* USB端点定义 */
typedef struct {
    uint8_t number;          /**< 端点号 */
    usb_endpoint_type_t type; /**< 端点类型 */
    usb_direction_t direction; /**< 端点方向 */
    uint16_t max_packet_size; /**< 最大包大小 */
    uint8_t interval;        /**< 轮询间隔(对于中断和同步端点) */
} usb_endpoint_t;

/* USB设备描述符 */
typedef struct {
    uint16_t vendor_id;      /**< 厂商ID */
    uint16_t product_id;     /**< 产品ID */
    uint16_t device_version; /**< 设备版本 */
    uint8_t device_class;    /**< 设备类别 */
    uint8_t device_subclass; /**< 设备子类别 */
    uint8_t device_protocol; /**< 设备协议 */
    char manufacturer[32];   /**< 制造商字符串 */
    char product[32];        /**< 产品字符串 */
    char serial_number[32];  /**< 序列号字符串 */
} usb_device_descriptor_t;

/* USB配置参数 */
typedef struct {
    usb_role_t role;         /**< USB设备角色 */
    usb_speed_t speed;       /**< USB速度 */
    uint8_t max_endpoints;   /**< 最大端点数量 */
    uint8_t max_interfaces;  /**< 最大接口数量 */
    uint8_t max_configurations; /**< 最大配置数量 */
    usb_device_descriptor_t *device_descriptor; /**< 设备描述符 */
    bool self_powered;       /**< 是否自供电 */
    uint16_t max_power;      /**< 最大电流(mA) */
} usb_config_t;

/* USB传输请求 */
typedef struct {
    void *data;             /**< 数据缓冲区 */
    uint32_t length;        /**< 数据长度 */
    uint32_t actual_length; /**< 实际传输长度 */
    uint8_t endpoint;       /**< 端点号 */
    usb_direction_t direction; /**< 传输方向 */
    uint32_t timeout;       /**< 超时时间(毫秒) */
    usb_status_t status;    /**< 传输状态 */
    void *user_data;        /**< 用户数据 */
} usb_transfer_t;

/* USB回调函数类型 */
typedef void (*usb_callback_t)(usb_transfer_t *transfer);

/* USB事件回调函数类型 */
typedef void (*usb_event_callback_t)(usb_device_state_t state, void *user_data);

/* USB设备句柄 */
typedef driver_handle_t usb_handle_t;

/**
 * @brief 初始化USB设备
 * 
 * @param config USB配置参数
 * @param handle USB设备句柄指针
 * @return api_status_t 操作状态
 */
api_status_t usb_init(const usb_config_t *config, usb_handle_t *handle);

/**
 * @brief 去初始化USB设备
 * 
 * @param handle USB设备句柄
 * @return api_status_t 操作状态
 */
api_status_t usb_deinit(usb_handle_t handle);

/**
 * @brief 启动USB设备
 * 
 * @param handle USB设备句柄
 * @return api_status_t 操作状态
 */
api_status_t usb_start(usb_handle_t handle);

/**
 * @brief 停止USB设备
 * 
 * @param handle USB设备句柄
 * @return api_status_t 操作状态
 */
api_status_t usb_stop(usb_handle_t handle);

/**
 * @brief 获取USB设备状态
 * 
 * @param handle USB设备句柄
 * @param state 设备状态指针
 * @return api_status_t 操作状态
 */
api_status_t usb_get_device_state(usb_handle_t handle, usb_device_state_t *state);

/**
 * @brief 注册USB事件回调函数
 * 
 * @param handle USB设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return api_status_t 操作状态
 */
api_status_t usb_register_event_callback(usb_handle_t handle, usb_event_callback_t callback, void *user_data);

/**
 * @brief 配置USB端点
 * 
 * @param handle USB设备句柄
 * @param endpoint 端点配置
 * @return api_status_t 操作状态
 */
api_status_t usb_configure_endpoint(usb_handle_t handle, const usb_endpoint_t *endpoint);

/**
 * @brief 取消配置USB端点
 * 
 * @param handle USB设备句柄
 * @param endpoint_num 端点号
 * @param direction 端点方向
 * @return api_status_t 操作状态
 */
api_status_t usb_unconfigure_endpoint(usb_handle_t handle, uint8_t endpoint_num, usb_direction_t direction);

/**
 * @brief 通过USB端点发送数据
 * 
 * @param handle USB设备句柄
 * @param endpoint_num 端点号
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t usb_send_data(usb_handle_t handle, uint8_t endpoint_num, const void *data, uint32_t length, uint32_t timeout_ms);

/**
 * @brief 从USB端点接收数据
 * 
 * @param handle USB设备句柄
 * @param endpoint_num 端点号
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param actual_length 实际接收长度指针
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t usb_receive_data(usb_handle_t handle, uint8_t endpoint_num, void *data, uint32_t length, uint32_t *actual_length, uint32_t timeout_ms);

/**
 * @brief 提交USB传输请求
 * 
 * @param handle USB设备句柄
 * @param transfer 传输请求
 * @param callback 完成回调函数
 * @return api_status_t 操作状态
 */
api_status_t usb_submit_transfer(usb_handle_t handle, usb_transfer_t *transfer, usb_callback_t callback);

/**
 * @brief 取消USB传输请求
 * 
 * @param handle USB设备句柄
 * @param transfer 传输请求
 * @return api_status_t 操作状态
 */
api_status_t usb_cancel_transfer(usb_handle_t handle, usb_transfer_t *transfer);

/**
 * @brief 设置USB设备地址
 * 
 * @param handle USB设备句柄
 * @param address 设备地址
 * @return api_status_t 操作状态
 */
api_status_t usb_set_address(usb_handle_t handle, uint8_t address);

/**
 * @brief 设置USB设备配置
 * 
 * @param handle USB设备句柄
 * @param config_num 配置号
 * @return api_status_t 操作状态
 */
api_status_t usb_set_configuration(usb_handle_t handle, uint8_t config_num);

/**
 * @brief USB设备进入挂起状态
 * 
 * @param handle USB设备句柄
 * @return api_status_t 操作状态
 */
api_status_t usb_suspend(usb_handle_t handle);

/**
 * @brief USB设备恢复
 * 
 * @param handle USB设备句柄
 * @return api_status_t 操作状态
 */
api_status_t usb_resume(usb_handle_t handle);

/**
 * @brief 获取USB速度
 * 
 * @param handle USB设备句柄
 * @param speed 速度指针
 * @return api_status_t 操作状态
 */
api_status_t usb_get_speed(usb_handle_t handle, usb_speed_t *speed);

/**
 * @brief 设置USB角色
 * 
 * @param handle USB设备句柄
 * @param role 角色
 * @return api_status_t 操作状态
 */
api_status_t usb_set_role(usb_handle_t handle, usb_role_t role);

/**
 * @brief 获取USB角色
 * 
 * @param handle USB设备句柄
 * @param role 角色指针
 * @return api_status_t 操作状态
 */
api_status_t usb_get_role(usb_handle_t handle, usb_role_t *role);

#ifdef __cplusplus
}
#endif

#endif /* USB_API_H */