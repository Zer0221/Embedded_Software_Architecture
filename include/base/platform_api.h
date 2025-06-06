/**
 * @file platform_api.h
 * @brief 平台抽象层接口定义
 *
 * 该头文件定义了平台抽象层的统一接口，使上层应用能够与底层硬件平台解耦
 */

#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 平台信息结构体 */
typedef struct {
    char platform_name[32];       /**< 平台名称 */
    char mcu_name[32];            /**< MCU名称 */
    uint32_t cpu_freq_hz;         /**< CPU频率(Hz) */
    uint32_t flash_size;          /**< Flash大小(字节) */
    uint32_t ram_size;            /**< RAM大小(字节) */
    char compiler_version[32];    /**< 编译器版本 */
    char build_date[16];          /**< 构建日期 */
    char build_time[16];          /**< 构建时间 */
} platform_info_t;

/**
 * @brief 平台初始化
 * 
 * @return api_status_t 操作状态
 */
api_status_t platform_init(void);

/**
 * @brief 平台去初始化
 * 
 * @return api_status_t 操作状态
 */
api_status_t platform_deinit(void);

/**
 * @brief 获取平台信息
 * 
 * @param info 平台信息结构体指针
 * @return api_status_t 操作状态
 */
api_status_t platform_get_info(platform_info_t *info);

/**
 * @brief 平台延时函数（毫秒）
 * 
 * @param ms 延时毫秒数
 */
void platform_delay_ms(uint32_t ms);

/**
 * @brief 平台延时函数（微秒）
 * 
 * @param us 延时微秒数
 */
void platform_delay_us(uint32_t us);

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间
 */
uint32_t platform_get_time_ms(void);

/**
 * @brief 获取系统运行时间（微秒）
 * 
 * @return uint64_t 系统运行时间
 */
uint64_t platform_get_time_us(void);

/**
 * @brief 系统复位
 */
void platform_reset(void);

/**
 * @brief 进入系统睡眠模式
 * 
 * @param sleep_mode 睡眠模式
 * @return api_status_t 操作状态
 */
api_status_t platform_enter_sleep(uint8_t sleep_mode);

/**
 * @brief 获取芯片唯一ID
 * 
 * @param id_buffer ID缓冲区
 * @param buffer_size 缓冲区大小
 * @return api_status_t 操作状态
 */
api_status_t platform_get_unique_id(uint8_t *id_buffer, uint8_t buffer_size);

/**
 * @brief 获取平台版本信息
 * 
 * @param version 版本信息缓冲区
 * @param buffer_size 缓冲区大小
 * @return api_status_t 操作状态
 */
api_status_t platform_get_version(char *version, uint8_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_API_H */