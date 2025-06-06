/**
 * @file cpu_freq_api.h
 * @brief CPU频率管理接口定义
 *
 * 该头文件定义了CPU频率和性能管理的统一接口，支持动态频率调整、性能模式切换等
 */

#ifndef CPU_FREQ_API_H
#define CPU_FREQ_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CPU频率级别 */
typedef enum {
    CPU_FREQ_LEVEL_MIN,       /**< 最低频率 */
    CPU_FREQ_LEVEL_LOW,       /**< 低频率 */
    CPU_FREQ_LEVEL_MEDIUM,    /**< 中等频率 */
    CPU_FREQ_LEVEL_HIGH,      /**< 高频率 */
    CPU_FREQ_LEVEL_MAX,       /**< 最高频率 */
    CPU_FREQ_LEVEL_CUSTOM     /**< 自定义频率 */
} cpu_freq_level_t;

/* CPU性能模式 */
typedef enum {
    CPU_PERF_MODE_POWERSAVE,  /**< 省电模式 */
    CPU_PERF_MODE_BALANCED,   /**< 平衡模式 */
    CPU_PERF_MODE_PERFORMANCE /**< 性能模式 */
} cpu_perf_mode_t;

/* 动态频率调整策略 */
typedef enum {
    CPU_DVFS_POLICY_MANUAL,   /**< 手动调整 */
    CPU_DVFS_POLICY_ONDEMAND, /**< 按需调整 */
    CPU_DVFS_POLICY_POWERSAVE,/**< 省电优先 */
    CPU_DVFS_POLICY_PERFORMANCE/**< 性能优先 */
} cpu_dvfs_policy_t;

/* CPU频率信息 */
typedef struct {
    uint32_t current_freq_hz;  /**< 当前频率(Hz) */
    uint32_t min_freq_hz;      /**< 最小频率(Hz) */
    uint32_t max_freq_hz;      /**< 最大频率(Hz) */
    cpu_freq_level_t level;    /**< 当前频率级别 */
    cpu_perf_mode_t mode;      /**< 当前性能模式 */
    cpu_dvfs_policy_t policy;  /**< 动态频率调整策略 */
} cpu_freq_info_t;

/* CPU工作模式 */
typedef enum {
    CPU_MODE_NORMAL,         /**< 正常工作模式 */
    CPU_MODE_IDLE,           /**< 空闲模式 */
    CPU_MODE_SLEEP,          /**< 睡眠模式 */
    CPU_MODE_DEEP_SLEEP,     /**< 深度睡眠模式 */
    CPU_MODE_STANDBY         /**< 待机模式 */
} cpu_mode_t;

/* CPU监控回调函数类型 */
typedef void (*cpu_monitor_callback_t)(uint32_t load_percent, void *user_data);

/**
 * @brief 初始化CPU频率管理
 *
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_init(void);

/**
 * @brief 反初始化CPU频率管理
 *
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_deinit(void);

/**
 * @brief 设置CPU频率级别
 *
 * @param level 频率级别
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_set_level(cpu_freq_level_t level);

/**
 * @brief 获取当前CPU频率级别
 *
 * @param level 获取到的频率级别
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_level(cpu_freq_level_t *level);

/**
 * @brief 设置CPU性能模式
 *
 * @param mode 性能模式
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_set_perf_mode(cpu_perf_mode_t mode);

/**
 * @brief 获取当前CPU性能模式
 *
 * @param mode 获取到的性能模式
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_perf_mode(cpu_perf_mode_t *mode);

/**
 * @brief 设置CPU频率(Hz)
 *
 * @param freq_hz 频率值(Hz)
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_set_freq(uint32_t freq_hz);

/**
 * @brief 获取当前CPU频率(Hz)
 *
 * @param freq_hz 获取到的频率值(Hz)
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_freq(uint32_t *freq_hz);

/**
 * @brief 获取CPU频率范围
 *
 * @param min_freq_hz 最小频率(Hz)
 * @param max_freq_hz 最大频率(Hz)
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_range(uint32_t *min_freq_hz, uint32_t *max_freq_hz);

/**
 * @brief 设置动态频率调整策略
 *
 * @param policy 策略
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_set_dvfs_policy(cpu_dvfs_policy_t policy);

/**
 * @brief 获取当前动态频率调整策略
 *
 * @param policy 获取到的策略
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_dvfs_policy(cpu_dvfs_policy_t *policy);

/**
 * @brief 获取CPU频率详细信息
 *
 * @param info 频率信息结构体
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_info(cpu_freq_info_t *info);

/**
 * @brief 获取CPU利用率
 *
 * @param load_percent CPU利用率百分比
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_get_load(uint32_t *load_percent);

/**
 * @brief 注册CPU负载监控回调
 *
 * @param callback 回调函数
 * @param threshold_percent 触发阈值(百分比)
 * @param user_data 用户数据
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_register_monitor(cpu_monitor_callback_t callback, uint32_t threshold_percent, void *user_data);

/**
 * @brief 取消注册CPU负载监控回调
 *
 * @param callback 回调函数
 * @return api_status_t 操作状态
 */
api_status_t cpu_freq_unregister_monitor(cpu_monitor_callback_t callback);

/**
 * @brief 设置CPU工作模式
 *
 * @param mode 工作模式
 * @return api_status_t 操作状态
 */
api_status_t cpu_mode_set(cpu_mode_t mode);

/**
 * @brief 获取当前CPU工作模式
 *
 * @param mode 获取到的工作模式
 * @return api_status_t 操作状态
 */
api_status_t cpu_mode_get(cpu_mode_t *mode);

/**
 * @brief 获取当前CPU核心数量
 *
 * @param core_count 核心数量
 * @return api_status_t 操作状态
 */
api_status_t cpu_get_core_count(uint32_t *core_count);

/**
 * @brief 获取指定CPU核心的频率
 *
 * @param core_id 核心ID
 * @param freq_hz 获取到的频率值(Hz)
 * @return api_status_t 操作状态
 */
api_status_t cpu_get_core_freq(uint32_t core_id, uint32_t *freq_hz);

/**
 * @brief 设置指定CPU核心的频率
 *
 * @param core_id 核心ID
 * @param freq_hz 频率值(Hz)
 * @return api_status_t 操作状态
 */
api_status_t cpu_set_core_freq(uint32_t core_id, uint32_t freq_hz);

/**
 * @brief 禁用指定CPU核心
 *
 * @param core_id 核心ID
 * @return api_status_t 操作状态
 */
api_status_t cpu_disable_core(uint32_t core_id);

/**
 * @brief 启用指定CPU核心
 *
 * @param core_id 核心ID
 * @return api_status_t 操作状态
 */
api_status_t cpu_enable_core(uint32_t core_id);

#ifdef __cplusplus
}
#endif

#endif /* CPU_FREQ_API_H */