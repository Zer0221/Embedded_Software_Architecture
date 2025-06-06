/**
 * @file error.c
 * @brief 错误处理接口实现
 *
 * 该文件实现了统一的错误处理接口，提供了错误处理和错误日志功能
 */

#include "common/error_api.h"
#include <stdio.h>
#include <string.h>

#if (CURRENT_RTOS != RTOS_NONE)
#include "common/rtos_api.h"
#endif

/* 错误回调函数最大数量 */
#define MAX_ERROR_CALLBACKS  5

/* 错误描述结构体 */
typedef struct {
    uint32_t code;           /**< 错误码 */
    const char *description; /**< 错误描述 */
} error_description_t;

/* 错误统计结构体 */
typedef struct {
    uint32_t module;         /**< 模块ID */
    uint32_t count;          /**< 错误计数 */
} error_statistics_t;

/* 错误回调信息结构体 */
typedef struct {
    error_callback_t callback; /**< 回调函数 */
    void *user_data;           /**< 用户数据 */
    bool used;                 /**< 是否已使用 */
} error_callback_info_t;

/* 全局变量 */
static error_callback_info_t g_error_callbacks[MAX_ERROR_CALLBACKS] = {0};
static error_statistics_t g_error_statistics[16] = {0};  /* 最多统计16个模块 */
static uint32_t g_statistics_count = 0;

#if (CURRENT_RTOS != RTOS_NONE)
static rtos_mutex_t g_error_mutex = NULL;
#endif

/* 错误描述表 */
static const error_description_t g_error_descriptions[] = {
    { ERROR_OK, "No error" },
    { ERROR_GENERAL, "General error" },
    { ERROR_MODULE_PLATFORM | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR, "Platform initialization error" },
    { ERROR_MODULE_RTOS | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR, "RTOS initialization error" },
    { ERROR_MODULE_DRIVER | ERROR_TYPE_PARAM | ERROR_SEVERITY_WARNING, "Driver invalid parameter warning" },
    { ERROR_MODULE_I2C | ERROR_TYPE_TIMEOUT | ERROR_SEVERITY_ERROR, "I2C timeout error" },
    { ERROR_MODULE_UART | ERROR_TYPE_OVERFLOW | ERROR_SEVERITY_WARNING, "UART buffer overflow warning" },
    { ERROR_MODULE_SPI | ERROR_TYPE_HARDWARE | ERROR_SEVERITY_ERROR, "SPI hardware error" },
    { ERROR_MODULE_GPIO | ERROR_TYPE_STATE | ERROR_SEVERITY_WARNING, "GPIO invalid state warning" },
    { ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_CRITICAL, "Power resource critical error" },
    { 0, NULL }  /* 结束标记 */
};

/**
 * @brief 错误处理初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_init(void)
{
    /* 初始化回调数组 */
    memset(g_error_callbacks, 0, sizeof(g_error_callbacks));
    
    /* 初始化统计信息 */
    memset(g_error_statistics, 0, sizeof(g_error_statistics));
    g_statistics_count = 0;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 创建互斥锁 */
    if (rtos_mutex_create(&g_error_mutex) != 0) {
        return -1;
    }
#endif
    
    return 0;
}

/**
 * @brief 错误处理去初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int error_deinit(void)
{
#if (CURRENT_RTOS != RTOS_NONE)
    /* 删除互斥锁 */
    if (g_error_mutex != NULL) {
        rtos_mutex_delete(g_error_mutex);
        g_error_mutex = NULL;
    }
#endif
    
    return 0;
}

/**
 * @brief 注册错误处理回调函数
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int error_register_callback(error_callback_t callback, void *user_data)
{
    int i;
    int ret = -1;
    
    if (callback == NULL) {
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_error_mutex, UINT32_MAX);
#endif
    
    /* 查找可用槽位 */
    for (i = 0; i < MAX_ERROR_CALLBACKS; i++) {
        if (!g_error_callbacks[i].used) {
            g_error_callbacks[i].callback = callback;
            g_error_callbacks[i].user_data = user_data;
            g_error_callbacks[i].used = true;
            ret = 0;
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_error_mutex);
#endif
    
    return ret;
}

/**
 * @brief 取消注册错误处理回调函数
 * 
 * @param callback 回调函数
 * @return int 0表示成功，非0表示失败
 */
