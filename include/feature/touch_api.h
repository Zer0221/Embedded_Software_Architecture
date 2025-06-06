/**
 * @file touch_api.h
 * @brief 触摸传感器接口定义
 *
 * 该头文件定义了触摸传感器的统一接口，支持电容式触摸和电阻式触摸屏
 */

#ifndef TOUCH_API_H
#define TOUCH_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 触摸句柄 */
typedef void* touch_handle_t;

/* 触摸类型 */
typedef enum {
    TOUCH_TYPE_CAPACITIVE = 0,   /**< 电容式触摸传感器 */
    TOUCH_TYPE_RESISTIVE,        /**< 电阻式触摸屏 */
    TOUCH_TYPE_INFRARED,         /**< 红外触摸屏 */
    TOUCH_TYPE_SURFACE_ACOUSTIC  /**< 表面声波触摸屏 */
} touch_type_t;

/* 触摸配置 */
typedef struct {
    touch_type_t type;            /**< 触摸类型 */
    uint8_t channel_num;          /**< 触摸通道数量 */
    uint16_t threshold;           /**< 触摸阈值 */
    uint16_t filter_level;        /**< 滤波级别 */
    uint8_t sensitivity;          /**< 灵敏度 */
    bool low_power_mode;          /**< 是否使用低功耗模式 */
    uint16_t debounce_time;       /**< 去抖时间（毫秒） */
    uint16_t sample_rate;         /**< 采样率（Hz） */
} touch_config_t;

/* 触摸点信息 */
typedef struct {
    uint16_t x;                   /**< X坐标 */
    uint16_t y;                   /**< Y坐标 */
    uint16_t pressure;            /**< 压力值，0表示释放 */
    uint8_t id;                   /**< 触摸点ID，用于多点触控 */
} touch_point_t;

/* 触摸状态 */
typedef struct {
    uint32_t active_channels;     /**< 活动通道位图 */
    uint16_t raw_values[32];      /**< 各通道原始值 */
    uint8_t touch_count;          /**< 触摸点数量 */
    touch_point_t points[10];     /**< 触摸点数据，最多支持10点 */
} touch_state_t;

/* 触摸回调函数类型 */
typedef void (*touch_cb_t)(touch_state_t *state, void *user_data);

/**
 * @brief 初始化触摸控制器
 * 
 * @param config 触摸配置
 * @param cb 触摸回调函数
 * @param user_data 用户数据，会传递给回调函数
 * @return touch_handle_t 触摸句柄，NULL表示失败
 */
touch_handle_t touch_init(touch_config_t *config, touch_cb_t cb, void *user_data);

/**
 * @brief 释放触摸控制器
 * 
 * @param handle 触摸句柄
 * @return int 0表示成功，负值表示失败
 */
int touch_deinit(touch_handle_t handle);

/**
 * @brief 启动触摸检测
 * 
 * @param handle 触摸句柄
 * @return int 0表示成功，负值表示失败
 */
int touch_start(touch_handle_t handle);

/**
 * @brief 停止触摸检测
 * 
 * @param handle 触摸句柄
 * @return int 0表示成功，负值表示失败
 */
int touch_stop(touch_handle_t handle);

/**
 * @brief 获取触摸状态（同步方式）
 * 
 * @param handle 触摸句柄
 * @param state 触摸状态
 * @return int 0表示成功，负值表示失败
 */
int touch_get_state(touch_handle_t handle, touch_state_t *state);

/**
 * @brief 设置触摸通道阈值
 * 
 * @param handle 触摸句柄
 * @param channel 通道索引
 * @param threshold 阈值
 * @return int 0表示成功，负值表示失败
 */
int touch_set_threshold(touch_handle_t handle, uint8_t channel, uint16_t threshold);

/**
 * @brief 校准触摸传感器
 * 
 * @param handle 触摸句柄
 * @return int 0表示成功，负值表示失败
 */
int touch_calibrate(touch_handle_t handle);

/**
 * @brief 获取触摸通道原始值
 * 
 * @param handle 触摸句柄
 * @param channel 通道索引
 * @param value 原始值
 * @return int 0表示成功，负值表示失败
 */
int touch_get_raw_value(touch_handle_t handle, uint8_t channel, uint16_t *value);

/**
 * @brief 设置触摸滤波级别
 * 
 * @param handle 触摸句柄
 * @param level 滤波级别
 * @return int 0表示成功，负值表示失败
 */
int touch_set_filter_level(touch_handle_t handle, uint16_t level);

/**
 * @brief 设置触摸唤醒功能
 * 
 * @param handle 触摸句柄
 * @param channel 唤醒通道
 * @param enable 是否启用
 * @return int 0表示成功，负值表示失败
 */
int touch_set_wakeup(touch_handle_t handle, uint8_t channel, bool enable);

/**
 * @brief 设置触摸防水功能
 * 
 * @param handle 触摸句柄
 * @param enable 是否启用
 * @return int 0表示成功，负值表示失败
 */
int touch_set_waterproof(touch_handle_t handle, bool enable);

/**
 * @brief 设置触摸采样率
 * 
 * @param handle 触摸句柄
 * @param rate 采样率（Hz）
 * @return int 0表示成功，负值表示失败
 */
int touch_set_sample_rate(touch_handle_t handle, uint16_t rate);

/**
 * @brief 设置触摸低功耗模式
 * 
 * @param handle 触摸句柄
 * @param enable 是否启用
 * @return int 0表示成功，负值表示失败
 */
int touch_set_low_power_mode(touch_handle_t handle, bool enable);

#endif /* TOUCH_API_H */
