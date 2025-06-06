/**
 * @file config.h
 * @brief 项目配置头文件
 *
 * 该头文件包含项目的全局配置选项和功能开关
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 硬件平台选择 */
#define PLATFORM_STM32F4    1
#define PLATFORM_STM32F7    2
#define PLATFORM_ESP32      3
#define PLATFORM_NRF52      4

/* 当前使用的硬件平台 */
#define CURRENT_PLATFORM    PLATFORM_STM32F4

/* RTOS选择 */
#define RTOS_NONE           0  /* 裸机模式，不使用RTOS */
#define RTOS_FREERTOS       1
#define RTOS_UCOS           2
#define RTOS_THREADX        3

/* 当前使用的RTOS */
#define CURRENT_RTOS        RTOS_FREERTOS

/* 调试配置 */
#define DEBUG_LEVEL_NONE    0  /* 不输出调试信息 */
#define DEBUG_LEVEL_ERROR   1  /* 只输出错误信息 */
#define DEBUG_LEVEL_WARN    2  /* 输出警告和错误信息 */
#define DEBUG_LEVEL_INFO    3  /* 输出一般信息、警告和错误信息 */
#define DEBUG_LEVEL_DEBUG   4  /* 输出调试信息、一般信息、警告和错误信息 */
#define DEBUG_LEVEL_VERBOSE 5  /* 输出所有信息 */

/* 当前调试级别 */
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_INFO

/* 外设使能开关 */
#define ENABLE_UART         1  /* 启用UART */
#define ENABLE_I2C          1  /* 启用I2C */
#define ENABLE_SPI          1  /* 启用SPI */
#define ENABLE_GPIO         1  /* 启用GPIO */
#define ENABLE_ADC          1  /* 启用ADC */
#define ENABLE_PWM          1  /* 启用PWM */

/* UART配置 */
#define UART_RX_BUF_SIZE    256  /* UART接收缓冲区大小 */
#define UART_TX_BUF_SIZE    256  /* UART发送缓冲区大小 */
#define UART_MAX_INSTANCE   4    /* 最大支持的UART实例数 */

/* I2C配置 */
#define I2C_MAX_INSTANCE    3    /* 最大支持的I2C实例数 */

/* SPI配置 */
#define SPI_MAX_INSTANCE    3    /* 最大支持的SPI实例数 */

/* RTOS配置 */
#define RTOS_MAX_TASKS      10   /* 最大任务数 */
#define RTOS_TICK_MS        1    /* RTOS滴答定时器周期（毫秒） */

/* 内存分配配置 */
#define USE_STATIC_MEMORY   0    /* 是否使用静态内存分配 */
#define STATIC_HEAP_SIZE    (32 * 1024)  /* 静态堆大小 */

/* 其他配置 */
#define USE_ASSERT          1    /* 使用断言 */
#define USE_WATCHDOG        1    /* 使用看门狗 */

/* 根据平台选择硬件相关头文件 */
#if (CURRENT_PLATFORM == PLATFORM_STM32F4)
    #include "stm32f4xx.h"
    #include "stm32f4xx_hal.h"
    #include "stm32_platform.h"
#elif (CURRENT_PLATFORM == PLATFORM_STM32F7)
    #include "stm32f7xx.h"
    #include "stm32f7xx_hal.h"
    #include "stm32_platform.h"
#elif (CURRENT_PLATFORM == PLATFORM_ESP32)
    #include "esp_system.h"
    #include "esp_platform.h"
    #include "esp32_platform.h"
#elif (CURRENT_PLATFORM == PLATFORM_NRF52)
    #include "nrf52.h"
    #include "nrf_platform.h"
#else
    #error "Unsupported platform"
#endif

/* 根据RTOS选择相关头文件 */
#if (CURRENT_RTOS == RTOS_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    #include "queue.h"
    #include "timers.h"
#elif (CURRENT_RTOS == RTOS_UCOS)
    #include "os.h"
#elif (CURRENT_RTOS == RTOS_THREADX)
    #include "tx_api.h"
#elif (CURRENT_RTOS == RTOS_NONE)
    /* 裸机模式，不包含RTOS头文件 */
#else
    #error "Unsupported RTOS"
#endif

/* 调试日志宏定义 */
#if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)
    #define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_ERROR(fmt, ...)
#endif

#if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_WARN)
    #define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_WARN(fmt, ...)
#endif

#if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO)
    #define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_INFO(fmt, ...)
#endif

#if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG)
    #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
#endif

#if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE)
    #define LOG_VERBOSE(fmt, ...) printf("[VERBOSE] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_VERBOSE(fmt, ...)
#endif

/* 断言宏定义 */
#if USE_ASSERT
    #define ASSERT(expr) \
        do { \
            if (!(expr)) { \
                LOG_ERROR("Assertion failed: %s, file %s, line %d", #expr, __FILE__, __LINE__); \
                while (1); \
            } \
        } while (0)
#else
    #define ASSERT(expr)
#endif

#endif /* CONFIG_H */
