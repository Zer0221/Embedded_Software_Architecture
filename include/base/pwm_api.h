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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    bool enabled;                     /**< 是否使能 */
} pwm_config_t;

/* PWM设备句柄 */
typedef driver_handle_t pwm_handle_t;

/**
 * @brief 初始化PWM通道
 * 
 * @param config PWM配置参数
 * @param handle PWM设备句柄指针
 * @return api_status_t 操作状态
 */
api_status_t pwm_init(const pwm_config_t *config, pwm_handle_t *handle);

/**
 * @brief 反初始化PWM通道
 * 
 * @param handle PWM设备句柄
 * @return api_status_t 操作状态
 */
api_status_t pwm_deinit(pwm_handle_t handle);

/**
 * @brief 启动PWM输出
 * 
 * @param handle PWM设备句柄
 * @return api_status_t 操作状态
 */
api_status_t pwm_start(pwm_handle_t handle);

/**
 * @brief 停止PWM输出
 * 
 * @param handle PWM设备句柄
 * @return api_status_t 操作状态
 */
api_status_t pwm_stop(pwm_handle_t handle);

/**
 * @brief 设置PWM频率
 * 
 * @param handle PWM设备句柄
 * @param frequency 频率(Hz)
 * @return api_status_t 操作状态
 */
api_status_t pwm_set_frequency(pwm_handle_t handle, uint32_t frequency);

/**
 * @brief 获取PWM频率
 * 
 * @param handle PWM设备句柄
 * @param frequency 频率指针(Hz)
 * @return api_status_t 操作状态
 */
api_status_t pwm_get_frequency(pwm_handle_t handle, uint32_t *frequency);

/**
 * @brief 设置PWM占空比
 * 
 * @param handle PWM设备句柄
 * @param duty_cycle 占空比(0.0-1.0)
 * @return api_status_t 操作状态
 */
api_status_t pwm_set_duty_cycle(pwm_handle_t handle, float duty_cycle);

/**
 * @brief 获取PWM占空比
 * 
 * @param handle PWM设备句柄
 * @param duty_cycle 占空比指针(0.0-1.0)
 * @return api_status_t 操作状态
 */
api_status_t pwm_get_duty_cycle(pwm_handle_t handle, float *duty_cycle);

/**
 * @brief 设置PWM极性
 * 
 * @param handle PWM设备句柄
 * @param polarity 极性
 * @return api_status_t 操作状态
 */
api_status_t pwm_set_polarity(pwm_handle_t handle, pwm_polarity_t polarity);

/**
 * @brief 设置PWM对齐模式
 * 
 * @param handle PWM设备句柄
 * @param align_mode 对齐模式
 * @return api_status_t 操作状态
 */
api_status_t pwm_set_align_mode(pwm_handle_t handle, pwm_align_mode_t align_mode);

/**
 * @brief 设置PWM计数器模式
 * 
 * @param handle PWM设备句柄
 * @param counter_mode 计数器模式
 * @return api_status_t 操作状态
 */
api_status_t pwm_set_counter_mode(pwm_handle_t handle, pwm_counter_mode_t counter_mode);

/**
 * @brief 获取PWM是否启用
 * 
 * @param handle PWM设备句柄
 * @param enabled 启用状态指针
 * @return api_status_t 操作状态
 */
api_status_t pwm_is_enabled(pwm_handle_t handle, bool *enabled);

/**
 * @brief 获取PWM通道配置
 * 
 * @param handle PWM设备句柄
 * @param config 配置参数指针
 * @return api_status_t 操作状态
 */
api_status_t pwm_get_config(pwm_handle_t handle, pwm_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* PWM_API_H */