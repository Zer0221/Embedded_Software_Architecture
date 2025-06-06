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

/* 通用错误码定义 */
#define DRIVER_OK            0    /**< 操作成功 */
#define DRIVER_ERROR        -1    /**< 一般错误 */
#define DRIVER_BUSY         -2    /**< 设备忙 */
#define DRIVER_TIMEOUT      -3    /**< 操作超时 */
#define DRIVER_INVALID_PARAM -4   /**< 无效参数 */
#define DRIVER_NOT_SUPPORTED -5   /**< 不支持的操作 */

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

#endif /* DRIVER_API_H */
