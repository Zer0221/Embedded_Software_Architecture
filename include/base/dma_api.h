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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* DMA传输事件类型 */
typedef enum {
    DMA_EVENT_TRANSFER_COMPLETE,  /**< 传输完成事件 */
    DMA_EVENT_TRANSFER_HALF,      /**< 传输一半事件 */
    DMA_EVENT_TRANSFER_ERROR,     /**< 传输错误事件 */
    DMA_EVENT_TRANSFER_ABORT      /**< 传输中止事件 */
} dma_event_t;

/* DMA请求类型 */
typedef enum {
    DMA_REQUEST_UART_TX,     /**< UART发送请求 */
    DMA_REQUEST_UART_RX,     /**< UART接收请求 */
    DMA_REQUEST_SPI_TX,      /**< SPI发送请求 */
    DMA_REQUEST_SPI_RX,      /**< SPI接收请求 */
    DMA_REQUEST_I2C_TX,      /**< I2C发送请求 */
    DMA_REQUEST_I2C_RX,      /**< I2C接收请求 */
    DMA_REQUEST_ADC,         /**< ADC请求 */
    DMA_REQUEST_DAC,         /**< DAC请求 */
    DMA_REQUEST_TIMER,       /**< 定时器请求 */
    DMA_REQUEST_CUSTOM       /**< 自定义请求 */
} dma_request_t;

/* DMA配置 */
typedef struct {
    dma_direction_t direction;   /**< 传输方向 */
    dma_mode_t mode;             /**< 传输模式 */
    dma_priority_t priority;     /**< 传输优先级 */
    dma_data_width_t src_width;  /**< 源数据宽度 */
    dma_data_width_t dst_width;  /**< 目标数据宽度 */
    bool src_increment;          /**< 源地址增量使能 */
    bool dst_increment;          /**< 目标地址增量使能 */
    dma_request_t request;       /**< 请求类型 */
    uint8_t peripheral_id;       /**< 外设ID */
    void *user_data;             /**< 用户数据 */
} dma_config_t;

/* DMA传输回调函数类型 */
typedef void (*dma_callback_t)(uint8_t channel, dma_event_t event, void *user_data);

/**
 * @brief 初始化DMA通道
 *
 * @param channel DMA通道号
 * @param config DMA配置参数
 * @return api_status_t 操作状态
 */
api_status_t dma_init(uint8_t channel, const dma_config_t *config);

/**
 * @brief 反初始化DMA通道
 *
 * @param channel DMA通道号
 * @return api_status_t 操作状态
 */
api_status_t dma_deinit(uint8_t channel);

/**
 * @brief 启动DMA传输
 *
 * @param channel DMA通道号
 * @param src_addr 源地址
 * @param dst_addr 目标地址
 * @param length 传输长度
 * @return api_status_t 操作状态
 */
api_status_t dma_start_transfer(uint8_t channel, const void *src_addr, void *dst_addr, uint32_t length);

/**
 * @brief 停止DMA传输
 *
 * @param channel DMA通道号
 * @return api_status_t 操作状态
 */
api_status_t dma_stop_transfer(uint8_t channel);

/**
 * @brief 注册DMA传输回调函数
 *
 * @param channel DMA通道号
 * @param callback 回调函数
 * @param events 感兴趣的事件
 * @return api_status_t 操作状态
 */
api_status_t dma_register_callback(uint8_t channel, dma_callback_t callback, uint32_t events);

/**
 * @brief 获取DMA传输状态
 *
 * @param channel DMA通道号
 * @param status 传输状态
 * @return api_status_t 操作状态
 */
api_status_t dma_get_status(uint8_t channel, dma_status_t *status);

/**
 * @brief 获取DMA传输剩余数量
 *
 * @param channel DMA通道号
 * @param remaining 剩余数量
 * @return api_status_t 操作状态
 */
api_status_t dma_get_transfer_count(uint8_t channel, uint32_t *remaining);

/**
 * @brief 更新DMA内存地址
 *
 * @param channel DMA通道号
 * @param address 新地址
 * @param is_source 是否为源地址
 * @return api_status_t 操作状态
 */
api_status_t dma_update_memory_address(uint8_t channel, void *address, bool is_source);

/**
 * @brief 获取可用的DMA通道
 *
 * @param channel 获取到的通道号
 * @return api_status_t 操作状态
 */
api_status_t dma_get_available_channel(uint8_t *channel);

/**
 * @brief 执行DMA内存拷贝(阻塞)
 *
 * @param dst_addr 目标地址
 * @param src_addr 源地址
 * @param length 长度
 * @return api_status_t 操作状态
 */
api_status_t dma_memcpy(void *dst_addr, const void *src_addr, uint32_t length);

/**
 * @brief 执行DMA内存拷贝(非阻塞)
 *
 * @param dst_addr 目标地址
 * @param src_addr 源地址
 * @param length 长度
 * @param callback 完成回调
 * @param user_data 用户数据
 * @return api_status_t 操作状态
 */
api_status_t dma_memcpy_async(void *dst_addr, const void *src_addr, uint32_t length, dma_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* DMA_API_H */