/**
 * @file error_handling.h
 * @brief 错误处理接口定义
 *
 * 该头文件定义了错误处理相关的宏、类型和函数，提供统一的错误处理机制
 */

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 基本错误码定义 */
typedef enum {
    ERROR_NONE = 0,                /**< 无错误 */
    ERROR_GENERAL = -1,            /**< 一般错误 */
    ERROR_INVALID_PARAM = -2,      /**< 无效参数 */
    ERROR_NOT_INITIALIZED = -3,    /**< 未初始化 */
    ERROR_NOT_SUPPORTED = -4,      /**< 不支持的操作 */
    ERROR_TIMEOUT = -5,            /**< 操作超时 */
    ERROR_BUSY = -6,               /**< 设备忙 */
    ERROR_MEMORY = -7,             /**< 内存错误 */
    ERROR_IO = -8,                 /**< IO错误 */
    ERROR_COMMUNICATION = -9,      /**< 通信错误 */
    ERROR_HARDWARE = -10,          /**< 硬件错误 */
    ERROR_OVERFLOW = -11,          /**< 溢出错误 */
    ERROR_UNDERFLOW = -12,         /**< 下溢错误 */
    ERROR_NOT_FOUND = -13,         /**< 未找到 */
    ERROR_ALREADY_EXISTS = -14,    /**< 已存在 */
    ERROR_PERMISSION = -15,        /**< 权限错误 */
    ERROR_FULL = -16,              /**< 满错误 */
    ERROR_EMPTY = -17,             /**< 空错误 */
    ERROR_CRC = -18,               /**< CRC错误 */
    ERROR_AUTH = -19,              /**< 认证错误 */
    ERROR_UNKNOWN = -20            /**< 未知错误 */
} error_code_t;

/* 错误信息结构体 */
typedef struct {
    error_code_t code;                     /**< 错误码 */
    char description[CONFIG_ERROR_MAX_INFO_LEN]; /**< 错误描述 */
#if CONFIG_ERROR_INCLUDE_FILE_LINE
    const char *file;                      /**< 发生错误的文件 */
    int line;                              /**< 发生错误的行号 */
    const char *func;                      /**< 发生错误的函数 */
#endif
    uint32_t timestamp;                    /**< 错误时间戳 */
} error_info_t;

/**
 * @brief 错误处理初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_init(void);

/**
 * @brief 设置当前错误
 * 
 * @param code 错误码
 * @param description 错误描述
 * @param file 文件名（可选）
 * @param line 行号（可选）
 * @param func 函数名（可选）
 * @return int 0表示成功，非0表示失败
 */
int error_set(error_code_t code, const char *description, 
               const char *file, int line, const char *func);

/**
 * @brief 获取最后一个错误信息
 * 
 * @param info 错误信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int error_get_last(error_info_t *info);

/**
 * @brief 获取错误历史
 * 
 * @param history 错误历史数组
 * @param size 数组大小
 * @param count 实际错误数量
 * @return int 0表示成功，非0表示失败
 */
int error_get_history(error_info_t *history, uint8_t size, uint8_t *count);

/**
 * @brief 清除错误历史
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_clear_history(void);

/**
 * @brief 获取错误字符串描述
 * 
 * @param code 错误码
 * @return const char* 错误描述字符串
 */
const char *error_to_str(error_code_t code);

/* 错误处理宏 */
#if CONFIG_ERROR_INCLUDE_FILE_LINE
    #define ERROR_SET(code, desc) error_set(code, desc, __FILE__, __LINE__, __func__)
#else
    #define ERROR_SET(code, desc) error_set(code, desc, NULL, 0, NULL)
#endif

/* 错误检查宏 */
#define ERROR_CHECK(expr) \
    do { \
        error_code_t __err = (expr); \
        if (__err != ERROR_NONE) { \
            ERROR_SET(__err, #expr); \
            return __err; \
        } \
    } while(0)

/* 参数检查宏 */
#define PARAM_CHECK(cond, ret) \
    do { \
        if (!(cond)) { \
            ERROR_SET(ERROR_INVALID_PARAM, #cond); \
            return ret; \
        } \
    } while(0)

/* 条件检查宏 */
#define COND_CHECK(cond, err, ret) \
    do { \
        if (!(cond)) { \
            ERROR_SET(err, #cond); \
            return ret; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_H */
