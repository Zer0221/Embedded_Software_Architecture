/**
 * @file stm32_power.c
 * @brief STM32平台电源管理实现
 *
 * 该文件实现了STM32平台的电源管理功能，提供了低功耗模式控制、电源状态监控和唤醒源管理功�?
 */

#include "base/power_api.h"
#include "stm32_platform.h"
#include "common/error_api.h"
#include <string.h>

#if (CURRENT_RTOS != RTOS_NONE)
#include "common/rtos_api.h"
#endif

/* 最大回调函数数�?*/
#define MAX_POWER_CALLBACKS  5

/* STM32电源设备结构�?*/
typedef struct {
    power_config_t config;             /**< 电源配置 */
    power_mode_t current_mode;         /**< 当前电源模式 */
    uint32_t enabled_wakeup_sources;   /**< 使能的唤醒源 */
    uint32_t active_wakeup_sources;    /**< 激活的唤醒�?*/
    bool initialized;                  /**< 初始化标�?*/
    bool auto_sleep_enabled;           /**< 自动睡眠使能标志 */
    uint32_t auto_sleep_timeout_ms;    /**< 自动睡眠超时时间 */
    uint32_t last_activity_time_ms;    /**< 最后活动时�?*/
    
    /* 回调函数 */
    struct {
        power_callback_t callback;     /**< 回调函数 */
        void *user_data;               /**< 用户数据 */
        bool used;                     /**< 使用标志 */
    } callbacks[MAX_POWER_CALLBACKS];
    
#if (CURRENT_RTOS != RTOS_NONE)
    rtos_timer_t auto_sleep_timer;     /**< 自动睡眠定时�?*/
    rtos_mutex_t mutex;                /**< 互斥�?*/
#endif
} stm32_power_t;

/* 全局变量 */
static stm32_power_t g_stm32_power;
static bool g_is_power_initialized = false;

/* 电池相关参数 */
static const float g_battery_voltage_max = 4.2f;  /* 最大电�?*/
static const float g_battery_voltage_min = 3.0f;  /* 最小电�?*/

/* 电池状态和健康状�?*/
static battery_status_t g_battery_status = BATTERY_STATUS_UNKNOWN;
static battery_health_t g_battery_health = BATTERY_HEALTH_UNKNOWN;
static charge_status_t g_charge_status = CHARGE_STATUS_UNKNOWN;
static float g_battery_temperature = 25.0f;  /* 默认温度（摄氏度�?*/

/* 电池监控任务相关 */
static bool g_battery_monitor_enabled = false;
static uint32_t g_battery_monitor_interval_ms = 10000;  /* 默认10�?*/

#if (CURRENT_RTOS != RTOS_NONE)
static rtos_task_t g_battery_monitor_task = NULL;
#endif

/**
 * @brief 自动睡眠定时器回调函�?
 */
#if (CURRENT_RTOS != RTOS_NONE)
static void auto_sleep_timer_callback(rtos_timer_t timer, void *arg)
{
    stm32_power_t *power = (stm32_power_t *)arg;
    uint32_t current_time_ms;
    
    if (power == NULL || !power->initialized || !power->auto_sleep_enabled) {
        return;
    }
    
    /* 获取当前时间 */
    current_time_ms = platform_get_time_ms();
    
    /* 检查是否超�?*/
    if (current_time_ms - power->last_activity_time_ms >= power->auto_sleep_timeout_ms) {
        /* 进入睡眠模式 */
        power_set_mode((power_handle_t)power, POWER_MODE_SLEEP, 0);
    }
}
#endif

/**
 * @brief 调用电源回调函数
 * 
 * @param power 电源设备
 * @param mode 电源模式
 * @param source 唤醒�?
 */
static void call_power_callbacks(stm32_power_t *power, power_mode_t mode, wakeup_source_t source)
{
    int i;
    
    for (i = 0; i < MAX_POWER_CALLBACKS; i++) {
        if (power->callbacks[i].used && power->callbacks[i].callback != NULL) {
            power->callbacks[i].callback(mode, source, power->callbacks[i].user_data);
        }
    }
}

