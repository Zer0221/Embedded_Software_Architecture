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
#include "driver_api.h"
#include "error_api.h"

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
    cpu_dvfs_policy_t policy;  /**< 当前调频策略 */
    uint8_t usage_percent;     /**< CPU使用率(百分比) */
    uint32_t supported_freqs[8];/**< 支持的频率列表(Hz) */
    uint8_t supported_freq_count;/**< 支持的频率数量 */
    float voltage;             /**< 当前电压(V) */
    float temperature;         /**< 当前温度(°C) */
} cpu_freq_info_t;

/* CPU频率配置 */
typedef struct {
    cpu_dvfs_policy_t policy;   /**< 动态调频策略 */
    cpu_perf_mode_t initial_mode;/**< 初始性能模式 */
    bool enable_thermal_throttling;/**< 是否启用温度限制 */
    float thermal_throttle_temp;/**< 温度限制阈值(°C) */
    uint8_t usage_high_threshold;/**< 使用率高阈值(百分比) */
    uint8_t usage_low_threshold;/**< 使用率低阈值(百分比) */
    uint32_t sampling_ms;      /**< 采样间隔(毫秒) */
    bool lock_freq_during_sleep;/**< 睡眠期间是否锁定频率 */
    uint32_t custom_freq_hz;   /**< 自定义频率(Hz) */
} cpu_freq_config_t;

/* CPU频率句柄 */
typedef void* cpu_freq_handle_t;

/* CPU频率事件回调函数 */
typedef void (*cpu_freq_callback_t)(cpu_freq_handle_t handle, cpu_freq_info_t* info, void* user_data);

/**
 * @brief 初始化CPU频率管理
 * 
 * @param config 配置参数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @param handle 返回的句柄
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_init(const cpu_freq_config_t* config, cpu_freq_callback_t callback, void* user_data, cpu_freq_handle_t* handle);

/**
 * @brief 反初始化CPU频率管理
 * 
 * @param handle 句柄
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_deinit(cpu_freq_handle_t handle);

/**
 * @brief 设置CPU频率级别
 * 
 * @param handle 句柄
 * @param level 频率级别
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_level(cpu_freq_handle_t handle, cpu_freq_level_t level);

/**
 * @brief 获取CPU频率级别
 * 
 * @param handle 句柄
 * @param level 返回的频率级别
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_level(cpu_freq_handle_t handle, cpu_freq_level_t* level);

/**
 * @brief 设置CPU性能模式
 * 
 * @param handle 句柄
 * @param mode 性能模式
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_perf_mode(cpu_freq_handle_t handle, cpu_perf_mode_t mode);

/**
 * @brief 获取CPU性能模式
 * 
 * @param handle 句柄
 * @param mode 返回的性能模式
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_perf_mode(cpu_freq_handle_t handle, cpu_perf_mode_t* mode);

/**
 * @brief 设置自定义CPU频率
 * 
 * @param handle 句柄
 * @param freq_hz 频率(Hz)
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_custom(cpu_freq_handle_t handle, uint32_t freq_hz);

/**
 * @brief 获取CPU频率信息
 * 
 * @param handle 句柄
 * @param info 返回的频率信息
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_info(cpu_freq_handle_t handle, cpu_freq_info_t* info);

/**
 * @brief 锁定CPU频率
 * 
 * 锁定后，动态调频策略将不生效，直到解锁
 * 
 * @param handle 句柄
 * @param level 锁定的频率级别
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_lock(cpu_freq_handle_t handle, cpu_freq_level_t level);

/**
 * @brief 解锁CPU频率
 * 
 * 解锁后，动态调频策略将重新生效
 * 
 * @param handle 句柄
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_unlock(cpu_freq_handle_t handle);

/**
 * @brief 设置动态调频策略
 * 
 * @param handle 句柄
 * @param policy 调频策略
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_policy(cpu_freq_handle_t handle, cpu_dvfs_policy_t policy);

/**
 * @brief 获取动态调频策略
 * 
 * @param handle 句柄
 * @param policy 返回的调频策略
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_policy(cpu_freq_handle_t handle, cpu_dvfs_policy_t* policy);

/**
 * @brief 获取CPU使用率
 * 
 * @param handle 句柄
 * @param usage_percent 返回的使用率(百分比)
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_usage(cpu_freq_handle_t handle, uint8_t* usage_percent);

/**
 * @brief 设置使用率阈值
 * 
 * 仅在CPU_DVFS_POLICY_ONDEMAND策略下有效
 * 
 * @param handle 句柄
 * @param high 高阈值(百分比)
 * @param low 低阈值(百分比)
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_thresholds(cpu_freq_handle_t handle, uint8_t high, uint8_t low);

/**
 * @brief 设置CPU核心使能状态
 * 
 * 对于多核处理器，可以开启或关闭特定核心
 * 
 * @param handle 句柄
 * @param core_id 核心ID
 * @param enable 是否使能
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_set_core_enabled(cpu_freq_handle_t handle, uint8_t core_id, bool enable);

/**
 * @brief 获取CPU核心使能状态
 * 
 * @param handle 句柄
 * @param core_id 核心ID
 * @param enabled 返回的使能状态
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_core_enabled(cpu_freq_handle_t handle, uint8_t core_id, bool* enabled);

/**
 * @brief 获取CPU核心数量
 * 
 * @param handle 句柄
 * @param count 返回的核心数量
 * @return int 成功返回0，失败返回错误码
 */
int cpu_freq_get_core_count(cpu_freq_handle_t handle, uint8_t* count);

#ifdef __cplusplus
}
#endif

#endif /* CPU_FREQ_API_H */
