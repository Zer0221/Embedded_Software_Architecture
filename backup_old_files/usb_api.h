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
#include "driver_api.h"

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

/* USB电源模式 */
typedef enum {
    USB_POWER_BUS,          /**< 总线供电 */
    USB_POWER_SELF          /**< 自供电 */
} usb_power_t;

/* USB设备状态 */
typedef enum {
    USB_DEVICE_DISCONNECTED, /**< 断开连接 */
    USB_DEVICE_CONNECTED,    /**< 已连接 */
    USB_DEVICE_SUSPENDED,    /**< 挂起 */
    USB_DEVICE_RESUMED,      /**< 已恢复 */
    USB_DEVICE_CONFIGURED,   /**< 已配置 */
    USB_DEVICE_ADDRESS,      /**< 已分配地址 */
    USB_DEVICE_DEFAULT       /**< 默认状态 */
} usb_device_state_t;

/* USB传输类型 */
typedef enum {
    USB_TRANSFER_SETUP,     /**< 设置传输 */
    USB_TRANSFER_DATA,      /**< 数据传输 */
    USB_TRANSFER_STATUS     /**< 状态传输 */
} usb_transfer_type_t;

/* USB类型 */
typedef enum {
    USB_CLASS_AUDIO = 0x01,         /**< 音频设备 */
    USB_CLASS_CDC = 0x02,           /**< 通信设备类 */
    USB_CLASS_HID = 0x03,           /**< 人机接口设备 */
    USB_CLASS_PHYSICAL = 0x05,      /**< 物理设备 */
    USB_CLASS_IMAGE = 0x06,         /**< 图像设备 */
    USB_CLASS_PRINTER = 0x07,       /**< 打印机设备 */
    USB_CLASS_MASS_STORAGE = 0x08,  /**< 大容量存储设备 */
    USB_CLASS_HUB = 0x09,           /**< HUB设备 */
    USB_CLASS_CDC_DATA = 0x0A,      /**< CDC数据设备 */
    USB_CLASS_SMART_CARD = 0x0B,    /**< 智能卡设备 */
    USB_CLASS_CONTENT_SECURITY = 0x0D, /**< 内容安全设备 */
    USB_CLASS_VIDEO = 0x0E,         /**< 视频设备 */
    USB_CLASS_HEALTHCARE = 0x0F,    /**< 健康设备 */
    USB_CLASS_DIAGNOSTIC = 0xDC,    /**< 诊断设备 */
    USB_CLASS_WIRELESS = 0xE0,      /**< 无线控制器 */
    USB_CLASS_MISC = 0xEF,          /**< 其他设备 */
    USB_CLASS_APP_SPECIFIC = 0xFE,  /**< 应用程序特定 */
    USB_CLASS_VENDOR_SPECIFIC = 0xFF /**< 厂商特定 */
} usb_class_t;

/* USB描述符 */
typedef struct {
    uint8_t *buffer;              /**< 描述符缓冲区 */
    uint16_t length;              /**< 描述符长度 */
} usb_descriptor_t;

/* USB端点配置 */
typedef struct {
    uint8_t ep_addr;              /**< 端点地址（包含方向位） */
    usb_endpoint_type_t type;     /**< 端点类型 */
    uint16_t max_packet_size;     /**< 最大包大小 */
    uint8_t interval;             /**< 轮询间隔 */
} usb_endpoint_config_t;

/* USB接口配置 */
typedef struct {
    uint8_t interface_num;        /**< 接口编号 */
    uint8_t alt_setting;          /**< 备用设置 */
    usb_class_t class_type;       /**< 接口类型 */
    uint8_t subclass;             /**< 子类 */
    uint8_t protocol;             /**< 协议 */
    uint8_t num_endpoints;        /**< 端点数量 */
    usb_endpoint_config_t *endpoints; /**< 端点配置数组 */
} usb_interface_config_t;

