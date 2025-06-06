/**
 * @file log_manager.c
 * @brief 日志系统接口实现
 *
 * 该文件实现了日志系统的统一接口，提供了不同级别的日志记录和管理功能
 */

#include "common/log_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/* 日志系统配置 */
static struct {
    log_level_t current_level;
    uint32_t targets;
    char log_file_path[256];
    FILE* log_file;
    char* memory_buffer;
    size_t memory_buffer_size;
    size_t memory_buffer_pos;
} log_config = {
    .current_level = LOG_LEVEL_INFO,
    .targets = LOG_TARGET_CONSOLE,
    .log_file_path = "",
    .log_file = NULL,
    .memory_buffer = NULL,
    .memory_buffer_size = 0,
    .memory_buffer_pos = 0
};

/* 日志级别对应的名称 */
static const char* log_level_names[] = {
    "NONE",
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "VERBOSE",
    "ALL"
};

/**
 * @brief 初始化日志系统
 * 
 * @param level 日志级别
 * @param targets 日志输出目标
 * @return 成功返回0，失败返回负值
 */
int log_init(log_level_t level, uint32_t targets)
{
    log_config.current_level = level;
    log_config.targets = targets;
    
    /* 清理之前的配置 */
    if (log_config.log_file) {
        fclose(log_config.log_file);
        log_config.log_file = NULL;
    }
    
    if (log_config.memory_buffer) {
        free(log_config.memory_buffer);
        log_config.memory_buffer = NULL;
        log_config.memory_buffer_size = 0;
        log_config.memory_buffer_pos = 0;
    }
    
    /* 初始化文件输出 */
    if (targets & LOG_TARGET_FILE) {
        /* 如果没有设置文件路径，使用默认路径 */
        if (log_config.log_file_path[0] == '\0') {
            strcpy(log_config.log_file_path, "application.log");
        }
        
        log_config.log_file = fopen(log_config.log_file_path, "a");
        if (!log_config.log_file) {
            return -1;
        }
    }
    
    /* 初始化内存缓冲区 */
    if (targets & LOG_TARGET_MEMORY) {
        log_config.memory_buffer_size = 4096; /* 默认4KB缓冲区 */
        log_config.memory_buffer = (char*)malloc(log_config.memory_buffer_size);
        if (!log_config.memory_buffer) {
            if (log_config.log_file) {
                fclose(log_config.log_file);
                log_config.log_file = NULL;
            }
            return -1;
        }
        memset(log_config.memory_buffer, 0, log_config.memory_buffer_size);
    }
    
    return 0;
}

/**
 * @brief 设置日志文件路径
 * 
 * @param file_path 日志文件路径
 * @return 成功返回0，失败返回负值
 */