int error_unregister_callback(error_callback_t callback)
{
    int i;
    int ret = -1;
    
    if (callback == NULL) {
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_error_mutex, UINT32_MAX);
#endif
    
    /* 查找回调函数 */
    for (i = 0; i < MAX_ERROR_CALLBACKS; i++) {
        if (g_error_callbacks[i].used && g_error_callbacks[i].callback == callback) {
            g_error_callbacks[i].used = false;
            ret = 0;
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_error_mutex);
#endif
    
    return ret;
}

/**
 * @brief 更新错误统计信息
 * 
 * @param module 模块ID
 */
static void update_error_statistics(uint32_t module)
{
    int i;
    
    /* 模块ID取高8位 */
    module &= 0xFF000000;
    
    /* 查找模块统计信息 */
    for (i = 0; i < g_statistics_count; i++) {
        if (g_error_statistics[i].module == module) {
            g_error_statistics[i].count++;
            return;
        }
    }
    
    /* 模块不存在，添加新的统计信息 */
    if (g_statistics_count < sizeof(g_error_statistics) / sizeof(g_error_statistics[0])) {
        g_error_statistics[g_statistics_count].module = module;
        g_error_statistics[g_statistics_count].count = 1;
        g_statistics_count++;
    }
}

/**
 * @brief 报告错误
 * 
 * @param error_code 错误码
 * @param file 文件名
 * @param line 行号
 * @return int 0表示成功，非0表示失败
 */
int error_report(uint32_t error_code, const char *file, uint32_t line)
{
    int i;
    uint8_t severity;
    
    if (error_code == ERROR_OK) {
        return 0;  /* 无错误，不处理 */
    }
    
    /* 获取错误严重程度 */
    severity = error_code & 0xFF;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_error_mutex, UINT32_MAX);
#endif
    
    /* 更新统计信息 */
    update_error_statistics(error_code & 0xFF000000);
    
    /* 根据错误严重程度输出日志 */
    switch (severity) {
        case ERROR_SEVERITY_INFO:
            LOG_INFO("Error: 0x%08X, File: %s, Line: %d", error_code, file, line);
            break;
        case ERROR_SEVERITY_WARNING:
            LOG_WARN("Error: 0x%08X, File: %s, Line: %d", error_code, file, line);
            break;
        case ERROR_SEVERITY_ERROR:
        case ERROR_SEVERITY_CRITICAL:
        case ERROR_SEVERITY_FATAL:
            LOG_ERROR("Error: 0x%08X, File: %s, Line: %d", error_code, file, line);
            break;
        default:
            LOG_ERROR("Unknown Error: 0x%08X, File: %s, Line: %d", error_code, file, line);
            break;
    }
    
    /* 调用注册的回调函数 */
    for (i = 0; i < MAX_ERROR_CALLBACKS; i++) {
        if (g_error_callbacks[i].used && g_error_callbacks[i].callback != NULL) {
            g_error_callbacks[i].callback(error_code, file, line, g_error_callbacks[i].user_data);
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_error_mutex);
#endif
    
    /* 如果是致命错误，停止系统 */
    if (severity == ERROR_SEVERITY_FATAL) {
        LOG_ERROR("Fatal error detected, system halted.");
        while (1);  /* 死循环 */
    }
    
    return 0;
}

/**
 * @brief 获取错误描述
 * 
 * @param error_code 错误码
 * @return const char* 错误描述字符串
 */
const char *error_get_description(uint32_t error_code)
{
    int i;
    
    /* 查找错误描述 */
    for (i = 0; g_error_descriptions[i].description != NULL; i++) {
        if (g_error_descriptions[i].code == error_code) {
            return g_error_descriptions[i].description;
        }
    }
    
    return "Unknown error";
}

/**
 * @brief 获取错误统计信息
 * 
 * @param module 模块标识，0表示所有模块
 * @param count 错误计数
 * @return int 0表示成功，非0表示失败
 */
int error_get_statistics(uint32_t module, uint32_t *count)
{
    int i;
    
    if (count == NULL) {
        return -1;
    }
    
    *count = 0;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_error_mutex, UINT32_MAX);
#endif
    
    if (module == 0) {
        /* 统计所有模块的错误 */
        for (i = 0; i < g_statistics_count; i++) {
            *count += g_error_statistics[i].count;
        }
    } else {
        /* 统计指定模块的错误 */
        for (i = 0; i < g_statistics_count; i++) {
            if (g_error_statistics[i].module == module) {
                *count = g_error_statistics[i].count;
                break;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_error_mutex);
#endif
    
    return 0;
}

/**
 * @brief 清除错误统计信息
 * 
 * @param module 模块标识，0表示所有模块
 * @return int 0表示成功，非0表示失败
 */
int error_clear_statistics(uint32_t module)
{
    int i;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_error_mutex, UINT32_MAX);
#endif
    
    if (module == 0) {
        /* 清除所有模块的统计信息 */
        memset(g_error_statistics, 0, sizeof(g_error_statistics));
        g_statistics_count = 0;
    } else {
        /* 清除指定模块的统计信息 */
        for (i = 0; i < g_statistics_count; i++) {
            if (g_error_statistics[i].module == module) {
                g_error_statistics[i].count = 0;
                break;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_error_mutex);
#endif
    
    return 0;
}