/* USB配置信息 */
typedef struct {
    usb_role_t role;              /**< USB角色 */
    usb_power_t power_mode;       /**< 电源模式 */
    uint16_t max_power;           /**< 最大功耗（2mA单位） */
    uint8_t num_interfaces;       /**< 接口数量 */
    usb_interface_config_t *interfaces; /**< 接口配置数组 */
    usb_descriptor_t device_descriptor; /**< 设备描述符 */
    usb_descriptor_t config_descriptor;  /**< 配置描述符 */
    usb_descriptor_t *string_descriptors; /**< 字符串描述符数组 */
    uint8_t num_string_descriptors; /**< 字符串描述符数量 */
} usb_config_t;

/* USB传输结构体 */
typedef struct {
    uint8_t ep_addr;              /**< 端点地址 */
    uint8_t *buffer;              /**< 数据缓冲区 */
    uint32_t length;              /**< 数据长度 */
    uint32_t actual_length;       /**< 实际传输长度 */
    usb_transfer_type_t type;     /**< 传输类型 */
    void *user_data;              /**< 用户数据 */
} usb_transfer_t;

/* USB主机设备信息 */
typedef struct {
    usb_speed_t speed;            /**< 设备速度 */
    uint16_t vendor_id;           /**< 厂商ID */
    uint16_t product_id;          /**< 产品ID */
    uint8_t device_address;       /**< 设备地址 */
    uint8_t num_configurations;   /**< 配置数量 */
    uint8_t current_configuration; /**< 当前配置 */
} usb_host_device_info_t;

/* USB回调函数类型 */
typedef void (*usb_callback_t)(void *arg, usb_status_t status, usb_transfer_t *transfer);

/* USB设备状态变化回调函数类型 */
typedef void (*usb_device_state_callback_t)(void *arg, usb_device_state_t state);

/* USB设备句柄 */
typedef driver_handle_t usb_handle_t;

/**
 * @brief 初始化USB设备
 * 
 * @param config USB配置参数
 * @param state_callback 设备状态回调函数
 * @param arg 传递给回调函数的参数
 * @param handle USB设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int usb_init(const usb_config_t *config, usb_device_state_callback_t state_callback, void *arg, usb_handle_t *handle);

/**
 * @brief 去初始化USB设备
 * 
 * @param handle USB设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_deinit(usb_handle_t handle);

/**
 * @brief 启动USB设备
 * 
 * @param handle USB设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_start(usb_handle_t handle);

/**
 * @brief 停止USB设备
 * 
 * @param handle USB设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_stop(usb_handle_t handle);

/**
 * @brief 设置USB设备地址
 * 
 * @param handle USB设备句柄
 * @param address 设备地址(1-127)
 * @return int 0表示成功，非0表示失败
 */
int usb_set_address(usb_handle_t handle, uint8_t address);

/**
 * @brief 设置USB设备配置
 * 
 * @param handle USB设备句柄
 * @param config_num 配置号
 * @return int 0表示成功，非0表示失败
 */
int usb_set_configuration(usb_handle_t handle, uint8_t config_num);

/**
 * @brief 设置USB接口
 * 
 * @param handle USB设备句柄
 * @param interface_num 接口号
 * @param alt_setting 备用设置
 * @return int 0表示成功，非0表示失败
 */
int usb_set_interface(usb_handle_t handle, uint8_t interface_num, uint8_t alt_setting);

/**
 * @brief 挂起USB设备
 * 
 * @param handle USB设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_suspend(usb_handle_t handle);

/**
 * @brief 恢复USB设备
 * 
 * @param handle USB设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_resume(usb_handle_t handle);

/**
 * @brief 获取USB设备速度
 * 
 * @param handle USB设备句柄
 * @param speed 速度指针
 * @return int 0表示成功，非0表示失败
 */
int usb_get_speed(usb_handle_t handle, usb_speed_t *speed);