/**
 * @brief 获取STM32唤醒�?
 * 
 * @return uint32_t 唤醒�?
 */
static uint32_t get_stm32_wakeup_source(void)
{
    uint32_t source = WAKEUP_SOURCE_NONE;
    
    /* 检查唤醒源 */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU) != RESET) {
        source |= WAKEUP_SOURCE_PIN;
    }
    
    if (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_ALRAF) != RESET) {
        source |= WAKEUP_SOURCE_RTC_ALARM;
    }
    
    if (__HAL_RTC_TIMESTAMP_GET_FLAG(&hrtc, RTC_FLAG_TSF) != RESET) {
        source |= WAKEUP_SOURCE_RTC_TIMESTAMP;
    }
    
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) {
        source |= WAKEUP_SOURCE_WATCHDOG;
    }
    
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET) {
        source |= WAKEUP_SOURCE_RESET;
    }
    
    return source;
}

/**
 * @brief 重置STM32唤醒标志
 * 
 * @param sources 唤醒�?
 */
static void reset_stm32_wakeup_flags(uint32_t sources)
{
    if (sources & WAKEUP_SOURCE_PIN) {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    }
    
    if (sources & WAKEUP_SOURCE_RTC_ALARM) {
        __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    }
    
    if (sources & WAKEUP_SOURCE_RTC_TIMESTAMP) {
        __HAL_RTC_TIMESTAMP_CLEAR_FLAG(&hrtc, RTC_FLAG_TSF);
    }
    
    if (sources & WAKEUP_SOURCE_WATCHDOG) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
    
    if (sources & WAKEUP_SOURCE_RESET) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
}

/**
 * @brief 配置STM32唤醒�?
 * 
 * @param sources 唤醒�?
 * @param enable 是否使能
 * @return int 0表示成功，非0表示失败
 */
static int config_stm32_wakeup_sources(uint32_t sources, bool enable)
{
    if (sources & WAKEUP_SOURCE_PIN) {
        if (enable) {
            HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
        } else {
            HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
        }
    }
    
    /* RTC闹钟唤醒 */
    if (sources & WAKEUP_SOURCE_RTC_ALARM) {
        if (enable) {
            __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);
        } else {
            __HAL_RTC_ALARM_DISABLE_IT(&hrtc, RTC_IT_ALRA);
        }
    }
    
    /* RTC时间戳唤�?*/
    if (sources & WAKEUP_SOURCE_RTC_TIMESTAMP) {
        if (enable) {
            __HAL_RTC_TIMESTAMP_ENABLE_IT(&hrtc, RTC_IT_TS);
        } else {
            __HAL_RTC_TIMESTAMP_DISABLE_IT(&hrtc, RTC_IT_TS);
        }
    }
    
    return 0;
}

/**
 * @brief 初始化电源管�?
 * 
 * @param config 电源配置参数
 * @param handle 电源设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int power_init(const power_config_t *config, power_handle_t *handle)
{
    if (config == NULL || handle == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 检查是否已初始�?*/
    if (g_is_power_initialized) {
        *handle = &g_stm32_power;
        return 0;
    }
    
    /* 初始化结构体 */
    memset(&g_stm32_power, 0, sizeof(g_stm32_power));
    
    /* 复制配置 */
    memcpy(&g_stm32_power.config, config, sizeof(power_config_t));
    
    /* 初始化成员变�?*/
    g_stm32_power.current_mode = POWER_MODE_ACTIVE;
    g_stm32_power.enabled_wakeup_sources = config->wakeup_sources;
    g_stm32_power.active_wakeup_sources = WAKEUP_SOURCE_NONE;
    g_stm32_power.auto_sleep_enabled = config->enable_auto_sleep;
    g_stm32_power.auto_sleep_timeout_ms = config->auto_sleep_timeout_ms;
    g_stm32_power.last_activity_time_ms = platform_get_time_ms();
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 创建互斥�?*/
    if (rtos_mutex_create(&g_stm32_power.mutex) != 0) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 创建自动睡眠定时�?*/
    if (g_stm32_power.auto_sleep_enabled) {
        if (rtos_timer_create(&g_stm32_power.auto_sleep_timer, "AutoSleepTimer", 
                            1000, true, 0, auto_sleep_timer_callback) != 0) {
            rtos_mutex_delete(g_stm32_power.mutex);
            REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
            return -1;
        }
        
        /* 启动定时�?*/
        rtos_timer_start(g_stm32_power.auto_sleep_timer);
    }
