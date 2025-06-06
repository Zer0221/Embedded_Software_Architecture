/**
 * @file interrupt_api.h
 * @brief 中断管理接口抽象层定义
 *
 * 该头文件定义了中断管理的统一抽象接口，使上层应用能够与底层中断处理机制解耦
 */

#ifndef INTERRUPT_API_H
#define INTERRUPT_API_H

#include <stdint.h>
#include "driver_api.h"

/* 中断触发类型 */
typedef enum {
    INTERRUPT_TRIGGER_RISING,      /**< 上升沿触发 */
    INTERRUPT_TRIGGER_FALLING,     /**< 下降沿触发 */
    INTERRUPT_TRIGGER_BOTH,        /**< 双边沿触发 */
    INTERRUPT_TRIGGER_HIGH,        /**< 高电平触发 */
    INTERRUPT_TRIGGER_LOW          /**< 低电平触发 */
} interrupt_trigger_t;

/* 中断优先级 */
typedef enum {
    INTERRUPT_PRIORITY_HIGHEST = 0,  /**< 最高优先级 */
    INTERRUPT_PRIORITY_HIGH,         /**< 高优先级 */
    INTERRUPT_PRIORITY_MEDIUM,       /**< 中优先级 */
    INTERRUPT_PRIORITY_LOW,          /**< 低优先级 */
    INTERRUPT_PRIORITY_LOWEST        /**< 最低优先级 */
} interrupt_priority_t;

/* 中断回调函数类型 */
typedef void (*interrupt_handler_t)(void *arg);

/* 中断配置参数 */
typedef struct {
    uint32_t irq_num;                  /**< 中断号 */
    interrupt_trigger_t trigger;       /**< 触发类型 */
    interrupt_priority_t priority;     /**< 优先级 */
    bool auto_clear;                   /**< 是否自动清除中断标志 */
} interrupt_config_t;

/* 中断设备句柄 */
typedef driver_handle_t interrupt_handle_t;

/**
 * @brief 初始化中断
 * 
 * @param config 中断配置参数
 * @param handler 中断处理函数
 * @param arg 传递给处理函数的参数
 * @param handle 中断句柄指针
 * @return int 0表示成功，非0表示失败
 */
int interrupt_init(const interrupt_config_t *config, interrupt_handler_t handler, void *arg, interrupt_handle_t *handle);

/**
 * @brief 去初始化中断
 * 
 * @param handle 中断句柄
 * @return int 0表示成功，非0表示失败
 */
int interrupt_deinit(interrupt_handle_t handle);

/**
 * @brief 使能中断
 * 
 * @param handle 中断句柄
 * @return int 0表示成功，非0表示失败
 */
int interrupt_enable(interrupt_handle_t handle);

/**
 * @brief 禁用中断
 * 
 * @param handle 中断句柄
 * @return int 0表示成功，非0表示失败
 */
int interrupt_disable(interrupt_handle_t handle);

/**
 * @brief 设置中断优先级
 * 
 * @param handle 中断句柄
 * @param priority 优先级
 * @return int 0表示成功，非0表示失败
 */
int interrupt_set_priority(interrupt_handle_t handle, interrupt_priority_t priority);

/**
 * @brief 设置中断触发类型
 * 
 * @param handle 中断句柄
 * @param trigger 触发类型
 * @return int 0表示成功，非0表示失败
 */
int interrupt_set_trigger(interrupt_handle_t handle, interrupt_trigger_t trigger);

/**
 * @brief 清除中断标志
 * 
 * @param handle 中断句柄
 * @return int 0表示成功，非0表示失败
 */
int interrupt_clear_flag(interrupt_handle_t handle);

/**
 * @brief 全局中断使能
 * 
 * @return int 0表示成功，非0表示失败
 */
int interrupt_global_enable(void);

/**
 * @brief 全局中断禁用
 * 
 * @return int 0表示成功，非0表示失败
 */
int interrupt_global_disable(void);

/**
 * @brief 中断锁定(进入临界区)
 * 
 * @param p_state 保存中断状态的变量指针
 * @return int 0表示成功，非0表示失败
 */
int interrupt_lock(uint32_t *p_state);

/**
 * @brief 中断解锁(退出临界区)
 * 
 * @param state 之前保存的中断状态
 * @return int 0表示成功，非0表示失败
 */
int interrupt_unlock(uint32_t state);

/**
 * @brief 获取当前是否在中断上下文中
 * 
 * @return bool true表示在中断上下文中，false表示不在
 */
bool interrupt_is_in_isr(void);

#endif /* INTERRUPT_API_H */
