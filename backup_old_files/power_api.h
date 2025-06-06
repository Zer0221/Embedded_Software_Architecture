/**
 * @file power_api.h
 * @brief 电源管理接口定义
 *
 * 该头文件定义了电源管理接口，提供了低功耗模式控制、电源状态监控和唤醒源管理功能
 */

#ifndef POWER_API_H
#define POWER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 电源模式定义 */
typedef enum {
    POWER_MODE_ACTIVE = 0,      /**< 正常运行模式 */
    POWER_MODE_SLEEP,           /**< 睡眠模式，CPU停止，外设运行 */
    POWER_MODE_DEEP_SLEEP,      /**< 深度睡眠模式，CPU和大部分外设停止 */
    POWER_MODE_STANDBY,         /**< 待机模式，只保留RTC和备份寄存器 */
    POWER_MODE_SHUTDOWN,        /**< 关机模式，完全断电，仅少数唤醒源有效 */
    POWER_MODE_MAX              /**< 电源模式最大数量 */
} power_mode_t;

/* 唤醒源定义 */
typedef enum {
    WAKEUP_SOURCE_NONE = 0x00,       /**< 无唤醒源 */
    WAKEUP_SOURCE_PIN = 0x01,         /**< 引脚唤醒 */
    WAKEUP_SOURCE_RTC_ALARM = 0x02,   /**< RTC闹钟唤醒 */
    WAKEUP_SOURCE_RTC_TIMESTAMP = 0x04, /**< RTC时间戳唤醒 */
    WAKEUP_SOURCE_WATCHDOG = 0x08,    /**< 看门狗唤醒 */
    WAKEUP_SOURCE_RESET = 0x10,       /**< 复位唤醒 */
    WAKEUP_SOURCE_ALL = 0xFF          /**< 所有唤醒源 */
} wakeup_source_t;

/* 电池类型定义 */
typedef enum {
    BATTERY_TYPE_NONE = 0,      /**< 无电池 */
    BATTERY_TYPE_LIPO,          /**< 锂聚合物电池 */
    BATTERY_TYPE_LIION,         /**< 锂离子电池 */
    BATTERY_TYPE_ALKALINE,      /**< 碱性电池 */
    BATTERY_TYPE_NIMH,          /**< 镍氢电池 */
    BATTERY_TYPE_LEAD_ACID,     /**< 铅酸电池 */
    BATTERY_TYPE_LIPO_HV,       /**< 高压锂聚合物电池 */
    BATTERY_TYPE_CUSTOM         /**< 自定义电池类型 */
} battery_type_t;

/* 电池状态定义 */
typedef enum {
    BATTERY_STATUS_UNKNOWN = 0, /**< 未知状态 */
    BATTERY_STATUS_NORMAL,      /**< 正常状态 */
    BATTERY_STATUS_LOW,         /**< 低电量状态 */
    BATTERY_STATUS_CRITICAL,    /**< 临界低电量状态 */
    BATTERY_STATUS_DEPLETED     /**< 电量耗尽 */
} battery_status_t;

/* 电池健康状态定义 */
typedef enum {
    BATTERY_HEALTH_UNKNOWN = 0, /**< 未知状态 */
    BATTERY_HEALTH_GOOD,        /**< 良好状态 */
    BATTERY_HEALTH_FAIR,        /**< 一般状态 */
    BATTERY_HEALTH_POOR,        /**< 较差状态 */
    BATTERY_HEALTH_FAILING      /**< 故障状态 */
} battery_health_t;

/* 充电状态定义 */
typedef enum {
    CHARGE_STATUS_UNKNOWN = 0,  /**< 未知状态 */
    CHARGE_STATUS_NONE,         /**< 未充电 */
    CHARGE_STATUS_TRICKLE,      /**< 涓流充电 */
    CHARGE_STATUS_NORMAL,       /**< 正常充电 */
    CHARGE_STATUS_FAST,         /**< 快速充电 */
    CHARGE_STATUS_COMPLETE,     /**< 充电完成 */
    CHARGE_STATUS_ERROR         /**< 充电错误 */
} charge_status_t;

/* 电源状态定义 */
typedef enum {
    POWER_STATE_UNKNOWN = 0,    /**< 未知状态 */
    POWER_STATE_BATTERY,        /**< 电池供电 */
    POWER_STATE_CHARGING,       /**< 充电中 */
    POWER_STATE_CHARGED,        /**< 已充满 */
    POWER_STATE_EXTERNAL,       /**< 外部电源供电 */
    POWER_STATE_USB,            /**< USB供电 */
    POWER_STATE_LOW_POWER       /**< 低功耗状态 */
} power_state_t;

/* 电源回调函数类型 */
typedef void (*power_callback_t)(power_mode_t mode, wakeup_source_t source, void *user_data);