#endif
    
    /* 配置唤醒�?*/
    config_stm32_wakeup_sources(g_stm32_power.enabled_wakeup_sources, true);
    
    /* 标记为已初始�?*/
    g_stm32_power.initialized = true;
    g_is_power_initialized = true;
    
    /* 返回句柄 */
    *handle = &g_stm32_power;
    
    return 0;
}

/**
 * @brief 去初始化电源管理
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_deinit(power_handle_t handle)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 停止自动睡眠定时�?*/
    if (power->auto_sleep_enabled && power->auto_sleep_timer != NULL) {
        rtos_timer_stop(power->auto_sleep_timer);
        rtos_timer_delete(power->auto_sleep_timer);
    }
    
    /* 删除互斥�?*/
    rtos_mutex_delete(power->mutex);
#endif
    
    /* 禁用唤醒�?*/
    config_stm32_wakeup_sources(power->enabled_wakeup_sources, false);
    
    /* 标记为未初始�?*/
    power->initialized = false;
    g_is_power_initialized = false;
    
    return 0;
}

/**
 * @brief 设置电源模式
 * 
 * @param handle 电源设备句柄
 * @param mode 电源模式
 * @param timeout_ms 超时时间（毫秒）�?表示永久生效，非0表示超时后自动唤�?
 * @return int 0表示成功，非0表示失败
 */
int power_set_mode(power_handle_t handle, power_mode_t mode, uint32_t timeout_ms)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || mode >= POWER_MODE_MAX) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 保存当前模式 */
    power->current_mode = mode;
    
    /* 调用回调函数 */
    call_power_callbacks(power, mode, WAKEUP_SOURCE_NONE);
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    /* 根据模式设置低功�?*/
    switch (mode) {
        case POWER_MODE_ACTIVE:
            /* 无需操作 */
            break;
            
        case POWER_MODE_SLEEP:
            /* 如果设置了超时，配置RTC闹钟 */
            if (timeout_ms > 0) {
                /* 配置RTC闹钟逻辑 */
                /* 注意：这里只是示例，实际需要根据STM32的HAL库函数来实现 */
            }
            
            /* 进入睡眠模式 */
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;
            
        case POWER_MODE_DEEP_SLEEP:
            /* 如果设置了超时，配置RTC闹钟 */
            if (timeout_ms > 0) {
                /* 配置RTC闹钟逻辑 */
            }
            
            /* 进入停止模式 */
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            break;
            
        case POWER_MODE_STANDBY:
            /* 如果设置了超时，配置RTC闹钟 */
            if (timeout_ms > 0) {
                /* 配置RTC闹钟逻辑 */
            }
            
            /* 进入待机模式 */
            HAL_PWR_EnterSTANDBYMode();
            break;
            
        case POWER_MODE_SHUTDOWN:
            /* STM32通常不支持真正的关机模式，使用待机模式代�?*/
            HAL_PWR_EnterSTANDBYMode();
            break;
            
        default:
            break;
    }
    
    /* 如果代码执行到这里，说明设备已唤�?*/
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 设置为活动模�?*/
    power->current_mode = POWER_MODE_ACTIVE;
    
    /* 获取唤醒�?*/
    power->active_wakeup_sources = get_stm32_wakeup_source();
    
    /* 重置唤醒标志 */
    reset_stm32_wakeup_flags(power->active_wakeup_sources);
    
    /* 重置活动时间 */
    power->last_activity_time_ms = platform_get_time_ms();
    
    /* 调用回调函数 */
    call_power_callbacks(power, POWER_MODE_ACTIVE, (wakeup_source_t)power->active_wakeup_sources);
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 获取当前电源模式
 * 
 * @param handle 电源设备句柄
 * @param mode 电源模式指针
 * @return int 0表示成功，非0表示失败
 */
