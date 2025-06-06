/**
 * @file esp32_platform.h
 * @brief ESP32平台相关的定义
 *
 * 该头文件包含ESP32平台相关的定义
 */

#ifndef ESP32_PLATFORM_H
#define ESP32_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

/* ESP32芯片系列定义 */
#define ESP32_SERIES_BASE    1    /**< 基础ESP32系列 */
#define ESP32_SERIES_S2      2    /**< ESP32-S2系列 */
#define ESP32_SERIES_S3      3    /**< ESP32-S3系列 */
#define ESP32_SERIES_C3      4    /**< ESP32-C3系列 */

/* 设置当前使用的ESP32系列 */
#define ESP32_CURRENT_SERIES    ESP32_SERIES_BASE

/* 根据芯片系列设置相关参数 */
#if (ESP32_CURRENT_SERIES == ESP32_SERIES_BASE)
    #define ESP32_CPU_TYPE      "ESP32"
    #define ESP32_FLASH_SIZE    (4 * 1024 * 1024)    /* 4MB Flash */
    #define ESP32_RAM_SIZE      (520 * 1024)         /* 520KB RAM */
#elif (ESP32_CURRENT_SERIES == ESP32_SERIES_S3)
    #define ESP32_CPU_TYPE      "ESP32-S3"
    #define ESP32_FLASH_SIZE    (8 * 1024 * 1024)    /* 8MB Flash */
    #define ESP32_RAM_SIZE      (512 * 1024)         /* 512KB RAM */
#else
    #error "Unsupported ESP32 series"
#endif

/* ESP32平台信息结构体 */
typedef struct {
    const char *cpu_type;       /**< CPU类型 */
    uint32_t cpu_clock;         /**< CPU时钟频率（Hz） */
    uint32_t flash_size;        /**< Flash大小（字节） */
    uint32_t ram_size;          /**< RAM大小（字节） */
} esp32_platform_info_t;

/**
 * @brief 系统时钟配置
 * 
 * @note 这是一个ESP32平台特定的函数，不是平台抽象层的一部分
 */
void esp_system_init(void);

#endif /* ESP32_PLATFORM_H */
