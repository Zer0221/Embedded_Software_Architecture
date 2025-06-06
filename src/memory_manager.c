/**
 * @file memory_manager.c
 * @brief 内存管理器实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/memory_manager.h"
#include "common/error_handling.h"

#ifdef CONFIG_USE_RTOS
#include "common/rtos_api.h"
#endif

/* 内存块结构 */
typedef struct memory_block {
    uint32_t magic;               /**< 魔数，用于验证块的有效性 */
    uint32_t size;                /**< 块大小 */
    bool used;                    /**< 是否使用中 */
    const char *alloc_file;       /**< 分配内存的文件 */
    int alloc_line;               /**< 分配内存的行号 */
    struct memory_block *next;    /**< 下一个块 */
    struct memory_block *prev;    /**< 上一个块 */
} memory_block_t;

/* 内存池结构 */
typedef struct {
    void *memory;                 /**< 内存池地址 */
    uint32_t size;                /**< 内存池大小 */
    memory_block_t *first_block;  /**< 第一个内存块 */
    mem_stats_t stats;            /**< 内存池统计信息 */
#ifdef CONFIG_USE_RTOS
    mutex_handle_t mutex;         /**< 互斥锁 */
#endif
} memory_pool_t;

/* 全局内存池 */
static memory_pool_t g_system_pool;

/* 魔数定义 */
#define MEMORY_BLOCK_MAGIC        0xDEADBEEF
#define MEMORY_POOL_HEADER_SIZE   sizeof(memory_pool_t)
#define MEMORY_BLOCK_HEADER_SIZE  sizeof(memory_block_t)

/* 内部函数声明 */
static void *mem_alloc_internal(memory_pool_t *pool, uint32_t size, const char *file, int line);
static int mem_free_internal(memory_pool_t *pool, void *ptr);
static void mem_update_stats(memory_pool_t *pool);
static void mem_dump_block_info(memory_block_t *block);

/**
 * @brief 初始化内存管理器
 */
int mem_init(void) {
    // 初始化系统内存池统计信息
    memset(&g_system_pool.stats, 0, sizeof(mem_stats_t));
    g_system_pool.first_block = NULL;
    
#ifdef CONFIG_USE_RTOS
    // 创建互斥锁
    if (mutex_create(&g_system_pool.mutex) != 0) {
        return -1;
    }
#endif
    
    return 0;
}

/**
 * @brief 创建内存池
 */
