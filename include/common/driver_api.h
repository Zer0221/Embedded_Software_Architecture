/**
 * @file driver_api.h
 * @brief 驱动层通用接口定义
 *
 * 该头文件定义了驱动层的统一接口，使上层应用能够与底层驱动实现解耦
 */

#ifndef DRIVER_API_H
#define DRIVER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"

/* 通用错误码定义 */
#define DRIVER_OK            0    /**< 操作成功 */
#define DRIVER_ERROR        -1    /**< 一般错误 */
#define DRIVER_BUSY         -2    /**< 设备忙 */
#define DRIVER_TIMEOUT      -3    /**< 操作超时 */
#define DRIVER_INVALID_PARAM -4   /**< 无效参数 */
#define DRIVER_NOT_SUPPORTED -5   /**< 不支持的操作 */

/* 驱动类型定义 */
typedef enum {
    DRIVER_TYPE_GPIO = 0,        /**< GPIO驱动 */
    DRIVER_TYPE_UART,            /**< UART驱动 */
    DRIVER_TYPE_I2C,             /**< I2C驱动 */
    DRIVER_TYPE_SPI,             /**< SPI驱动 */
    DRIVER_TYPE_ADC,             /**< ADC驱动 */
    DRIVER_TYPE_PWM,             /**< PWM驱动 */
    DRIVER_TYPE_TIMER,           /**< 定时器驱动 */
    DRIVER_TYPE_FLASH,           /**< 闪存驱动 */
    DRIVER_TYPE_SDIO,            /**< SDIO驱动 */
    DRIVER_TYPE_USB,             /**< USB驱动 */
    DRIVER_TYPE_CAN,             /**< CAN驱动 */
    DRIVER_TYPE_WATCHDOG,        /**< 看门狗驱动 */
    DRIVER_TYPE_RTC,             /**< 实时时钟驱动 */
    DRIVER_TYPE_DMA,             /**< DMA驱动 */
    DRIVER_TYPE_CRYPTO,          /**< 加密驱动 */
    DRIVER_TYPE_SENSOR,          /**< 传感器驱动 */
    DRIVER_TYPE_DISPLAY,         /**< 显示驱动 */
    DRIVER_TYPE_AUDIO,           /**< 音频驱动 */
    DRIVER_TYPE_NETWORK,         /**< 网络驱动 */
    DRIVER_TYPE_FILESYSTEM,      /**< 文件系统驱动 */
    DRIVER_TYPE_POWER,           /**< 电源管理驱动 */
    DRIVER_TYPE_CUSTOM,          /**< 自定义驱动 */
    DRIVER_TYPE_MAX              /**< 驱动类型数量 */
} driver_type_t;

/* 驱动状态定义 */
typedef enum {
    DRIVER_STATUS_UNINITIALIZED = 0,  /**< 未初始化 */
    DRIVER_STATUS_INITIALIZED,         /**< 已初始化 */
    DRIVER_STATUS_RUNNING,             /**< 运行中 */
    DRIVER_STATUS_SUSPENDED,           /**< 已挂起 */
    DRIVER_STATUS_ERROR                /**< 错误状态 */
} driver_status_t;

/* 通用设备句柄定义 */
typedef void* driver_handle_t;

/**
 * @brief 驱动初始化函数类型
 */
typedef int (*driver_init_func_t)(void);

/**
 * @brief 驱动去初始化函数类型
 */
typedef int (*driver_deinit_func_t)(void);

/**
 * @brief 驱动开启函数类型
 */
typedef int (*driver_open_func_t)(driver_handle_t *handle);

/**
 * @brief 驱动关闭函数类型
 */
typedef int (*driver_close_func_t)(driver_handle_t handle);

/**
 * @brief 驱动读取函数类型
 */
typedef int (*driver_read_func_t)(driver_handle_t handle, void *buf, uint32_t len);

/**
 * @brief 驱动写入函数类型
 */
typedef int (*driver_write_func_t)(driver_handle_t handle, const void *buf, uint32_t len);

/**
 * @brief 驱动控制函数类型
 */
typedef int (*driver_ioctl_func_t)(driver_handle_t handle, uint32_t cmd, void *arg);

/**
 * @brief 驱动操作结构体
 */
typedef struct {
    driver_init_func_t    init;     /**< 初始化函数 */
    driver_deinit_func_t  deinit;   /**< 去初始化函数 */
    driver_open_func_t    open;     /**< 打开函数 */
    driver_close_func_t   close;    /**< 关闭函数 */
    driver_read_func_t    read;     /**< 读取函数 */
    driver_write_func_t   write;    /**< 写入函数 */
    driver_ioctl_func_t   ioctl;    /**< 控制函数 */
} driver_ops_t;

/**
 * @brief 驱动信息结构体
 */
typedef struct {
    const char*      name;          /**< 驱动名称 */
    const char*      description;   /**< 驱动描述 */
    const char*      version;       /**< 驱动版本 */
    driver_type_t    type;          /**< 驱动类型 */
    const void*      interface;     /**< 驱动接口 */
    uint32_t         capabilities;  /**< 驱动能力标志 */
    driver_status_t  status;        /**< 驱动状态 */
    driver_init_func_t    init;     /**< 初始化函数 */
    driver_deinit_func_t  deinit;   /**< 去初始化函数 */
    void*            private_data;  /**< 私有数据 */
#endif /* DRIVER_API_H */

#endif /* DRIVER_API_H */
