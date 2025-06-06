/**
 * @file stm32_platform.h
 * @brief STM32平台相关的定义
 *
 * 该头文件包含STM32平台相关的定义
 */

#ifndef STM32_PLATFORM_H
#define STM32_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/* STM32芯片系列定义 */
#define STM32_SERIES_F1    1    /**< STM32F1系列 */
#define STM32_SERIES_F2    2    /**< STM32F2系列 */
#define STM32_SERIES_F3    3    /**< STM32F3系列 */
#define STM32_SERIES_F4    4    /**< STM32F4系列 */
#define STM32_SERIES_F7    7    /**< STM32F7系列 */
#define STM32_SERIES_H7    8    /**< STM32H7系列 */

/* 设置当前使用的STM32系列 */
#define STM32_CURRENT_SERIES    STM32_SERIES_F4

/* 根据目标平台包含适当的STM32 HAL头文件 */
#if defined(TARGET_STM32)
#if (STM32_CURRENT_SERIES == STM32_SERIES_F4)
    #include "stm32f4xx_hal.h"
#elif (STM32_CURRENT_SERIES == STM32_SERIES_F7)
    #include "stm32f7xx_hal.h"
#endif
#endif

/* 根据芯片系列设置相关参数 */
#if (STM32_CURRENT_SERIES == STM32_SERIES_F4)
    #define STM32_CPU_TYPE      "STM32F407"
    #define STM32_FLASH_SIZE    (1024 * 1024)    /* 1MB Flash */
    #define STM32_RAM_SIZE      (192 * 1024)     /* 192KB RAM */
#elif (STM32_CURRENT_SERIES == STM32_SERIES_F7)
    #define STM32_CPU_TYPE      "STM32F767"
    #define STM32_FLASH_SIZE    (2048 * 1024)    /* 2MB Flash */
    #define STM32_RAM_SIZE      (512 * 1024)     /* 512KB RAM */
#else
    #error "Unsupported STM32 series"
#endif

/* STM32平台信息结构体 */
typedef struct {
    const char *cpu_type;       /**< CPU类型 */
    uint32_t cpu_clock;         /**< CPU时钟频率（Hz） */
    uint32_t flash_size;        /**< Flash大小（字节） */
    uint32_t ram_size;          /**< RAM大小（字节） */
} stm32_platform_info_t;

/**
 * @brief 系统时钟配置
 * 
 * @note 这是一个STM32平台特定的函数，不是平台抽象层的一部分
 */
static void SystemClock_Config(void);

#endif /* STM32_PLATFORM_H */
