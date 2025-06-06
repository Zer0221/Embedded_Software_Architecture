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
#include "driver_api.h"

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
 * @return int 0表示成功，非0表示失败
 */
int watchdog_init(const watchdog_config_t *config, watchdog_callback_t callback, 
                  void *arg, watchdog_handle_t *handle);

/**
 * @brief 去初始化看门狗
 * 
 * @param handle 看门狗句柄
 * @return int 0表示成功，非0表示失败
 */
int watchdog_deinit(watchdog_handle_t handle);

/**
 * @brief 启动看门狗
 * 
 * @param handle 看门狗句柄
 * @return int 0表示成功，非0表示失败
 */
int watchdog_start(watchdog_handle_t handle);

/**
 * @brief 停止看门狗
 * 
 * @param handle 看门狗句柄
 * @return int 0表示成功，非0表示失败
 */
int watchdog_stop(watchdog_handle_t handle);

/**
 * @brief 喂狗/刷新看门狗
 * 
 * @param handle 看门狗句柄
 * @return int 0表示成功，非0表示失败
 */
int watchdog_feed(watchdog_handle_t handle);

/**
 * @brief 设置看门狗超时时间
 * 
 * @param handle 看门狗句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int watchdog_set_timeout(watchdog_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 获取看门狗超时时间
 * 
 * @param handle 看门狗句柄
 * @param timeout_ms 超时时间指针(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int watchdog_get_timeout(watchdog_handle_t handle, uint32_t *timeout_ms);

/**
 * @brief 获取看门狗剩余时间
 * 
 * @param handle 看门狗句柄
 * @param remaining_ms 剩余时间指针(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int watchdog_get_remaining(watchdog_handle_t handle, uint32_t *remaining_ms);

/**
 * @brief 设置看门狗回调函数
 * 
 * @param handle 看门狗句柄
 * @param callback 中断回调函数
 * @param arg 传递给回调函数的参数
 * @return int 0表示成功，非0表示失败
 */
int watchdog_set_callback(watchdog_handle_t handle, watchdog_callback_t callback, void *arg);

/**
 * @brief 检查是否由看门狗触发的复位
 * 
 * @param handle 看门狗句柄
 * @param is_watchdog_reset 复位标志指针
 * @return int 0表示成功，非0表示失败
 */
int watchdog_is_reset_by_watchdog(watchdog_handle_t handle, bool *is_watchdog_reset);

/**
 * @brief 清除看门狗复位标志
 * 
 * @param handle 看门狗句柄
 * @return int 0表示成功，非0表示失败
 */
int watchdog_clear_reset_flag(watchdog_handle_t handle);

#endif /* WATCHDOG_API_H */
