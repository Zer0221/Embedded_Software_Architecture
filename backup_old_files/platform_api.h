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

/**
 * @brief 平台初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_init(void);

/**
 * @brief 平台去初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_deinit(void);

/**
 * @brief 获取平台信息
 * 
 * @param info 平台信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int platform_get_info(void *info);

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
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t platform_get_time_ms(void);

#endif /* PLATFORM_API_H */