/**
 * @brief 获取USB设备状态
 * 
 * @param handle USB设备句柄
 * @param state 状态指针
 * @return int 0表示成功，非0表示失败
 */
int usb_get_device_state(usb_handle_t handle, usb_device_state_t *state);

/**
 * @brief 传输USB数据
 * 
 * @param handle USB设备句柄
 * @param transfer 传输结构体
 * @param callback 完成回调函数
 * @param arg 传递给回调函数的参数
 * @return int 0表示成功，非0表示失败
 */
int usb_transfer(usb_handle_t handle, usb_transfer_t *transfer, usb_callback_t callback, void *arg);

/**
 * @brief 取消USB传输
 * 
 * @param handle USB设备句柄
 * @param ep_addr 端点地址
 * @return int 0表示成功，非0表示失败
 */
int usb_cancel_transfer(usb_handle_t handle, uint8_t ep_addr);

/**
 * @brief 获取USB设备描述符
 * 
 * @param handle USB设备句柄
 * @param type 描述符类型
 * @param index 描述符索引
 * @param lang_id 语言ID
 * @param data 数据缓冲区
 * @param length 缓冲区长度
 * @return int 0表示成功，非0表示失败
 */
int usb_get_descriptor(usb_handle_t handle, uint8_t type, uint8_t index, uint16_t lang_id, uint8_t *data, uint16_t length);

/**
 * @brief 清除端点暂停状态
 * 
 * @param handle USB设备句柄
 * @param ep_addr 端点地址
 * @return int 0表示成功，非0表示失败
 */
int usb_clear_halt(usb_handle_t handle, uint8_t ep_addr);

/**
 * @brief 获取端点状态
 * 
 * @param handle USB设备句柄
 * @param ep_addr 端点地址
 * @param halted 暂停状态指针
 * @return int 0表示成功，非0表示失败
 */
int usb_get_endpoint_status(usb_handle_t handle, uint8_t ep_addr, bool *halted);

/**
 * @brief 注册端点回调函数
 * 
 * @param handle USB设备句柄
 * @param ep_addr 端点地址
 * @param callback 回调函数
 * @param arg 传递给回调函数的参数
 * @return int 0表示成功，非0表示失败
 */
int usb_register_endpoint_callback(usb_handle_t handle, uint8_t ep_addr, usb_callback_t callback, void *arg);

/**
 * @brief 获取USB操作状态
 * 
 * @param handle USB设备句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int usb_get_status(usb_handle_t handle, usb_status_t *status);

/* USB主机模式函数 */

/**
 * @brief 枚举USB设备
 * 
 * @param handle USB主机句柄
 * @param port 端口号
 * @param device_info 设备信息指针
 * @return int 0表示成功，非0表示失败
 */
int usb_host_enumerate_device(usb_handle_t handle, uint8_t port, usb_host_device_info_t *device_info);

/**
 * @brief 打开USB设备
 * 
 * @param handle USB主机句柄
 * @param address 设备地址
 * @param device_handle 设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int usb_host_open_device(usb_handle_t handle, uint8_t address, usb_handle_t *device_handle);

/**
 * @brief 关闭USB设备
 * 
 * @param device_handle 设备句柄
 * @return int 0表示成功，非0表示失败
 */
int usb_host_close_device(usb_handle_t device_handle);

/**
 * @brief 获取连接的设备数量
 * 
 * @param handle USB主机句柄
 * @param count 设备数量指针
 * @return int 0表示成功，非0表示失败
 */
int usb_host_get_device_count(usb_handle_t handle, uint8_t *count);

/**
 * @brief 获取设备信息
 * 
 * @param device_handle 设备句柄
 * @param device_info 设备信息指针
 * @return int 0表示成功，非0表示失败
 */
int usb_host_get_device_info(usb_handle_t device_handle, usb_host_device_info_t *device_info);

#endif /* USB_API_H */