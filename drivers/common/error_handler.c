/**
 * @file error_handler.c
 * @brief 错误处理接口实现
 *
 * 该文件实现了统一的错误处理接口，提供了错误码处理和错误日志功能
 */

#include "common/error_api.h"
#include "common/log_api.h"
#include <stdio.h>
#include <string.h>

/* 错误信息存储 */
static char error_message[256];
static uint32_t last_error_code = 0;

/**
 * @brief 设置错误码和错误信息
 *
 * @param error_code 错误码
 * @param message 错误信息
 */
void set_error(uint32_t error_code, const char* message)
{
    last_error_code = error_code;
    
    if (message) {
        strncpy(error_message, message, sizeof(error_message) - 1);
        error_message[sizeof(error_message) - 1] = '\0';
    } else {
        error_message[0] = '\0';
    }
    
    /* 记录错误日志 */
    log_error("错误发生: [0x%08X] %s", error_code, message ? message : "未知错误");
}

/**
 * @brief 获取最后一次错误码
 *
 * @return 错误码
 */
uint32_t get_last_error(void)
{
    return last_error_code;
}

/**
 * @brief 获取错误信息
 *
 * @return 错误信息字符串
 */
const char* get_error_message(void)
{
    return error_message;
}

/**
 * @brief 清除错误状态
 */
void clear_error(void)
{
    last_error_code = 0;
    error_message[0] = '\0';
}

/**
 * @brief 根据错误码获取模块名称
 *
 * @param error_code 错误码
 * @return 模块名称字符串
 */
const char* get_error_module_name(uint32_t error_code)
{
    uint32_t module = error_code & 0xFF000000;
    
    switch (module) {
        case ERROR_MODULE_PLATFORM: return "平台";
        case ERROR_MODULE_RTOS:     return "RTOS";
        case ERROR_MODULE_DRIVER:   return "驱动";
        case ERROR_MODULE_I2C:      return "I2C";
        case ERROR_MODULE_UART:     return "UART";
        case ERROR_MODULE_SPI:      return "SPI";
        case ERROR_MODULE_GPIO:     return "GPIO";
        case ERROR_MODULE_ADC:      return "ADC";
        case ERROR_MODULE_PWM:      return "PWM";
        case ERROR_MODULE_POWER:    return "电源";
        case ERROR_MODULE_APP:      return "应用";
        default:                    return "未知";
    }
}

/**
 * @brief 根据错误码获取错误类型名称
 *
 * @param error_code 错误码
 * @return 错误类型名称字符串
 */
const char* get_error_type_name(uint32_t error_code)
{
    uint32_t type = error_code & 0x00FF0000;
    
    switch (type) {
        case ERROR_TYPE_NONE:     return "无错误";
        case ERROR_TYPE_INIT:     return "初始化错误";
        case ERROR_TYPE_PARAM:    return "参数错误";
        case ERROR_TYPE_TIMEOUT:  return "超时错误";
        case ERROR_TYPE_HARDWARE: return "硬件错误";
        case ERROR_TYPE_RESOURCE: return "资源错误";
        case ERROR_TYPE_STATE:    return "状态错误";
        case ERROR_TYPE_INTERNAL: return "内部错误";
        default:                  return "未知错误";
    }
}

/**
 * @brief 格式化错误信息
 *
 * @param error_code 错误码
 * @param buffer 格式化后的错误信息存储缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回0，失败返回负值
 */
int format_error(uint32_t error_code, char* buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    const char* module = get_error_module_name(error_code);
    const char* type = get_error_type_name(error_code);
    uint32_t code = error_code & 0x0000FFFF;
    
    snprintf(buffer, buffer_size, "[%s:%s:0x%04X] %s", 
             module, type, code, error_message);
    
    return 0;
}
