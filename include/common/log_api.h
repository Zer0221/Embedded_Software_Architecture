/**
 * @file log_api.h
 * @brief 日志系统接口定义
 *
 * 该头文件定义了日志系统的统一接口，提供了不同级别的日志记录和管理功能
 */

#ifndef LOG_API_H
#define LOG_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/* 日志级别 */
typedef enum {
    LOG_LEVEL_NONE = 0,      /**< 不记录日志 */
    LOG_LEVEL_FATAL,         /**< 致命错误 */
    LOG_LEVEL_ERROR,         /**< 一般错误 */
    LOG_LEVEL_WARN,          /**< 警告 */
    LOG_LEVEL_INFO,          /**< 信息 */
    LOG_LEVEL_DEBUG,         /**< 调试 */
    LOG_LEVEL_VERBOSE,       /**< 详细 */
    LOG_LEVEL_ALL            /**< 所有级别 */
} log_level_t;

/* 日志输出目标 */
typedef enum {
    LOG_TARGET_CONSOLE = 0x01,   /**< 控制台 */
    LOG_TARGET_FILE = 0x02,      /**< 文件 */
    LOG_TARGET_MEMORY = 0x04,    /**< 内存 */
    LOG_TARGET_REMOTE = 0x08,    /**< 远程服务器 */
    LOG_TARGET_UART = 0x10,      /**< UART */
    LOG_TARGET_CUSTOM = 0x20     /**< 自定义目标 */
} log_target_t;

/* 日志格式选项 */
typedef enum {
    LOG_FORMAT_LEVEL = 0x01,     /**< 包含日志级别 */
    LOG_FORMAT_TIME = 0x02,      /**< 包含时间戳 */
    LOG_FORMAT_MODULE = 0x04,    /**< 包含模块名 */
    LOG_FORMAT_FILE = 0x08,      /**< 包含文件名 */
    LOG_FORMAT_LINE = 0x10,      /**< 包含行号 */
    LOG_FORMAT_FUNC = 0x20,      /**< 包含函数名 */
    LOG_FORMAT_COLOR = 0x40,     /**< 使用颜色输出 */
    LOG_FORMAT_DEFAULT = 0x7F    /**< 默认格式（包含所有） */
} log_format_t;

/* 日志配置 */
typedef struct {
    log_level_t global_level;    /**< 全局日志级别 */
    uint32_t target_mask;        /**< 输出目标掩码 */
    uint32_t format_mask;        /**< 格式掩码 */
    char* log_file_path;         /**< 日志文件路径 */
    uint32_t max_file_size;      /**< 最大文件大小（字节），0表示无限制 */
    uint8_t max_backup_files;    /**< 最大备份文件数 */
    uint32_t memory_buffer_size; /**< 内存缓冲区大小（字节） */
    char* remote_host;           /**< 远程服务器地址 */
    uint16_t remote_port;        /**< 远程服务器端口 */
    uint8_t uart_instance;       /**< UART实例 */
    bool async_mode;             /**< 是否使用异步模式 */
} log_config_t;

/* 日志模块配置 */
typedef struct {
    const char* module_name;     /**< 模块名称 */
    log_level_t level;           /**< 模块日志级别 */
} log_module_config_t;

/* 日志自定义输出回调函数类型 */
typedef void (*log_output_cb_t)(log_level_t level, const char* module, const char* msg, void* user_data);

/**
 * @brief 初始化日志系统
 * 
 * @param config 日志配置，NULL表示使用默认配置
 * @return int 0表示成功，负值表示失败
 */
int log_init(log_config_t* config);

/**
 * @brief 释放日志系统
 * 
 * @return int 0表示成功，负值表示失败
 */
int log_deinit(void);

/**
 * @brief 设置全局日志级别
 * 
 * @param level 日志级别
 * @return int 0表示成功，负值表示失败
 */
int log_set_level(log_level_t level);

/**
 * @brief 获取全局日志级别
 * 
 * @return log_level_t 日志级别
 */
log_level_t log_get_level(void);

/**
 * @brief 设置模块日志级别
 * 
 * @param module 模块名称
 * @param level 日志级别
 * @return int 0表示成功，负值表示失败
 */
int log_set_module_level(const char* module, log_level_t level);

/**
 * @brief 获取模块日志级别
 * 
 * @param module 模块名称
 * @return log_level_t 日志级别
 */
log_level_t log_get_module_level(const char* module);

/**
 * @brief 设置日志输出目标
 * 
 * @param target_mask 输出目标掩码
 * @return int 0表示成功，负值表示失败
 */
int log_set_target(uint32_t target_mask);

/**
 * @brief 设置日志格式
 * 
 * @param format_mask 格式掩码
 * @return int 0表示成功，负值表示失败
 */
int log_set_format(uint32_t format_mask);

/**
 * @brief 设置日志文件
 * 
 * @param file_path 文件路径
 * @param max_size 最大文件大小（字节），0表示无限制
 * @param max_backup 最大备份文件数
 * @return int 0表示成功，负值表示失败
 */
int log_set_file(const char* file_path, uint32_t max_size, uint8_t max_backup);

/**
 * @brief 设置自定义输出回调
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，负值表示失败
 */
int log_set_custom_output(log_output_cb_t callback, void* user_data);

/**
 * @brief 记录日志（变参数版本）
 * 
 * @param level 日志级别
 * @param module 模块名称
 * @param file 文件名
 * @param line 行号
 * @param func 函数名
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return int 0表示成功，负值表示失败
 */
int log_write(log_level_t level, const char* module, const char* file, int line, const char* func, const char* fmt, ...);

/**
 * @brief 记录日志（va_list版本）
 * 
 * @param level 日志级别
 * @param module 模块名称
 * @param file 文件名
 * @param line 行号
 * @param func 函数名
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return int 0表示成功，负值表示失败
 */
int log_write_v(log_level_t level, const char* module, const char* file, int line, const char* func, const char* fmt, va_list args);

/**
 * @brief 记录十六进制数据
 * 
 * @param level 日志级别
 * @param module 模块名称
 * @param file 文件名
 * @param line 行号
 * @param func 函数名
 * @param prefix 前缀字符串
 * @param data 数据
 * @param len 数据长度
 * @return int 0表示成功，负值表示失败
 */
int log_hex_dump(log_level_t level, const char* module, const char* file, int line, const char* func, const char* prefix, const void* data, uint32_t len);

/**
 * @brief 清空日志缓冲区
 * 
 * @return int 0表示成功，负值表示失败
 */
int log_flush(void);

/**
 * @brief 获取日志统计信息
 * 
 * @param msg_count 已记录的消息数量
 * @param dropped_count 丢弃的消息数量
 * @return int 0表示成功，负值表示失败
 */
int log_get_stats(uint32_t* msg_count, uint32_t* dropped_count);

/* 便捷宏定义 */
#define LOG_FATAL(module, fmt, ...) log_write(LOG_LEVEL_FATAL, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) log_write(LOG_LEVEL_ERROR, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_WARN(module, fmt, ...)  log_write(LOG_LEVEL_WARN, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_INFO(module, fmt, ...)  log_write(LOG_LEVEL_INFO, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(module, fmt, ...) log_write(LOG_LEVEL_DEBUG, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_VERBOSE(module, fmt, ...) log_write(LOG_LEVEL_VERBOSE, module, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_HEX_DUMP(level, module, prefix, data, len) log_hex_dump(level, module, __FILE__, __LINE__, __FUNCTION__, prefix, data, len)

#endif /* LOG_API_H */