int power_get_mode(power_handle_t handle, power_mode_t *mode)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || mode == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 获取当前模式 */
    *mode = power->current_mode;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 获取唤醒�?
 * 
 * @param handle 电源设备句柄
 * @param source 唤醒源指�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_wakeup_source(power_handle_t handle, uint32_t *source)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || source == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 获取活动唤醒�?*/
    *source = power->active_wakeup_sources;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 配置唤醒�?
 * 
 * @param handle 电源设备句柄
 * @param sources 唤醒源，多个唤醒源可通过或运算组�?
 * @param enable 是否使能
 * @return int 0表示成功，非0表示失败
 */
int power_config_wakeup_source(power_handle_t handle, uint32_t sources, bool enable)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 更新使能的唤醒源 */
    if (enable) {
        power->enabled_wakeup_sources |= sources;
    } else {
        power->enabled_wakeup_sources &= ~sources;
    }
    
    /* 配置STM32唤醒�?*/
    config_stm32_wakeup_sources(sources, enable);
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 重置唤醒源状�?
 * 
 * @param handle 电源设备句柄
 * @param sources 唤醒源，多个唤醒源可通过或运算组�?
 * @return int 0表示成功，非0表示失败
 */
int power_reset_wakeup_source(power_handle_t handle, uint32_t sources)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 清除指定的活动唤醒源 */
    power->active_wakeup_sources &= ~sources;
    
    /* 重置STM32唤醒标志 */
    reset_stm32_wakeup_flags(sources);
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 注册电源回调函数
 * 
 * @param handle 电源设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int power_register_callback(power_handle_t handle, power_callback_t callback, void *user_data)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    int i;
    int ret = -1;
    
    if (power == NULL || !power->initialized || callback == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 查找空闲槽位 */
    for (i = 0; i < MAX_POWER_CALLBACKS; i++) {
        if (!power->callbacks[i].used) {
            power->callbacks[i].callback = callback;
            power->callbacks[i].user_data = user_data;
            power->callbacks[i].used = true;
            ret = 0;
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    if (ret != 0) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
    }
    
    return ret;
}

/**
 * @brief 取消注册电源回调函数
 * 
 * @param handle 电源设备句柄
 * @param callback 回调函数
 * @return int 0表示成功，非0表示失败
 */
int power_unregister_callback(power_handle_t handle, power_callback_t callback)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    int i;
    int ret = -1;
    
    if (power == NULL || !power->initialized || callback == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 查找回调函数 */
    for (i = 0; i < MAX_POWER_CALLBACKS; i++) {
        if (power->callbacks[i].used && power->callbacks[i].callback == callback) {
            power->callbacks[i].used = false;
            ret = 0;
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    if (ret != 0) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_NOT_FOUND | ERROR_SEVERITY_WARNING);
    }
    
    return ret;
}

/**
 * @brief 获取电池电压
 * 
 * @param handle 电源设备句柄
 * @param voltage 电压值指针（伏特�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_voltage(power_handle_t handle, float *voltage)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || voltage == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 实际应用中，这里应该读取ADC获取电池电压 */
    /* 这里只是示例，返回一个模拟�?*/
    *voltage = 3.8f;
    
    return 0;
}

/**
 * @brief 获取电池电量百分�?
 * 
 * @param handle 电源设备句柄
 * @param percentage 电量百分比指针（0-100�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_percentage(power_handle_t handle, uint8_t *percentage)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    float voltage;
    
    if (power == NULL || !power->initialized || percentage == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 获取电池电压 */
    if (power_get_battery_voltage(handle, &voltage) != 0) {
        return -1;
    }
    
    /* 根据电池类型计算电量百分�?*/
    switch (power->config.battery_type) {
        case BATTERY_TYPE_LIPO:
        case BATTERY_TYPE_LIION:
            /* 线性映射电压到百分�?*/
            *percentage = (uint8_t)((voltage - g_battery_voltage_min) / 
                         (g_battery_voltage_max - g_battery_voltage_min) * 100.0f);
            break;
            
        case BATTERY_TYPE_ALKALINE:
        case BATTERY_TYPE_NIMH:
        case BATTERY_TYPE_CUSTOM:
            /* 简化处理，实际应该使用查表法或特定曲线 */
            *percentage = (uint8_t)((voltage - g_battery_voltage_min) / 
                         (g_battery_voltage_max - g_battery_voltage_min) * 100.0f);
            break;
            
        case BATTERY_TYPE_NONE:
        default:
            *percentage = 0;
            break;
    }
    
    /* 确保百分比在有效范围�?*/
    if (*percentage > 100) {
        *percentage = 100;
    }
    
    return 0;
}

