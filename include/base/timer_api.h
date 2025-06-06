/**
 * @file timer_api.h
 * @brief 定时器接口抽象层定义
 *
 * 该头文件定义了定时器的统一抽象接口，使上层应用能够与底层定时器硬件实现解耦
 */

#ifndef TIMER_API_H
#define TIMER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 定时器模式 */
typedef enum {
    TIMER_MODE_ONE_SHOT,    /**< 单次触发模式 */
    TIMER_MODE_PERIODIC     /**< 周期触发模式 */
} timer_mode_t;

/* 定时器触发类型 */
typedef enum {
    TIMER_TRIGGER_OVERFLOW,        /**< 溢出触发 */
    TIMER_TRIGGER_COMPARE_MATCH,   /**< 比较匹配触发 */
    TIMER_TRIGGER_INPUT_CAPTURE    /**< 输入捕获触发 */
} timer_trigger_t;

/* 定时器时钟源 */
typedef enum {
    TIMER_CLOCK_INTERNAL,      /**< 内部时钟源 */
    TIMER_CLOCK_EXTERNAL,      /**< 外部时钟源 */
    TIMER_CLOCK_PCLK,          /**< PCLK时钟源 */
    TIMER_CLOCK_HCLK           /**< HCLK时钟源 */
} timer_clock_source_t;

/* 定时器配置参数 */
typedef struct {
    timer_mode_t mode;               /**< 定时器模式 */
    timer_trigger_t trigger;         /**< 触发类型 */
    timer_clock_source_t clock_src;  /**< 时钟源 */
    uint32_t prescaler;              /**< 预分频值 */
    uint32_t period_us;              /**< 定时周期(微秒) */
    bool auto_reload;                /**< 是否自动重载 */
} timer_config_t;

/* 定时器回调函数类型 */
typedef void (*timer_callback_t)(void *arg);

/* 定时器设备句柄 */
typedef driver_handle_t timer_handle_t;

/**
 * @brief 初始化定时器
 * 
 * @param timer_id 定时器ID
 * @param config 定时器配置参数
 * @param callback 定时器回调函数
 * @param arg 回调函数参数
 * @param handle 定时器设备句柄指针
 * @return api_status_t 操作状态
 */
api_status_t timer_init(uint8_t timer_id, const timer_config_t *config, timer_callback_t callback, void *arg, timer_handle_t *handle);

/**
 * @brief 反初始化定时器
 * 
 * @param handle 定时器设备句柄
 * @return api_status_t 操作状态
 */
api_status_t timer_deinit(timer_handle_t handle);

/**
 * @brief 启动定时器
 * 
 * @param handle 定时器设备句柄
 * @return api_status_t 操作状态
 */
api_status_t timer_start(timer_handle_t handle);

/**
 * @brief 停止定时器
 * 
 * @param handle 定时器设备句柄
 * @return api_status_t 操作状态
 */
api_status_t timer_stop(timer_handle_t handle);

/**
 * @brief 设置定时器周期
 * 
 * @param handle 定时器设备句柄
 * @param period_us 定时周期(微秒)
 * @return api_status_t 操作状态
 */
api_status_t timer_set_period(timer_handle_t handle, uint32_t period_us);

/**
 * @brief 获取定时器周期
 * 
 * @param handle 定时器设备句柄
 * @param period_us 定时周期指针(微秒)
 * @return api_status_t 操作状态
 */
api_status_t timer_get_period(timer_handle_t handle, uint32_t *period_us);

/**
 * @brief 设置定时器预分频值
 * 
 * @param handle 定时器设备句柄
 * @param prescaler 预分频值
 * @return api_status_t 操作状态
 */
api_status_t timer_set_prescaler(timer_handle_t handle, uint32_t prescaler);

/**
 * @brief 获取定时器预分频值
 * 
 * @param handle 定时器设备句柄
 * @param prescaler 预分频值指针
 * @return api_status_t 操作状态
 */
api_status_t timer_get_prescaler(timer_handle_t handle, uint32_t *prescaler);

/**
 * @brief 设置定时器模式
 * 
 * @param handle 定时器设备句柄
 * @param mode 定时器模式
 * @return api_status_t 操作状态
 */
api_status_t timer_set_mode(timer_handle_t handle, timer_mode_t mode);

/**
 * @brief 获取定时器计数值
 * 
 * @param handle 定时器设备句柄
 * @param count 计数值指针
 * @return api_status_t 操作状态
 */
api_status_t timer_get_count(timer_handle_t handle, uint32_t *count);

/**
 * @brief 设置定时器计数值
 * 
 * @param handle 定时器设备句柄
 * @param count 计数值
 * @return api_status_t 操作状态
 */
api_status_t timer_set_count(timer_handle_t handle, uint32_t count);

/**
 * @brief 设置定时器比较值
 * 
 * @param handle 定时器设备句柄
 * @param channel 通道
 * @param compare_value 比较值
 * @return api_status_t 操作状态
 */
api_status_t timer_set_compare(timer_handle_t handle, uint8_t channel, uint32_t compare_value);

/**
 * @brief 设置定时器回调函数
 * 
 * @param handle 定时器设备句柄
 * @param callback 回调函数
 * @param arg 回调函数参数
 * @return api_status_t 操作状态
 */
api_status_t timer_set_callback(timer_handle_t handle, timer_callback_t callback, void *arg);

/**
 * @brief 等待定时器触发
 * 
 * @param handle 定时器设备句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t timer_wait(timer_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 获取定时器状态
 * 
 * @param handle 定时器设备句柄
 * @param is_running 运行状态指针
 * @return api_status_t 操作状态
 */
api_status_t timer_is_running(timer_handle_t handle, bool *is_running);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_API_H */