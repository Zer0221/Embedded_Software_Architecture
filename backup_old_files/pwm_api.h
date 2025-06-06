/**
 * @file pwm_api.h
 * @brief PWM接口抽象层定义
 *
 * 该头文件定义了PWM接口的统一抽象，使上层应用能够与底层PWM硬件实现解耦
 */

#ifndef PWM_API_H
#define PWM_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* PWM通道ID定义 */
typedef enum {
    PWM_CHANNEL_0 = 0,    /**< PWM通道0 */
    PWM_CHANNEL_1,        /**< PWM通道1 */
    PWM_CHANNEL_2,        /**< PWM通道2 */
    PWM_CHANNEL_3,        /**< PWM通道3 */
    PWM_CHANNEL_4,        /**< PWM通道4 */
    PWM_CHANNEL_5,        /**< PWM通道5 */
    PWM_CHANNEL_6,        /**< PWM通道6 */
    PWM_CHANNEL_7,        /**< PWM通道7 */
    PWM_CHANNEL_MAX       /**< PWM通道最大数量 */
} pwm_channel_t;

/* PWM对齐模式定义 */
typedef enum {
    PWM_ALIGN_EDGE = 0,    /**< 边沿对齐模式 */
    PWM_ALIGN_CENTER,      /**< 中心对齐模式 */
} pwm_align_mode_t;

/* PWM极性定义 */
typedef enum {
    PWM_POLARITY_NORMAL = 0,    /**< 正常极性(高电平有效) */
    PWM_POLARITY_INVERTED,      /**< 反转极性(低电平有效) */
} pwm_polarity_t;

/* PWM计数器模式定义 */
typedef enum {
    PWM_COUNTER_UP = 0,        /**< 向上计数模式 */
    PWM_COUNTER_DOWN,          /**< 向下计数模式 */
    PWM_COUNTER_UP_DOWN,       /**< 上下计数模式 */
} pwm_counter_mode_t;

/* PWM配置参数 */
typedef struct {
    pwm_channel_t channel;            /**< PWM通道 */
    uint32_t frequency;               /**< PWM频率(Hz) */
    float duty_cycle;                 /**< 占空比(0.0-1.0) */
    pwm_align_mode_t align_mode;      /**< 对齐模式 */
    pwm_polarity_t polarity;          /**< 极性 */
    pwm_counter_mode_t counter_mode;  /**< 计数器模式 */
    bool complementary;               /**< 是否使用互补输出 */
    uint32_t dead_time;               /**< 死区时间(ns)，仅互补模式有效 */
} pwm_config_t;

/* PWM回调函数类型 */
typedef void (*pwm_callback_t)(void *user_data);

/* PWM事件类型 */
typedef enum {
    PWM_EVENT_PERIOD_ELAPSED = 0,    /**< 周期结束事件 */
    PWM_EVENT_PULSE_FINISHED,        /**< 脉冲完成事件 */
    PWM_EVENT_BREAK,                 /**< 中断事件 */
} pwm_event_t;

/* PWM设备句柄 */
typedef driver_handle_t pwm_handle_t;

/**
 * @brief 初始化PWM
 * 
 * @param config PWM配置参数
 * @param handle PWM设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int pwm_init(const pwm_config_t *config, pwm_handle_t *handle);

/**
 * @brief 去初始化PWM
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_deinit(pwm_handle_t handle);

/**
 * @brief 启动PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_start(pwm_handle_t handle);

/**
 * @brief 停止PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_stop(pwm_handle_t handle);

/**
 * @brief 设置PWM频率
 * 
 * @param handle PWM设备句柄
 * @param frequency 频率(Hz)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_frequency(pwm_handle_t handle, uint32_t frequency);

/**
 * @brief 设置PWM占空比
 * 
 * @param handle PWM设备句柄
 * @param duty_cycle 占空比(0.0-1.0)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_duty_cycle(pwm_handle_t handle, float duty_cycle);

/**
 * @brief 设置PWM死区时间
 * 
 * @param handle PWM设备句柄
 * @param dead_time 死区时间(ns)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_dead_time(pwm_handle_t handle, uint32_t dead_time);

/**
 * @brief 注册PWM事件回调函数
 * 
 * @param handle PWM设备句柄
 * @param event 事件类型
 * @param callback 回调函数
 * @param user_data 用户数据，传递给回调函数
 * @return int 0表示成功，非0表示失败
 */
int pwm_register_callback(pwm_handle_t handle, pwm_event_t event, 
                         pwm_callback_t callback, void *user_data);

/**
 * @brief 取消注册PWM事件回调函数
 * 
 * @param handle PWM设备句柄
 * @param event 事件类型
 * @return int 0表示成功，非0表示失败
 */
int pwm_unregister_callback(pwm_handle_t handle, pwm_event_t event);

/**
 * @brief 生成PWM脉冲
 * 
 * @param handle PWM设备句柄
 * @param pulse_count 脉冲数量
 * @return int 0表示成功，非0表示失败
 */
int pwm_generate_pulse(pwm_handle_t handle, uint32_t pulse_count);

#endif /* PWM_API_H */