int log_set_file_path(const char* file_path)
{
    if (!file_path) {
        return -1;
    }
    
    /* 关闭旧的日志文件 */
    if (log_config.log_file) {
        fclose(log_config.log_file);
        log_config.log_file = NULL;
    }
    
    strncpy(log_config.log_file_path, file_path, sizeof(log_config.log_file_path) - 1);
    log_config.log_file_path[sizeof(log_config.log_file_path) - 1] = '\0';
    
    /* 只有在目标包含文件输出时才打开文件 */
    if (log_config.targets & LOG_TARGET_FILE) {
        log_config.log_file = fopen(log_config.log_file_path, "a");
        if (!log_config.log_file) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 设置内存缓冲区大小
 * 
 * @param size 缓冲区大小（字节）
 * @return 成功返回0，失败返回负值
 */
int log_set_memory_buffer_size(size_t size)
{
    if (size == 0) {
        return -1;
    }
    
    /* 释放旧的缓冲区 */
    if (log_config.memory_buffer) {
        free(log_config.memory_buffer);
    }
    
    log_config.memory_buffer_size = size;
    log_config.memory_buffer = (char*)malloc(size);
    if (!log_config.memory_buffer) {
        log_config.memory_buffer_size = 0;
        return -1;
    }
    
    memset(log_config.memory_buffer, 0, size);
    log_config.memory_buffer_pos = 0;
    
    return 0;
}

/**
 * @brief 设置日志级别
 * 
 * @param level 日志级别
 */
void log_set_level(log_level_t level)
{
    log_config.current_level = level;
}

/**
 * @brief 获取当前日志级别
 * 
 * @return 当前日志级别
 */
log_level_t log_get_level(void)
{
    return log_config.current_level;
}

/**
 * @brief 获取当前时间字符串
 * 
 * @param buffer 时间字符串缓冲区
 * @param size 缓冲区大小
 */
static void get_time_string(char* buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * @brief 写入日志到目标
 * 
 * @param level 日志级别
 * @param format 格式化字符串
 * @param args 参数列表
 */
static void log_write(log_level_t level, const char* format, va_list args)
{
    /* 检查日志级别 */
    if (level > log_config.current_level || level == LOG_LEVEL_NONE) {
        return;
    }
    
    char time_str[32];
    get_time_string(time_str, sizeof(time_str));
    
    /* 构建日志前缀 */
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "[%s][%s] ", time_str, log_level_names[level]);
    
    /* 格式化日志内容 */
    char log_content[1024];
    vsnprintf(log_content, sizeof(log_content), format, args);
    
    /* 写入控制台 */
    if (log_config.targets & LOG_TARGET_CONSOLE) {
        FILE* output = (level <= LOG_LEVEL_ERROR) ? stderr : stdout;
        fprintf(output, "%s%s\n", prefix, log_content);
        fflush(output);
    }
    
    /* 写入文件 */
    if ((log_config.targets & LOG_TARGET_FILE) && log_config.log_file) {
        fprintf(log_config.log_file, "%s%s\n", prefix, log_content);
        fflush(log_config.log_file);
    }
    
    /* 写入内存 */
    if ((log_config.targets & LOG_TARGET_MEMORY) && log_config.memory_buffer) {
        size_t entry_len = strlen(prefix) + strlen(log_content) + 2; /* +2 for \n and \0 */
        
        /* 检查缓冲区是否有足够空间 */
        if (log_config.memory_buffer_pos + entry_len >= log_config.memory_buffer_size) {
            /* 缓冲区已满，丢弃最早的日志 */
            size_t half_size = log_config.memory_buffer_size / 2;
            memmove(log_config.memory_buffer, 
                    log_config.memory_buffer + half_size, 
                    log_config.memory_buffer_size - half_size);
            log_config.memory_buffer_pos = log_config.memory_buffer_size - half_size;
            memset(log_config.memory_buffer + log_config.memory_buffer_pos, 0, half_size);
        }
        
        /* 写入日志 */
        char* write_pos = log_config.memory_buffer + log_config.memory_buffer_pos;
        int written = snprintf(write_pos, 
                              log_config.memory_buffer_size - log_config.memory_buffer_pos, 
                              "%s%s\n", prefix, log_content);
        
        if (written > 0) {
            log_config.memory_buffer_pos += written;
        }
    }
}

/**
 * @brief 记录致命错误日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_fatal(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_FATAL, format, args);
    va_end(args);
}

/**
 * @brief 记录错误日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

/**
 * @brief 记录警告日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

/**
 * @brief 记录信息日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

/**
 * @brief 记录调试日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

/**
 * @brief 记录详细日志
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_verbose(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_VERBOSE, format, args);
    va_end(args);
}

/**
 * @brief 关闭日志系统
 */
void log_deinit(void)
{
    if (log_config.log_file) {
        fclose(log_config.log_file);
        log_config.log_file = NULL;
    }
    
    if (log_config.memory_buffer) {
        free(log_config.memory_buffer);
        log_config.memory_buffer = NULL;
        log_config.memory_buffer_size = 0;
        log_config.memory_buffer_pos = 0;
    }
}
