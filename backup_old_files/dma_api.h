/**
 * @file dma_api.h
 * @brief DMA接口抽象层定义
 *
 * 该头文件定义了DMA的统一抽象接口，使上层应用能够与底层DMA硬件实现解耦
 */

#ifndef DMA_API_H
#define DMA_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* DMA传输方向 */
typedef enum {
    DMA_DIR_MEM_TO_MEM,      /**< 内存到内存 */
    DMA_DIR_MEM_TO_PERIPH,   /**< 内存到外设 */
    DMA_DIR_PERIPH_TO_MEM,   /**< 外设到内存 */
    DMA_DIR_PERIPH_TO_PERIPH /**< 外设到外设 */
} dma_direction_t;

/* DMA传输模式 */
typedef enum {
    DMA_MODE_NORMAL,      /**< 普通模式 */
    DMA_MODE_CIRCULAR     /**< 循环模式 */
} dma_mode_t;

/* DMA传输优先级 */
typedef enum {
    DMA_PRIORITY_LOW,       /**< 低优先级 */
    DMA_PRIORITY_MEDIUM,    /**< 中优先级 */
    DMA_PRIORITY_HIGH,      /**< 高优先级 */
    DMA_PRIORITY_VERY_HIGH  /**< 超高优先级 */
} dma_priority_t;

/* DMA数据宽度 */
typedef enum {
    DMA_DATA_WIDTH_8BIT,    /**< 8位数据宽度 */
    DMA_DATA_WIDTH_16BIT,   /**< 16位数据宽度 */
    DMA_DATA_WIDTH_32BIT    /**< 32位数据宽度 */
} dma_data_width_t;

/* DMA传输状态 */
typedef enum {
    DMA_STATUS_IDLE,        /**< 空闲状态 */
    DMA_STATUS_BUSY,        /**< 忙状态 */
    DMA_STATUS_COMPLETE,    /**< 传输完成 */
    DMA_STATUS_ERROR,       /**< 传输错误 */
    DMA_STATUS_ABORT        /**< 传输中止 */
} dma_status_t;

/* DMA配置参数 */
typedef struct {
    dma_direction_t direction;   /**< 传输方向 */
    dma_mode_t mode;             /**< 传输模式 */
    dma_priority_t priority;     /**< 传输优先级 */
    dma_data_width_t src_width;  /**< 源数据宽度 */
    dma_data_width_t dst_width;  /**< 目标数据宽度 */
    bool src_inc;                /**< 源地址是否自增 */
    bool dst_inc;                /**< 目标地址是否自增 */
    uint32_t src_addr;           /**< 源地址 */
    uint32_t dst_addr;           /**< 目标地址 */
    uint32_t data_size;          /**< 数据大小(字节) */
    uint32_t burst_size;         /**< 突发大小 */
} dma_config_t;

/* DMA回调函数类型 */
typedef void (*dma_callback_t)(void *arg, dma_status_t status);

/* DMA设备句柄 */
typedef driver_handle_t dma_handle_t;

/**
 * @brief 初始化DMA
 * 
 * @param dma_channel DMA通道
 * @param config DMA配置参数
 * @param callback DMA传输完成回调函数
 * @param arg 传递给回调函数的参数
 * @param handle DMA句柄指针
 * @return int 0表示成功，非0表示失败
 */
int dma_init(uint32_t dma_channel, const dma_config_t *config, 
             dma_callback_t callback, void *arg, dma_handle_t *handle);

/**
 * @brief 去初始化DMA
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_deinit(dma_handle_t handle);

/**
 * @brief 开始DMA传输
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_start(dma_handle_t handle);

/**
 * @brief 停止DMA传输
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_stop(dma_handle_t handle);

/**
 * @brief 获取DMA传输状态
 * 
 * @param handle DMA句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int dma_get_status(dma_handle_t handle, dma_status_t *status);

/**
 * @brief 获取DMA剩余传输数据大小
 * 
 * @param handle DMA句柄
 * @param remaining_size 剩余数据大小指针
 * @return int 0表示成功，非0表示失败
 */
int dma_get_remaining(dma_handle_t handle, uint32_t *remaining_size);

/**
 * @brief 设置DMA传输源地址
 * 
 * @param handle DMA句柄
 * @param src_addr 源地址
 * @return int 0表示成功，非0表示失败
 */
int dma_set_src_address(dma_handle_t handle, uint32_t src_addr);

/**
 * @brief 设置DMA传输目标地址
 * 
 * @param handle DMA句柄
 * @param dst_addr 目标地址
 * @return int 0表示成功，非0表示失败
 */
int dma_set_dst_address(dma_handle_t handle, uint32_t dst_addr);

/**
 * @brief 设置DMA传输数据大小
 * 
 * @param handle DMA句柄
 * @param data_size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int dma_set_data_size(dma_handle_t handle, uint32_t data_size);

/**
 * @brief 使能DMA中断
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_enable_interrupt(dma_handle_t handle);

/**
 * @brief 禁用DMA中断
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_disable_interrupt(dma_handle_t handle);

#endif /* DMA_API_H */
