/**
 * @file fm33lc0xx_platform.h
 * @brief FM33LC0xx平台适配层头文件
 *
 * 该头文件定义了FM33LC0xx平台的硬件抽象层接口
 */

#ifndef FM33LC0XX_PLATFORM_H
#define FM33LC0XX_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include "platform_api.h"

/**
 * @brief 初始化FM33LC0xx平台
 * 
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_init(void);

/**
 * @brief 去初始化FM33LC0xx平台
 * 
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_deinit(void);

/**
 * @brief 获取FM33LC0xx平台信息
 * 
 * @param info 平台信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_info(platform_info_t *info);

/**
 * @brief FM33LC0xx平台睡眠
 * 
 * @param ms 睡眠时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_sleep(uint32_t ms);

/**
 * @brief FM33LC0xx平台复位
 * 
 * @param type 复位类型
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_reset(platform_reset_type_t type);

/**
 * @brief 获取FM33LC0xx平台运行时间
 * 
 * @param uptime 运行时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_uptime(uint32_t *uptime);

/**
 * @brief 获取FM33LC0xx平台唯一ID
 * 
 * @param id ID缓冲区
 * @param len ID缓冲区长度
 * @return int 实际ID长度，负值表示错误
 */
int fm33lc0xx_platform_get_unique_id(uint8_t *id, uint32_t len);

/**
 * @brief 获取FM33LC0xx平台随机数
 * 
 * @param data 随机数缓冲区
 * @param len 随机数长度
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_random(uint8_t *data, uint32_t len);

/**
 * @brief 获取FM33LC0xx平台闪存信息
 * 
 * @param info 闪存信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_flash_info(platform_flash_info_t *info);

/**
 * @brief 获取FM33LC0xx平台RAM信息
 * 
 * @param info RAM信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_ram_info(platform_ram_info_t *info);

#endif /* FM33LC0XX_PLATFORM_H */
