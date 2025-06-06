/**
 * @file watchdog_api.h
 * @brief 看门狗接口抽象层定义
 *
 * 该头文件定义了看门狗的统一抽象接口，使上层应用能够与底层看门狗硬件实现解耦
 */

#ifndef WATCHDOG_API_H
#define WATCHDOG_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 看门狗超时动作 */
typedef enum {
    WATCHDOG_ACTION_RESET,     /**< 系统复位 */
    WATCHDOG_ACTION_INTERRUPT, /**< 产生中断 */
    WATCHDOG_ACTION_BOTH       /**< 先产生中断，然后复位 */
} watchdog_action_t;

/* 看门狗配置参数 */
typedef struct {
    uint32_t timeout_ms;       /**< 超时时间(毫秒) */
    watchdog_action_t action;  /**< 超时动作 */
    bool auto_start;           /**< 是否自动启动 */
} watchdog_config_t;

/* 看门狗回调函数类型 */
typedef void (*watchdog_callback_t)(void *arg);

/* 看门狗设备句柄 */
typedef driver_handle_t watchdog_handle_t;

/**
 * @brief 初始化看门狗
 * 
 * @param config 看门狗配置参数
 * @param callback 中断回调函数(仅当action包含INTERRUPT时有效)
 * @param arg 传递给回调函数的参数
 * @param handle 看门狗句柄指针
 * @return api_status_t 操作状态
 */
api_status_t watchdog_init(const watchdog_config_t *config, watchdog_callback_t callback, 
                  void *arg, watchdog_handle_t *handle);

/**
 * @brief 去初始化看门狗
 * 
 * @param handle 看门狗句柄
 * @return api_status_t 操作状态
 */
api_status_t watchdog_deinit(watchdog_handle_t handle);

/**
 * @brief 启动看门狗
 * 
 * @param handle 看门狗句柄
 * @return api_status_t 操作状态
 */
api_status_t watchdog_start(watchdog_handle_t handle);

/**
 * @brief 停止看门狗
 * 
 * @param handle 看门狗句柄
 * @return api_status_t 操作状态
 */
api_status_t watchdog_stop(watchdog_handle_t handle);

/**
 * @brief 喂狗(重新加载计数器)
 * 
 * @param handle 看门狗句柄
 * @return api_status_t 操作状态
 */
api_status_t watchdog_feed(watchdog_handle_t handle);

/**
 * @brief 设置看门狗超时时间
 * 
 * @param handle 看门狗句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t watchdog_set_timeout(watchdog_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 获取看门狗超时时间
 * 
 * @param handle 看门狗句柄
 * @param timeout_ms 超时时间指针(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t watchdog_get_timeout(watchdog_handle_t handle, uint32_t *timeout_ms);

/**
 * @brief 设置看门狗动作
 * 
 * @param handle 看门狗句柄
 * @param action 超时动作
 * @return api_status_t 操作状态
 */
api_status_t watchdog_set_action(watchdog_handle_t handle, watchdog_action_t action);

/**
 * @brief 获取看门狗状态
 * 
 * @param handle 看门狗句柄
 * @param is_running 运行状态指针
 * @return api_status_t 操作状态
 */
api_status_t watchdog_get_status(watchdog_handle_t handle, bool *is_running);

/**
 * @brief 获取看门狗计数器
 * 
 * @param handle 看门狗句柄
 * @param count 计数器值指针
 * @return api_status_t 操作状态
 */
api_status_t watchdog_get_counter(watchdog_handle_t handle, uint32_t *count);

/**
 * @brief 获取看门狗超时剩余时间
 * 
 * @param handle 看门狗句柄
 * @param remain_ms 剩余时间指针(毫秒)
 * @return api_status_t 操作状态
 */
api_status_t watchdog_get_remain_time(watchdog_handle_t handle, uint32_t *remain_ms);

/**
 * @brief 设置看门狗回调函数
 * 
 * @param handle 看门狗句柄
 * @param callback 回调函数
 * @param arg 回调函数参数
 * @return api_status_t 操作状态
 */
api_status_t watchdog_set_callback(watchdog_handle_t handle, watchdog_callback_t callback, void *arg);

/**
 * @brief 清除看门狗中断标志
 * 
 * @param handle 看门狗句柄
 * @return api_status_t 操作状态
 */
api_status_t watchdog_clear_interrupt(watchdog_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* WATCHDOG_API_H */