int mem_pool_create(uint32_t size, mem_pool_handle_t *handle) {
    if (size == 0 || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 分配内存池结构体
    memory_pool_t *pool = (memory_pool_t *)malloc(sizeof(memory_pool_t));
    if (pool == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    // 分配内存池空间
    pool->memory = malloc(size);
    if (pool->memory == NULL) {
        free(pool);
        return ERROR_NO_MEMORY;
    }
    
    // 初始化内存池
    pool->size = size;
    pool->first_block = NULL;
    memset(&pool->stats, 0, sizeof(mem_stats_t));
    pool->stats.total_size = size;
    pool->stats.free_size = size;
    
#ifdef CONFIG_USE_RTOS
    // 创建互斥锁
    if (mutex_create(&pool->mutex) != 0) {
        free(pool->memory);
        free(pool);
        return ERROR_MUTEX_CREATE_FAILED;
    }
#endif
    
    *handle = (mem_pool_handle_t)pool;
    return 0;
}

/**
 * @brief 销毁内存池
 */
int mem_pool_destroy(mem_pool_handle_t handle) {
    memory_pool_t *pool = (memory_pool_t *)handle;
    
    if (pool == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 检查是否还有分配的内存未释放
    if (pool->stats.used_size > 0) {
        // 存在内存泄漏，打印警告并释放所有内存块
        printf("Warning: Memory leak detected in pool %p, %u bytes not freed\n", 
               pool, pool->stats.used_size);
        
        // 可以遍历链表释放所有分配的内存块，这里简化处理
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁并销毁互斥锁
    mutex_unlock(pool->mutex);
    mutex_destroy(pool->mutex);
#endif
    
    // 释放内存池
    free(pool->memory);
    free(pool);
    
    return 0;
}

/**
 * @brief 从指定内存池分配内存
 */
void *mem_pool_alloc(mem_pool_handle_t handle, uint32_t size) {
    return mem_alloc_internal((memory_pool_t *)handle, size, __FILE__, __LINE__);
}

/**
 * @brief 从指定内存池释放内存
 */
int mem_pool_free(mem_pool_handle_t handle, void *ptr) {
    return mem_free_internal((memory_pool_t *)handle, ptr);
}

/**
 * @brief 从系统堆分配内存
 */
void *mem_alloc(uint32_t size) {
    return mem_alloc_internal(&g_system_pool, size, __FILE__, __LINE__);
}

/**
 * @brief 释放通过mem_alloc分配的内存
 */
int mem_free(void *ptr) {
    return mem_free_internal(&g_system_pool, ptr);
}

/**
 * @brief 获取内存统计信息
 */
int mem_get_stats(mem_pool_handle_t handle, mem_stats_t *stats) {
    memory_pool_t *pool;
    
    if (stats == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 如果handle为NULL，使用系统池
    if (handle == NULL) {
        pool = &g_system_pool;
    } else {
        pool = (memory_pool_t *)handle;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 更新统计信息
    mem_update_stats(pool);
    
    // 复制统计信息
    memcpy(stats, &pool->stats, sizeof(mem_stats_t));
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(pool->mutex);
#endif
    
    return 0;
}

/**
 * @brief 检查内存泄漏
 */
int mem_check_leaks(mem_pool_handle_t handle, uint32_t *leak_count) {
    memory_pool_t *pool;
    memory_block_t *block;
    uint32_t count = 0;
    
    if (leak_count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // 如果handle为NULL，使用系统池
    if (handle == NULL) {
        pool = &g_system_pool;
    } else {
        pool = (memory_pool_t *)handle;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 遍历所有内存块，检查是否有未释放的
    block = pool->first_block;
    while (block != NULL) {
        if (block->used) {
            count++;
            printf("Memory leak detected: %u bytes at %p, allocated in %s:%d\n",
                   block->size, (char*)block + MEMORY_BLOCK_HEADER_SIZE,
                   block->alloc_file, block->alloc_line);
        }
        block = block->next;
    }
    
    *leak_count = count;
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(pool->mutex);
#endif
    
    return 0;
}

/**
 * @brief 内存调试函数，输出内存使用情况
 */
int mem_debug_info(mem_pool_handle_t handle) {
    memory_pool_t *pool;
    memory_block_t *block;
    
    // 如果handle为NULL，使用系统池
    if (handle == NULL) {
        pool = &g_system_pool;
        printf("System Memory Pool Debug Info:\n");
    } else {
        pool = (memory_pool_t *)handle;
        printf("Custom Memory Pool Debug Info (%p):\n", pool);
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 更新并打印统计信息
    mem_update_stats(pool);
    printf("Total: %u bytes, Used: %u bytes, Free: %u bytes\n",
           pool->stats.total_size, pool->stats.used_size, pool->stats.free_size);
    printf("Alloc count: %u, Free count: %u\n",
           pool->stats.alloc_count, pool->stats.free_count);
    printf("Max block size: %u, Min block size: %u\n",
           pool->stats.max_block_size, pool->stats.min_block_size);
    printf("Fragmentation: %u%%\n", pool->stats.fragmentation);
    
    // 打印所有内存块信息
    printf("\nMemory Blocks:\n");
    block = pool->first_block;
    while (block != NULL) {
        mem_dump_block_info(block);
        block = block->next;
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(pool->mutex);
#endif
    
    return 0;
}

/**
 * @brief 内部内存分配函数
 */
static void *mem_alloc_internal(memory_pool_t *pool, uint32_t size, const char *file, int line) {
    memory_block_t *block;
    memory_block_t *new_block;
    void *ptr = NULL;
    
    if (pool == NULL || size == 0) {
        return NULL;
    }
    
    // 对齐大小
    size = (size + 3) & ~3;  // 4字节对齐
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 系统堆分配
    if (pool == &g_system_pool) {
        // 分配内存块头部和用户数据
        new_block = (memory_block_t *)malloc(MEMORY_BLOCK_HEADER_SIZE + size);
        if (new_block == NULL) {
#ifdef CONFIG_USE_RTOS
            mutex_unlock(pool->mutex);
#endif
            return NULL;
        }
        
        // 初始化内存块
        new_block->magic = MEMORY_BLOCK_MAGIC;
        new_block->size = size;
        new_block->used = true;
        new_block->alloc_file = file;
        new_block->alloc_line = line;
        
        // 将新块添加到链表
        if (pool->first_block == NULL) {
            new_block->prev = NULL;
            new_block->next = NULL;
            pool->first_block = new_block;
        } else {
            new_block->prev = NULL;
            new_block->next = pool->first_block;
            pool->first_block->prev = new_block;
            pool->first_block = new_block;
        }
        
        // 更新统计信息
        pool->stats.alloc_count++;
        pool->stats.used_size += size;
        if (size > pool->stats.max_block_size) {
            pool->stats.max_block_size = size;
        }
        if (pool->stats.min_block_size == 0 || size < pool->stats.min_block_size) {
            pool->stats.min_block_size = size;
        }
        
        ptr = (void *)((char *)new_block + MEMORY_BLOCK_HEADER_SIZE);
    }
    // 自定义内存池分配
    else {
        // 这里应该实现自定义内存池的分配算法
        // 为简化示例，暂不实现
        // 实际上应该实现一个适合嵌入式设备的内存分配算法，如first-fit, best-fit等
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(pool->mutex);
#endif
    
    return ptr;
}

/**
 * @brief 内部内存释放函数
 */
static int mem_free_internal(memory_pool_t *pool, void *ptr) {
    memory_block_t *block;
    
    if (pool == NULL || ptr == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
#ifdef CONFIG_USE_RTOS
    // 锁定互斥锁
    mutex_lock(pool->mutex);
#endif
    
    // 获取内存块头部
    block = (memory_block_t *)((char *)ptr - MEMORY_BLOCK_HEADER_SIZE);
    
    // 验证魔数
    if (block->magic != MEMORY_BLOCK_MAGIC) {
#ifdef CONFIG_USE_RTOS
        mutex_unlock(pool->mutex);
#endif
        return ERROR_INVALID_MEMORY;
    }
    
    // 系统堆释放
    if (pool == &g_system_pool) {
        // 更新统计信息
        pool->stats.free_count++;
        pool->stats.used_size -= block->size;
        
        // 从链表中移除
        if (block->prev != NULL) {
            block->prev->next = block->next;
        } else {
            pool->first_block = block->next;
        }
        
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
        
        // 释放内存
        free(block);
    }
    // 自定义内存池释放
    else {
        // 这里应该实现自定义内存池的释放算法
        // 为简化示例，暂不实现
    }
    
#ifdef CONFIG_USE_RTOS
    // 解锁互斥锁
    mutex_unlock(pool->mutex);
#endif
    
    return 0;
}

/**
 * @brief 更新内存池统计信息
 */
static void mem_update_stats(memory_pool_t *pool) {
    // 对于系统堆，我们只能估计碎片化程度
    if (pool == &g_system_pool) {
        // 简单估计：碎片化程度 = (1 - 最大连续空闲块大小/总空闲大小) * 100
        // 由于难以获取系统堆的最大连续空闲块大小，这里使用一个简化的计算
        pool->stats.fragmentation = (pool->stats.alloc_count > pool->stats.free_count) ? 
            ((pool->stats.alloc_count - pool->stats.free_count) * 10) : 0;
        
        // 确保碎片化程度不超过100
        if (pool->stats.fragmentation > 100) {
            pool->stats.fragmentation = 100;
        }
    }
    // 自定义内存池的统计更新
    else {
        // 遍历内存池中的所有块计算统计信息
        // 为简化示例，暂不实现
    }
}

/**
 * @brief 打印内存块信息
 */
static void mem_dump_block_info(memory_block_t *block) {
    if (block == NULL) {
        return;
    }
    
    printf("Block %p: Size=%u, %s, Allocated at %s:%d\n",
           (char*)block + MEMORY_BLOCK_HEADER_SIZE,
           block->size,
           block->used ? "Used" : "Free",
           block->alloc_file, block->alloc_line);
}
