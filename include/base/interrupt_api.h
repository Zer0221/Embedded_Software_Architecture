/**
 * @file interrupt_api.h
 * @brief 中断管理接口抽象层定义
 *
 * 该头文件定义了中断管理的统一抽象接口，使上层应用能够与底层中断处理机制解耦
 */

#ifndef INTERRUPT_API_H
#define INTERRUPT_API_H

#include <stdint.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @param handle 中断设备句柄指针
 * @return api_status_t 操作状态
 */
api_status_t interrupt_init(const interrupt_config_t *config, interrupt_handler_t handler, void *arg, interrupt_handle_t *handle);

/**
 * @brief 反初始化中断
 * 
 * @param handle 中断设备句柄
 * @return api_status_t 操作状态
 */
api_status_t interrupt_deinit(interrupt_handle_t handle);

/**
 * @brief 启用中断
 * 
 * @param handle 中断设备句柄
 * @return api_status_t 操作状态
 */
api_status_t interrupt_enable(interrupt_handle_t handle);

/**
 * @brief 禁用中断
 * 
 * @param handle 中断设备句柄
 * @return api_status_t 操作状态
 */
api_status_t interrupt_disable(interrupt_handle_t handle);

/**
 * @brief 清除中断标志
 * 
 * @param handle 中断设备句柄
 * @return api_status_t 操作状态
 */
api_status_t interrupt_clear(interrupt_handle_t handle);

/**
 * @brief 设置中断触发类型
 * 
 * @param handle 中断设备句柄
 * @param trigger 触发类型
 * @return api_status_t 操作状态
 */
api_status_t interrupt_set_trigger(interrupt_handle_t handle, interrupt_trigger_t trigger);

/**
 * @brief 设置中断优先级
 * 
 * @param handle 中断设备句柄
 * @param priority 优先级
 * @return api_status_t 操作状态
 */
api_status_t interrupt_set_priority(interrupt_handle_t handle, interrupt_priority_t priority);

/**
 * @brief 全局启用中断
 * 
 * @return api_status_t 操作状态
 */
api_status_t interrupt_global_enable(void);

/**
 * @brief 全局禁用中断
 * 
 * @return api_status_t 操作状态
 */
api_status_t interrupt_global_disable(void);

/**
 * @brief 进入临界区
 * 
 * @return uint32_t 中断状态
 */
uint32_t interrupt_enter_critical(void);

/**
 * @brief 退出临界区
 * 
 * @param state 中断状态
 */
void interrupt_exit_critical(uint32_t state);

/**
 * @brief 检查中断状态
 * 
 * @param handle 中断设备句柄
 * @param is_active 中断是否激活
 * @return api_status_t 操作状态
 */
api_status_t interrupt_is_active(interrupt_handle_t handle, bool *is_active);

/**
 * @brief 检查中断是否使能
 * 
 * @param handle 中断设备句柄
 * @param is_enabled 中断是否使能
 * @return api_status_t 操作状态
 */
api_status_t interrupt_is_enabled(interrupt_handle_t handle, bool *is_enabled);

/**
 * @brief 设置中断服务函数
 * 
 * @param handle 中断设备句柄
 * @param handler 中断处理函数
 * @param arg 传递给处理函数的参数
 * @return api_status_t 操作状态
 */
api_status_t interrupt_set_handler(interrupt_handle_t handle, interrupt_handler_t handler, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* INTERRUPT_API_H */