/**
 * @brief 获取电池状�?
 * 
 * @param handle 电源设备句柄
 * @param status 电池状态指�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_status(power_handle_t handle, battery_status_t *status)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || status == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 如果启用了电池监控，直接返回缓存的状�?*/
    if (g_battery_monitor_enabled) {
        *status = g_battery_status;
    } else {
        /* 否则更新电池状�?*/
        update_battery_status(power);
        *status = g_battery_status;
    }
    
    return 0;
}

/**
 * @brief 获取电池健康状�?
 * 
 * @param handle 电源设备句柄
 * @param health 电池健康状态指�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_battery_health(power_handle_t handle, battery_health_t *health)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || health == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 如果启用了电池监控，直接返回缓存的状�?*/
    if (g_battery_monitor_enabled) {
        *health = g_battery_health;
    } else {
        /* 否则更新电池状�?*/
        update_battery_status(power);
        *health = g_battery_health;
    }
    
    return 0;
}

/**
 * @brief 获取充电状�?
 * 
 * @param handle 电源设备句柄
 * @param status 充电状态指�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_charge_status(power_handle_t handle, charge_status_t *status)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || status == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 实际应用中，这里应该检测充电芯片的状态引�?*/
    /* 这里只是示例，返回一个模拟�?*/
    *status = g_charge_status;
    
    return 0;
}

/**
 * @brief 获取当前设备温度
 * 
 * @param handle 电源设备句柄
 * @param temperature 温度指针（摄氏度�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_temperature(power_handle_t handle, float *temperature)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized || temperature == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 实际应用中，这里应该读取温度传感�?*/
    /* 这里只是示例，返回一个模拟�?*/
    *temperature = g_battery_temperature;
    
    return 0;
}

/**
 * @brief 启用/禁用自动睡眠
 * 
 * @param handle 电源设备句柄
 * @param enable 是否启用
 * @param timeout_ms 超时时间（毫秒）
 * @return int 0表示成功，非0表示失败
 */
int power_set_auto_sleep(power_handle_t handle, bool enable, uint32_t timeout_ms)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
    
    /* 更新自动睡眠设置 */
    power->auto_sleep_enabled = enable;
    power->auto_sleep_timeout_ms = timeout_ms;
    
    /* 更新定时�?*/
    if (power->auto_sleep_timer != NULL) {
        if (enable) {
            /* 重置定时�?*/
            rtos_timer_reset(power->auto_sleep_timer);
        } else {
            /* 停止定时�?*/
            rtos_timer_stop(power->auto_sleep_timer);
        }
    } else if (enable) {
        /* 创建定时�?*/
        if (rtos_timer_create(&power->auto_sleep_timer, "AutoSleepTimer", 
                            1000, true, 0, auto_sleep_timer_callback) != 0) {
            power->auto_sleep_enabled = false;
            rtos_mutex_unlock(power->mutex);
            REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
            return -1;
        }
        
        /* 启动定时�?*/
        rtos_timer_start(power->auto_sleep_timer);
    }
    
    /* 重置活动时间 */
    power->last_activity_time_ms = platform_get_time_ms();
    
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#else
    /* 裸机模式下简化处�?*/
    power->auto_sleep_enabled = enable;
    power->auto_sleep_timeout_ms = timeout_ms;
    power->last_activity_time_ms = platform_get_time_ms();
#endif
    
    return 0;
}

/**
 * @brief 重置自动睡眠计时�?
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_reset_auto_sleep_timer(power_handle_t handle)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 更新活动时间 */
    power->last_activity_time_ms = platform_get_time_ms();
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 执行系统复位
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_system_reset(power_handle_t handle)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 执行系统复位 */
    NVIC_SystemReset();
    
    /* 不会执行到这�?*/
    return 0;
}

