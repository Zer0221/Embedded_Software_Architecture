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
#include "driver_api.h"

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
 * @param arg 传递给回调函数的参数
 * @param handle 定时器句柄指针
 * @return int 0表示成功，非0表示失败
 */
int timer_init(uint32_t timer_id, const timer_config_t *config, 
               timer_callback_t callback, void *arg, timer_handle_t *handle);

/**
 * @brief 去初始化定时器
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_deinit(timer_handle_t handle);

/**
 * @brief 启动定时器
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_start(timer_handle_t handle);

/**
 * @brief 停止定时器
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_stop(timer_handle_t handle);

/**
 * @brief 设置定时器周期
 * 
 * @param handle 定时器句柄
 * @param period_us 定时周期(微秒)
 * @return int 0表示成功，非0表示失败
 */
int timer_set_period(timer_handle_t handle, uint32_t period_us);

/**
 * @brief 获取定时器计数值
 * 
 * @param handle 定时器句柄
 * @param value 计数值指针
 * @return int 0表示成功，非0表示失败
 */
int timer_get_count(timer_handle_t handle, uint32_t *value);

/**
 * @brief 设置定时器计数值
 * 
 * @param handle 定时器句柄
 * @param value 计数值
 * @return int 0表示成功，非0表示失败
 */
int timer_set_count(timer_handle_t handle, uint32_t value);

/**
 * @brief 设置定时器预分频值
 * 
 * @param handle 定时器句柄
 * @param prescaler 预分频值
 * @return int 0表示成功，非0表示失败
 */
int timer_set_prescaler(timer_handle_t handle, uint32_t prescaler);

/**
 * @brief 设置定时器回调函数
 * 
 * @param handle 定时器句柄
 * @param callback 定时器回调函数
 * @param arg 传递给回调函数的参数
 * @return int 0表示成功，非0表示失败
 */
int timer_set_callback(timer_handle_t handle, timer_callback_t callback, void *arg);

/**
 * @brief 使能定时器中断
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_enable_interrupt(timer_handle_t handle);

/**
 * @brief 禁用定时器中断
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_disable_interrupt(timer_handle_t handle);

/**
 * @brief 清除定时器中断标志
 * 
 * @param handle 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int timer_clear_interrupt_flag(timer_handle_t handle);

/**
 * @brief 延时指定微秒数
 * 
 * @param us 微秒数
 * @return int 0表示成功，非0表示失败
 */
int timer_delay_us(uint32_t us);

/**
 * @brief 延时指定毫秒数
 * 
 * @param ms 毫秒数
 * @return int 0表示成功，非0表示失败
 */
int timer_delay_ms(uint32_t ms);

#endif /* TIMER_API_H */
