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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @return api_status_t 操作状态
 */
api_status_t flash_init(flash_callback_t callback, void *arg, flash_handle_t *handle);

/**
 * @brief 去初始化Flash设备
 * 
 * @param handle Flash设备句柄
 * @return api_status_t 操作状态
 */
api_status_t flash_deinit(flash_handle_t handle);

/**
 * @brief 读取Flash数据
 * 
 * @param handle Flash设备句柄
 * @param addr Flash地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return api_status_t 操作状态
 */
api_status_t flash_read(flash_handle_t handle, uint32_t addr, void *data, uint32_t len);

/**
 * @brief 写入Flash数据
 * 
 * @param handle Flash设备句柄
 * @param addr Flash地址
 * @param data 数据缓冲区
 * @param len 写入长度
 * @return api_status_t 操作状态
 */
api_status_t flash_write(flash_handle_t handle, uint32_t addr, const void *data, uint32_t len);

/**
 * @brief 擦除Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param sector_addr 扇区地址
 * @return api_status_t 操作状态
 */
api_status_t flash_erase_sector(flash_handle_t handle, uint32_t sector_addr);

/**
 * @brief 擦除多个Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param start_sector 起始扇区地址
 * @param sector_count 扇区数量
 * @return api_status_t 操作状态
 */
api_status_t flash_erase_sectors(flash_handle_t handle, uint32_t start_sector, uint32_t sector_count);

/**
 * @brief 擦除整个Flash
 * 
 * @param handle Flash设备句柄
 * @return api_status_t 操作状态
 */
api_status_t flash_erase_chip(flash_handle_t handle);

/**
 * @brief 获取Flash状态
 * 
 * @param handle Flash设备句柄
 * @param status 状态指针
 * @return api_status_t 操作状态
 */
api_status_t flash_get_status(flash_handle_t handle, flash_status_t *status);

/**
 * @brief 获取Flash信息
 * 
 * @param handle Flash设备句柄
 * @param size Flash大小(字节)
 * @param sector_size 扇区大小(字节)
 * @param sector_count 扇区数量
 * @return api_status_t 操作状态
 */
api_status_t flash_get_info(flash_handle_t handle, uint32_t *size, uint32_t *sector_size, uint32_t *sector_count);

/**
 * @brief 锁定Flash
 * 
 * @param handle Flash设备句柄
 * @return api_status_t 操作状态
 */
api_status_t flash_lock(flash_handle_t handle);

/**
 * @brief 解锁Flash
 * 
 * @param handle Flash设备句柄
 * @return api_status_t 操作状态
 */
api_status_t flash_unlock(flash_handle_t handle);

/**
 * @brief 锁定特定Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param sector_addr 扇区地址
 * @return api_status_t 操作状态
 */
api_status_t flash_lock_sector(flash_handle_t handle, uint32_t sector_addr);

/**
 * @brief 解锁特定Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param sector_addr 扇区地址
 * @return api_status_t 操作状态
 */
api_status_t flash_unlock_sector(flash_handle_t handle, uint32_t sector_addr);

/**
 * @brief 获取Flash扇区保护状态
 * 
 * @param handle Flash设备句柄
 * @param sector_addr 扇区地址
 * @param is_protected 保护状态指针
 * @return api_status_t 操作状态
 */
api_status_t flash_is_sector_protected(flash_handle_t handle, uint32_t sector_addr, bool *is_protected);

/**
 * @brief 计算地址对应的扇区号
 * 
 * @param handle Flash设备句柄
 * @param addr Flash地址
 * @param sector_number 扇区号指针
 * @return api_status_t 操作状态
 */
api_status_t flash_get_sector_number(flash_handle_t handle, uint32_t addr, uint32_t *sector_number);

/**
 * @brief 获取扇区起始地址
 * 
 * @param handle Flash设备句柄
 * @param sector_number 扇区号
 * @param addr 地址指针
 * @return api_status_t 操作状态
 */
api_status_t flash_get_sector_address(flash_handle_t handle, uint32_t sector_number, uint32_t *addr);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_API_H */