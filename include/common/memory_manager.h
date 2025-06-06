/**
 * @file memory_manager.h
 * @brief 内存管理接口定义
 *
 * 该头文件定义了内存管理相关的接口，提供统一的内存分配和释放功能
 */

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 内存池句柄 */
typedef void* mem_pool_handle_t;

/* 内存统计信息 */
typedef struct {
    uint32_t total_size;       /**< 总内存大小 */
    uint32_t used_size;        /**< 已使用内存大小 */
    uint32_t free_size;        /**< 可用内存大小 */
    uint32_t alloc_count;      /**< 分配次数 */
    uint32_t free_count;       /**< 释放次数 */
    uint32_t max_block_size;   /**< 最大可分配块大小 */
    uint32_t min_block_size;   /**< 最小可分配块大小 */
    uint32_t fragmentation;    /**< 内存碎片化程度 (0-100) */
} mem_stats_t;

/**
 * @brief 初始化内存管理器
 * 
 * @return int 0表示成功，非0表示失败
 */
int mem_init(void);

/**
 * @brief 创建内存池
 * 
 * @param size 内存池大小
 * @param handle 返回的内存池句柄
 * @return int 0表示成功，非0表示失败
 */
int mem_pool_create(uint32_t size, mem_pool_handle_t *handle);

/**
 * @brief 销毁内存池
 * 
 * @param handle 内存池句柄
 * @return int 0表示成功，非0表示失败
 */
int mem_pool_destroy(mem_pool_handle_t handle);

/**
 * @brief 从指定内存池分配内存
 * 
 * @param handle 内存池句柄
 * @param size 要分配的大小
 * @return void* 分配的内存指针，分配失败返回NULL
 */
void *mem_pool_alloc(mem_pool_handle_t handle, uint32_t size);

/**
 * @brief 从指定内存池释放内存
 * 
 * @param handle 内存池句柄
 * @param ptr 要释放的内存指针
 * @return int 0表示成功，非0表示失败
 */
int mem_pool_free(mem_pool_handle_t handle, void *ptr);

/**
 * @brief 从系统堆分配内存
 * 
 * @param size 要分配的大小
 * @return void* 分配的内存指针，分配失败返回NULL
 */
void *mem_alloc(uint32_t size);

/**
 * @brief 释放通过mem_alloc分配的内存
 * 
 * @param ptr 要释放的内存指针
 * @return int 0表示成功，非0表示失败
 */
int mem_free(void *ptr);

/**
 * @brief 获取内存统计信息
 * 
 * @param handle 内存池句柄，为NULL时获取系统堆的统计信息
 * @param stats 统计信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int mem_get_stats(mem_pool_handle_t handle, mem_stats_t *stats);

/**
 * @brief 检查内存泄漏
 * 
 * @param handle 内存池句柄，为NULL时检查系统堆
 * @param leak_count 返回检测到的泄漏数量
 * @return int 0表示成功，非0表示失败
 */
int mem_check_leaks(mem_pool_handle_t handle, uint32_t *leak_count);

/**
 * @brief 内存调试函数，输出内存使用情况
 * 
 * @param handle 内存池句柄，为NULL时输出系统堆
 * @return int 0表示成功，非0表示失败
 */
int mem_debug_info(mem_pool_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_H */