/* 电源配置参数 */
typedef struct {
    bool enable_auto_sleep;          /**< 是否启用自动睡眠 */
    uint32_t auto_sleep_timeout_ms;  /**< 自动睡眠超时时间（毫秒） */
    uint32_t wakeup_sources;         /**< 唤醒源，多个唤醒源可通过或运算组合 */
    battery_type_t battery_type;     /**< 电池类型 */
    float battery_low_threshold;     /**< 电池低电量阈值（伏特） */
    float battery_critical_threshold; /**< 电池临界电量阈值（伏特） */
    bool enable_battery_monitor;     /**< 是否启用电池监控 */
    uint32_t battery_monitor_interval_ms; /**< 电池监控间隔（毫秒） */
    bool enable_power_saving;        /**< 是否启用节能模式 */
    uint8_t power_saving_level;      /**< 节能级别（0-100）*/
    bool enable_thermal_protection;  /**< 是否启用热保护 */
    float thermal_shutdown_temp;     /**< 热保护关机温度（摄氏度） */
} power_config_t;

/* 电源设备句柄 */
typedef driver_handle_t power_handle_t;

/**
 * @brief 初始化电源管理
 * 
 * @param config 电源配置参数
 * @param handle 电源设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int power_init(const power_config_t *config, power_handle_t *handle);

/**
 * @brief 去初始化电源管理
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_deinit(power_handle_t handle);

/**
 * @brief 设置电源模式
 * 
 * @param handle 电源设备句柄
 * @param mode 电源模式
 * @param timeout_ms 超时时间（毫秒），0表示永久生效，非0表示超时后自动唤醒
 * @return int 0表示成功，非0表示失败
 */
int power_set_mode(power_handle_t handle, power_mode_t mode, uint32_t timeout_ms);

/**
 * @brief 获取当前电源模式
 * 
 * @param handle 电源设备句柄
 * @param mode 电源模式指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_mode(power_handle_t handle, power_mode_t *mode);

/**
 * @brief 获取唤醒源
 * 
 * @param handle 电源设备句柄
 * @param source 唤醒源指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_wakeup_source(power_handle_t handle, uint32_t *source);

/**
 * @brief 配置唤醒源
 * 
 * @param handle 电源设备句柄
 * @param sources 唤醒源，多个唤醒源可通过或运算组合
 * @param enable 是否使能
 * @return int 0表示成功，非0表示失败
 */
int power_config_wakeup_source(power_handle_t handle, uint32_t sources, bool enable);

/**
 * @brief 重置唤醒源状态
 * 
 * @param handle 电源设备句柄
 * @param sources 唤醒源，多个唤醒源可通过或运算组合
 * @return int 0表示成功，非0表示失败
 */
int power_reset_wakeup_source(power_handle_t handle, uint32_t sources);

/**
 * @brief 注册电源回调函数
 * 
 * @param handle 电源设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int power_register_callback(power_handle_t handle, power_callback_t callback, void *user_data);

/**
 * @brief 取消注册电源回调函数
 * 
 * @param handle 电源设备句柄
 * @param callback 回调函数
 * @return int 0表示成功，非0表示失败
 */
int power_unregister_callback(power_handle_t handle, power_callback_t callback);

/**
 * @brief 获取电池电压
 * 
 * @param handle 电源设备句柄
 * @param voltage 电压值指针（伏特）
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_voltage(power_handle_t handle, float *voltage);

/**
 * @brief 获取电池电量百分比
 * 
 * @param handle 电源设备句柄
 * @param percentage 电量百分比指针（0-100）
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_percentage(power_handle_t handle, uint8_t *percentage);

/**
 * @brief 获取电源状态
 * 
 * @param handle 电源设备句柄
 * @param state 电源状态指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_state(power_handle_t handle, power_state_t *state);

/**
 * @brief 获取电池状态
 * 
 * @param handle 电源设备句柄
 * @param status 电池状态指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_status(power_handle_t handle, battery_status_t *status);

/**
 * @brief 获取电池健康状态
 * 
 * @param handle 电源设备句柄
 * @param health 电池健康状态指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_health(power_handle_t handle, battery_health_t *health);

/**
 * @brief 获取充电状态
 * 
 * @param handle 电源设备句柄
 * @param status 充电状态指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_charge_status(power_handle_t handle, charge_status_t *status);

/**
 * @brief 获取当前设备温度
 * 
 * @param handle 电源设备句柄
 * @param temperature 温度指针（摄氏度）
 * @return int 0表示成功，非0表示失败
 */
int power_get_temperature(power_handle_t handle, float *temperature);

/**
 * @brief 启用/禁用电池监控
 * 
 * @param handle 电源设备句柄
 * @param enable 是否启用
 * @param interval_ms 监控间隔（毫秒）
 * @return int 0表示成功，非0表示失败
 */
int power_set_battery_monitor(power_handle_t handle, bool enable, uint32_t interval_ms);

/**
 * @brief 启用/禁用自动睡眠
 * 
 * @param handle 电源设备句柄
 * @param enable 是否启用
 * @param timeout_ms 超时时间（毫秒）
 * @return int 0表示成功，非0表示失败
 */
int power_set_auto_sleep(power_handle_t handle, bool enable, uint32_t timeout_ms);

/**
 * @brief 重置自动睡眠计时器
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_reset_auto_sleep_timer(power_handle_t handle);

/**
 * @brief 执行系统复位
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_system_reset(power_handle_t handle);

/**
 * @brief 执行MCU复位
 * 
 * @param handle 电源设备句柄
 * @return int 该函数执行成功后不会返回
 */
int power_mcu_reset(power_handle_t handle);

#endif /* POWER_API_H */
