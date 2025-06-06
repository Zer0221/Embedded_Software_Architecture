/**
 * @file error_api.h
 * @brief 错误处理接口定义
 *
 * 该头文件定义了统一的错误处理接口，提供了错误码定义、错误处理和错误日志功能
 */

#ifndef ERROR_API_H
#define ERROR_API_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/* 错误码模块标识，高8位 */
#define ERROR_MODULE_PLATFORM    (0x01 << 24)  /**< 平台模块错误 */
#define ERROR_MODULE_RTOS        (0x02 << 24)  /**< RTOS模块错误 */
#define ERROR_MODULE_DRIVER      (0x03 << 24)  /**< 驱动模块错误 */
#define ERROR_MODULE_I2C         (0x04 << 24)  /**< I2C模块错误 */
#define ERROR_MODULE_UART        (0x05 << 24)  /**< UART模块错误 */
#define ERROR_MODULE_SPI         (0x06 << 24)  /**< SPI模块错误 */
#define ERROR_MODULE_GPIO        (0x07 << 24)  /**< GPIO模块错误 */
#define ERROR_MODULE_ADC         (0x08 << 24)  /**< ADC模块错误 */
#define ERROR_MODULE_PWM         (0x09 << 24)  /**< PWM模块错误 */
#define ERROR_MODULE_POWER       (0x0A << 24)  /**< 电源管理模块错误 */
#define ERROR_MODULE_APP         (0x0F << 24)  /**< 应用模块错误 */

/* 错误类型，中间8位 */
#define ERROR_TYPE_NONE          (0x00 << 16)  /**< 无错误 */
#define ERROR_TYPE_INIT          (0x01 << 16)  /**< 初始化错误 */
#define ERROR_TYPE_PARAM         (0x02 << 16)  /**< 参数错误 */
#define ERROR_TYPE_TIMEOUT       (0x03 << 16)  /**< 超时错误 */
#define ERROR_TYPE_RESOURCE      (0x04 << 16)  /**< 资源错误 */
#define ERROR_TYPE_HARDWARE      (0x05 << 16)  /**< 硬件错误 */
#define ERROR_TYPE_COMMUNICATION (0x06 << 16)  /**< 通信错误 */
#define ERROR_TYPE_STATE         (0x07 << 16)  /**< 状态错误 */
#define ERROR_TYPE_MEMORY        (0x08 << 16)  /**< 内存错误 */
#define ERROR_TYPE_OVERFLOW      (0x09 << 16)  /**< 溢出错误 */
#define ERROR_TYPE_UNDERFLOW     (0x0A << 16)  /**< 下溢错误 */
#define ERROR_TYPE_PERMISSION    (0x0B << 16)  /**< 权限错误 */
#define ERROR_TYPE_NOT_SUPPORTED (0x0C << 16)  /**< 不支持的操作 */
#define ERROR_TYPE_NOT_FOUND     (0x0D << 16)  /**< 未找到 */
#define ERROR_TYPE_BUSY          (0x0E << 16)  /**< 设备忙 */
#define ERROR_TYPE_GENERAL       (0x0F << 16)  /**< 一般错误 */

/* 错误严重程度，低8位 */
#define ERROR_SEVERITY_INFO      0x00  /**< 信息 */
#define ERROR_SEVERITY_WARNING   0x01  /**< 警告 */
#define ERROR_SEVERITY_ERROR     0x02  /**< 错误 */
#define ERROR_SEVERITY_CRITICAL  0x03  /**< 严重错误 */
#define ERROR_SEVERITY_FATAL     0x04  /**< 致命错误 */

/* 通用错误码定义 */
#define ERROR_OK                 0x00000000  /**< 无错误 */
#define ERROR_GENERAL            0x0F0F0002  /**< 一般错误 */

/* 错误回调函数类型 */
typedef void (*error_callback_t)(uint32_t error_code, const char *file, uint32_t line, void *user_data);

/**
 * @brief 错误处理初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_init(void);

/**
 * @brief 错误处理去初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_deinit(void);

/**
 * @brief 注册错误处理回调函数
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int error_register_callback(error_callback_t callback, void *user_data);

/**
 * @brief 取消注册错误处理回调函数
 * 
 * @param callback 回调函数
 * @return int 0表示成功，非0表示失败
 */
int error_unregister_callback(error_callback_t callback);

/**
 * @brief 报告错误
 * 
 * @param error_code 错误码
 * @param file 文件名
 * @param line 行号
 * @return int 0表示成功，非0表示失败
 */
int error_report(uint32_t error_code, const char *file, uint32_t line);

/**
 * @brief 获取错误描述
 * 
 * @param error_code 错误码
 * @return const char* 错误描述字符串
 */
const char *error_get_description(uint32_t error_code);

/**
 * @brief 获取错误统计信息
 * 
 * @param module 模块标识，0表示所有模块
 * @param count 错误计数
 * @return int 0表示成功，非0表示失败
 */
int error_get_statistics(uint32_t module, uint32_t *count);

/**
 * @brief 清除错误统计信息
 * 
 * @param module 模块标识，0表示所有模块
 * @return int 0表示成功，非0表示失败
 */
int error_clear_statistics(uint32_t module);

/* 错误报告宏 */
#define REPORT_ERROR(code) error_report(code, __FILE__, __LINE__)

/* 错误检查宏 */
#define CHECK_ERROR(expr, code) \
    do { \
        if (!(expr)) { \
            error_report(code, __FILE__, __LINE__); \
            return code; \
        } \
    } while (0)

#endif /* ERROR_API_H */