/**
 * @brief 执行MCU复位
 * 
 * @param handle 电源设备句柄
 * @return int 该函数执行成功后不会返回
 */
int power_mcu_reset(power_handle_t handle)
{
    /* 直接调用系统复位 */
    return power_system_reset(handle);
}

/**
 * @brief 更新电池状�?
 * 
 * @param power 电源设备
 */
static void update_battery_status(stm32_power_t *power)
{
    float voltage;
    uint8_t percentage;
    battery_status_t new_status;
    
    /* 获取电池电压 */
    if (power_get_battery_voltage((power_handle_t)power, &voltage) != 0) {
        return;
    }
    
    /* 获取电池电量百分�?*/
    if (power_get_battery_percentage((power_handle_t)power, &percentage) != 0) {
        return;
    }
    
    /* 更新电池状�?*/
    if (percentage < 5) {
        new_status = BATTERY_STATUS_DEPLETED;
    } else if (percentage < 15) {
        new_status = BATTERY_STATUS_CRITICAL;
    } else if (percentage < 30) {
        new_status = BATTERY_STATUS_LOW;
    } else {
        new_status = BATTERY_STATUS_NORMAL;
    }
    
    /* 如果状态变化，调用回调函数 */
    if (new_status != g_battery_status) {
        g_battery_status = new_status;
        
        /* 如果电池状态发生重大变化，调用回调函数 */
        if (new_status == BATTERY_STATUS_CRITICAL || new_status == BATTERY_STATUS_DEPLETED) {
            call_power_callbacks(power, power->current_mode, WAKEUP_SOURCE_NONE);
        }
    }
    
    /* 更新电池健康状�?*/
    /* 实际应用中，应该根据充放电次数、温度等因素综合评估 */
    /* 这里只是简单示�?*/
    if (voltage < g_battery_voltage_min + 0.2f) {
        g_battery_health = BATTERY_HEALTH_POOR;
    } else {
        g_battery_health = BATTERY_HEALTH_GOOD;
    }
}

/**
 * @brief 电池监控任务
 */
#if (CURRENT_RTOS != RTOS_NONE)
static void battery_monitor_task(void *arg)
{
    stm32_power_t *power = (stm32_power_t *)arg;
    
    while (g_battery_monitor_enabled) {
        /* 更新电池状�?*/
        update_battery_status(power);
        
        /* 延时指定时间 */
        rtos_task_delay(g_battery_monitor_interval_ms);
    }
    
    /* 任务结束 */
    rtos_task_delete(NULL);
}
#endif

/**
 * @brief 启动电池监控
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_start_battery_monitor(power_handle_t handle)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
    
    /* 启用电池监控 */
    g_battery_monitor_enabled = true;
    
    /* 创建电池监控任务 */
    if (g_battery_monitor_task == NULL) {
        if (rtos_task_create(&g_battery_monitor_task, "BatteryMonitor", 
                            2048, power, 1, NULL) != 0) {
            g_battery_monitor_enabled = false;
            rtos_mutex_unlock(power->mutex);
            REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
            return -1;
        }
    }
    
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 停止电池监控
 * 
 * @param handle 电源设备句柄
 * @return int 0表示成功，非0表示失败
 */
int power_stop_battery_monitor(power_handle_t handle)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
    
    /* 禁用电池监控 */
    g_battery_monitor_enabled = false;
    
    /* 删除电池监控任务 */
    if (g_battery_monitor_task != NULL) {
        rtos_task_delete(g_battery_monitor_task);
        g_battery_monitor_task = NULL;
    }
    
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    return 0;
}

/**
 * @brief 设置电池监控间隔
 * 
 * @param handle 电源设备句柄
 * @param interval_ms 间隔时间（毫秒）
 * @return int 0表示成功，非0表示失败
 */
int power_set_battery_monitor_interval(power_handle_t handle, uint32_t interval_ms)
{
    stm32_power_t *power = (stm32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 更新间隔 */
    g_battery_monitor_interval_ms = interval_ms;
    
    return 0;
}

