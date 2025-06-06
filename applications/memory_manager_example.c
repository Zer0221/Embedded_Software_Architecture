/**
 * @file memory_manager_example.c
 * @brief 内存管理器使用示例
 */

#include <stdio.h>
#include "common/memory_manager.h"
#include "common/error_handling.h"

// 示例函数：使用系统堆
void memory_system_heap_example(void) {
    // 分配内存
    void *ptr1 = mem_alloc(1024);
    if (ptr1 == NULL) {
        printf("Failed to allocate memory from system heap\n");
        return;
    }
    printf("Allocated 1024 bytes from system heap at %p\n", ptr1);
    
    // 再分配一块内存
    void *ptr2 = mem_alloc(2048);
    if (ptr2 == NULL) {
        printf("Failed to allocate memory from system heap\n");
        mem_free(ptr1);
        return;
    }
    printf("Allocated 2048 bytes from system heap at %p\n", ptr2);
    
    // 获取内存统计信息
    mem_stats_t stats;
    if (mem_get_stats(NULL, &stats) == 0) {
        printf("System heap statistics:\n");
        printf("  Total size: %u bytes\n", stats.total_size);
        printf("  Used size: %u bytes\n", stats.used_size);
        printf("  Free size: %u bytes\n", stats.free_size);
        printf("  Alloc count: %u\n", stats.alloc_count);
        printf("  Free count: %u\n", stats.free_count);
    }
    
    // 释放内存
    mem_free(ptr1);
    mem_free(ptr2);
    printf("Memory freed\n");
    
    // 再次获取统计信息
    if (mem_get_stats(NULL, &stats) == 0) {
        printf("System heap statistics after free:\n");
        printf("  Used size: %u bytes\n", stats.used_size);
        printf("  Alloc count: %u\n", stats.alloc_count);
        printf("  Free count: %u\n", stats.free_count);
    }
}

// 示例函数：使用自定义内存池
void memory_pool_example(void) {
    mem_pool_handle_t pool;
    
    // 创建内存池
    if (mem_pool_create(4096, &pool) != 0) {
        printf("Failed to create memory pool\n");
        return;
    }
    printf("Memory pool created\n");
    
    // 从内存池分配内存
    void *ptr1 = mem_pool_alloc(pool, 512);
    if (ptr1 == NULL) {
        printf("Failed to allocate memory from pool\n");
        mem_pool_destroy(pool);
        return;
    }
    printf("Allocated 512 bytes from pool at %p\n", ptr1);
    
    // 再分配一块内存
    void *ptr2 = mem_pool_alloc(pool, 1024);
    if (ptr2 == NULL) {
        printf("Failed to allocate memory from pool\n");
        mem_pool_free(pool, ptr1);
        mem_pool_destroy(pool);
        return;
    }
    printf("Allocated 1024 bytes from pool at %p\n", ptr2);
    
    // 获取内存池统计信息
    mem_stats_t stats;
    if (mem_get_stats(pool, &stats) == 0) {
        printf("Memory pool statistics:\n");
        printf("  Total size: %u bytes\n", stats.total_size);
        printf("  Used size: %u bytes\n", stats.used_size);
        printf("  Free size: %u bytes\n", stats.free_size);
        printf("  Alloc count: %u\n", stats.alloc_count);
        printf("  Free count: %u\n", stats.free_count);
    }
    
    // 释放内存
    mem_pool_free(pool, ptr1);
    mem_pool_free(pool, ptr2);
    printf("Pool memory freed\n");
    
    // 检查内存泄漏
    uint32_t leak_count;
    if (mem_check_leaks(pool, &leak_count) == 0) {
        if (leak_count > 0) {
            printf("Memory leak detected: %u blocks\n", leak_count);
        } else {
            printf("No memory leaks detected\n");
        }
    }
    
    // 销毁内存池
    mem_pool_destroy(pool);
    printf("Memory pool destroyed\n");
}

// 测试函数：运行内存管理示例
void run_memory_manager_examples(void) {
    // 初始化内存管理器
    if (mem_init() != 0) {
        printf("Failed to initialize memory manager\n");
        return;
    }
    printf("Memory manager initialized\n");
    
    // 运行系统堆示例
    printf("\n=== System Heap Example ===\n");
    memory_system_heap_example();
    
    // 运行内存池示例
    printf("\n=== Memory Pool Example ===\n");
    memory_pool_example();
    
    // 输出调试信息
    printf("\n=== Memory Debug Info ===\n");
    mem_debug_info(NULL);
}
