/**
 * @file project_config.h
 * @brief 项目配置中心
 *
 * 本文件集中管理项目的所有配置选项，减少对CMake和平台特定配置的依赖
 */

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/*==========================
 * 编译器自动检测
 *==========================*/
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
    #define COMPILER_KEIL
#elif defined(__GNUC__)
    #define COMPILER_GCC
#elif defined(__ICCARM__)
    #define COMPILER_IAR
#endif

/*==========================
 * 平台配置
 *==========================*/
/* 支持的MCU平台 */
#define PLATFORM_STM32       1
#define PLATFORM_ESP32       2
#define PLATFORM_IT986XX     3
#define PLATFORM_FM33LC0XX   4

/* 选择当前平台 (可以在CMake或项目设置中覆盖) */
#ifndef TARGET_PLATFORM
    #define TARGET_PLATFORM PLATFORM_STM32
#endif

/*==========================
 * 基础功能配置
 *==========================*/
/* 错误处理配置 */
#define CONFIG_ERROR_HISTORY_SIZE       10      /* 错误历史记录大小 */
#define CONFIG_ERROR_INCLUDE_FILE_LINE   1      /* 是否在错误中包含文件和行号 */
#define CONFIG_ERROR_MAX_INFO_LEN       100     /* 错误信息最大长度 */

/* 驱动管理器配置 */
#define CONFIG_MAX_DRIVERS              50      /* 最大驱动数量 */
#define CONFIG_AUTO_DRIVER_REGISTER      1      /* 启用自动驱动注册 */
#define CONFIG_DRIVER_CATEGORY_SUPPORT   1      /* 启用驱动分类支持 */

/* 日志配置 */
#define CONFIG_LOG_ENABLED               1      /* 启用日志 */
#define CONFIG_LOG_LEVEL                 3      /* 日志级别: 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE */
#define CONFIG_LOG_COLORS                1      /* 启用彩色日志 */
#define CONFIG_LOG_TIMESTAMP             1      /* 在日志中包含时间戳 */

/* 内存管理配置 */
#define CONFIG_MEMORY_MANAGER_ENABLED    1      /* 启用内存管理器 */
#define CONFIG_MEMORY_STATS              1      /* 启用内存统计 */
#define CONFIG_MEMORY_POOL_SIZE       8192      /* 内存池大小 (如果使用) */

/*==========================
 * RTOS配置
 *==========================*/
/* 支持的RTOS类型 */
#define RTOS_NONE           0
#define RTOS_FREERTOS       1
#define RTOS_THREADX        2
#define RTOS_UCOS           3

/* 选择当前RTOS (可以在CMake或项目设置中覆盖) */
#ifndef USE_RTOS
    #define USE_RTOS RTOS_FREERTOS
#endif

/*==========================
 * 驱动支持配置
 *==========================*/
#define CONFIG_UART_ENABLED              1      /* 启用UART驱动 */
#define CONFIG_I2C_ENABLED               1      /* 启用I2C驱动 */
#define CONFIG_SPI_ENABLED               1      /* 启用SPI驱动 */
#define CONFIG_GPIO_ENABLED              1      /* 启用GPIO驱动 */
#define CONFIG_ADC_ENABLED               1      /* 启用ADC驱动 */
#define CONFIG_PWM_ENABLED               1      /* 启用PWM驱动 */
#define CONFIG_FLASH_ENABLED             1      /* 启用FLASH驱动 */
#define CONFIG_FILESYSTEM_ENABLED        1      /* 启用文件系统 */
#define CONFIG_POWER_MANAGEMENT_ENABLED  1      /* 启用电源管理 */
#define CONFIG_DISPLAY_ENABLED           1      /* 启用显示驱动 */
#define CONFIG_NETWORK_ENABLED           1      /* 启用网络功能 */

/*==========================
 * 设备树配置
 *==========================*/
#define CONFIG_DEVICE_TREE_ENABLED       1      /* 启用设备树 */
#define CONFIG_MAX_DEVICE_NODES         50      /* 最大设备节点数 */

/*==========================
 * 模块化构建支持
 *==========================*/
#define CONFIG_MODULE_SUPPORT            1      /* 启用模块支持 */
#define CONFIG_MAX_MODULES              20      /* 最大模块数量 */

/*==========================
 * 单元测试配置
 *==========================*/
#define CONFIG_UNIT_TEST_ENABLED         1      /* 启用单元测试框架 */
#define CONFIG_TEST_ASSERT_ENABLED       1      /* 启用测试断言 */
#define CONFIG_TEST_MOCK_ENABLED         1      /* 启用测试模拟 */

/* 可在CMake或平台特定文件中覆盖以上配置 */

#endif /* PROJECT_CONFIG_H */
