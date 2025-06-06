/**
 * @file flash_api.h
 * @brief Flash存储接口抽象层定义
 *
 * 该头文件定义了Flash存储的统一抽象接口，使上层应用能够与底层Flash硬件实现解耦
 */

#ifndef FLASH_API_H
#define FLASH_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* Flash操作状态 */
typedef enum {
    FLASH_STATUS_IDLE,       /**< 空闲状态 */
    FLASH_STATUS_BUSY,       /**< 忙状态 */
    FLASH_STATUS_COMPLETE,   /**< 操作完成 */
    FLASH_STATUS_ERROR,      /**< 操作错误 */
    FLASH_STATUS_TIMEOUT     /**< 操作超时 */
} flash_status_t;

/* Flash回调函数类型 */
typedef void (*flash_callback_t)(void *arg, flash_status_t status);

/* Flash设备句柄 */
typedef driver_handle_t flash_handle_t;

/**
 * @brief 初始化Flash设备
 * 
 * @param callback Flash操作完成回调函数
 * @param arg 传递给回调函数的参数
 * @param handle Flash设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int flash_init(flash_callback_t callback, void *arg, flash_handle_t *handle);

/**
 * @brief 去初始化Flash设备
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_deinit(flash_handle_t handle);

/**
 * @brief 读取Flash数据
 * 
 * @param handle Flash设备句柄
 * @param address 起始地址
 * @param data 数据缓冲区
 * @param size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int flash_read(flash_handle_t handle, uint32_t address, void *data, uint32_t size);

/**
 * @brief 写入Flash数据
 * 
 * @param handle Flash设备句柄
 * @param address 起始地址
 * @param data 数据缓冲区
 * @param size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int flash_write(flash_handle_t handle, uint32_t address, const void *data, uint32_t size);

/**
 * @brief 擦除Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param sector_address 扇区地址
 * @return int 0表示成功，非0表示失败
 */
int flash_erase_sector(flash_handle_t handle, uint32_t sector_address);

/**
 * @brief 擦除Flash块
 * 
 * @param handle Flash设备句柄
 * @param block_address 块地址
 * @return int 0表示成功，非0表示失败
 */
int flash_erase_block(flash_handle_t handle, uint32_t block_address);

/**
 * @brief 获取Flash状态
 * 
 * @param handle Flash设备句柄
 * @param status Flash状态指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_status(flash_handle_t handle, flash_status_t *status);

/**
 * @brief 获取Flash扇区大小
 * 
 * @param handle Flash设备句柄
 * @param sector_size 扇区大小指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_sector_size(flash_handle_t handle, uint32_t *sector_size);

/**
 * @brief 获取Flash块大小
 * 
 * @param handle Flash设备句柄
 * @param block_size 块大小指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_block_size(flash_handle_t handle, uint32_t *block_size);

/**
 * @brief 获取Flash总大小
 * 
 * @param handle Flash设备句柄
 * @param total_size Flash总大小指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_total_size(flash_handle_t handle, uint32_t *total_size);

/**
 * @brief 设置Flash保护
 * 
 * @param handle Flash设备句柄
 * @param start_address 起始地址
 * @param end_address 结束地址
 * @param enable 是否启用保护
 * @return int 0表示成功，非0表示失败
 */
int flash_set_protection(flash_handle_t handle, uint32_t start_address, uint32_t end_address, bool enable);

/**
 * @brief 获取Flash保护状态
 * 
 * @param handle Flash设备句柄
 * @param address 地址
 * @param is_protected 保护状态指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_protection(flash_handle_t handle, uint32_t address, bool *is_protected);

/**
 * @brief 锁定Flash
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_lock(flash_handle_t handle);

/**
 * @brief 解锁Flash
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_unlock(flash_handle_t handle);

#endif /* FLASH_API_H */
