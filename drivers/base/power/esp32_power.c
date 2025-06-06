/**
 * @file esp32_power.c
 * @brief ESP32平台电源管理实现
 *
 * 该文件实现了ESP32平台的电源管理功能，提供了低功耗模式控制、电源状态监控和电池管理功能
 */

#include "base/power_api.h"
#include "common/error_api.h"
#include "esp_platform.h"
#include "esp_sleep.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include <string.h>

#if (CURRENT_RTOS != RTOS_NONE)
#include "common/rtos_api.h"
#endif

/* 最大回调函数数�?*/
#define MAX_POWER_CALLBACKS  5

/* 电池ADC通道配置 */
#define BATTERY_ADC_CHANNEL      ADC1_CHANNEL_6  // 根据实际硬件调整
#define BATTERY_ADC_ATTEN        ADC_ATTEN_DB_11
#define BATTERY_ADC_WIDTH        ADC_WIDTH_BIT_12
#define BATTERY_ADC_SAMPLES      64

/* 电池分压电阻配置 */
#define BATTERY_DIVIDER_RATIO    2.0f  // 根据实际硬件分压比例调整

/* ESP32电源设备结构�?*/
typedef struct {
    power_config_t config;             /**< 电源配置 */
    power_mode_t current_mode;         /**< 当前电源模式 */
    uint32_t enabled_wakeup_sources;   /**< 使能的唤醒源 */
    uint32_t active_wakeup_sources;    /**< 激活的唤醒�?*/
    bool initialized;                  /**< 初始化标�?*/
    bool auto_sleep_enabled;           /**< 自动睡眠使能标志 */
    uint32_t auto_sleep_timeout_ms;    /**< 自动睡眠超时时间 */
    uint32_t last_activity_time_ms;    /**< 最后活动时�?*/
    
    /* 电池监控 */
    bool battery_monitor_enabled;      /**< 电池监控使能标志 */
    float battery_voltage;             /**< 电池电压 */
    uint8_t battery_percentage;        /**< 电池电量百分�?*/
    battery_status_t battery_status;   /**< 电池状�?*/
    battery_health_t battery_health;   /**< 电池健康状�?*/
    charge_status_t charge_status;     /**< 充电状�?*/
    power_state_t power_state;         /**< 电源状�?*/
    esp_adc_cal_characteristics_t adc_chars; /**< ADC特�?*/
    
    /* 回调函数 */
    struct {
        power_callback_t callback;     /**< 回调函数 */
        void *user_data;               /**< 用户数据 */
        bool used;                     /**< 使用标志 */
    } callbacks[MAX_POWER_CALLBACKS];
    
#if (CURRENT_RTOS != RTOS_NONE)
    rtos_timer_t auto_sleep_timer;     /**< 自动睡眠定时�?*/
    rtos_mutex_t mutex;                /**< 互斥�?*/
    rtos_task_t battery_monitor_task;  /**< 电池监控任务 */
#endif
} esp32_power_t;

/* 全局变量 */
static esp32_power_t g_esp32_power;
static bool g_is_power_initialized = false;

/* 电池相关参数 */
static const float g_battery_voltage_max = 4.2f;  /* 最大电�?*/
static const float g_battery_voltage_min = 3.0f;  /* 最小电�?*/

/**
 * @brief 自动睡眠定时器回调函�?
 */
#if (CURRENT_RTOS != RTOS_NONE)
static void auto_sleep_timer_callback(rtos_timer_t timer, void *arg)
{
    esp32_power_t *power = (esp32_power_t *)arg;
    uint32_t current_time_ms;
    
    if (power == NULL || !power->initialized || !power->auto_sleep_enabled) {
        return;
    }
    
    /* 获取当前时间 */
    current_time_ms = esp_timer_get_time() / 1000;
    
    /* 检查是否超�?*/
    if (current_time_ms - power->last_activity_time_ms >= power->auto_sleep_timeout_ms) {
        /* 进入睡眠模式 */
        power_set_mode((power_handle_t)power, POWER_MODE_SLEEP, 0);
    }
}
#endif

/**
 * @brief 电池监控任务
 */
#if (CURRENT_RTOS != RTOS_NONE)
static void battery_monitor_task(void *arg)
{
    esp32_power_t *power = (esp32_power_t *)arg;
    float voltage;
    uint8_t percentage;
    battery_status_t status;
    
    while (power->battery_monitor_enabled) {
        /* 测量电池电压 */
        if (power_get_battery_voltage((power_handle_t)power, &voltage) == 0) {
            power->battery_voltage = voltage;
            
            /* 计算电池电量 */
            if (power_get_battery_percentage((power_handle_t)power, &percentage) == 0) {
                power->battery_percentage = percentage;
                
                /* 更新电池状�?*/
                if (percentage < 5) {
                    status = BATTERY_STATUS_DEPLETED;
                } else if (percentage < 15) {
                    status = BATTERY_STATUS_CRITICAL;
                } else if (percentage < 30) {
                    status = BATTERY_STATUS_LOW;
                } else {
                    status = BATTERY_STATUS_NORMAL;
                }
                
                /* 如果状态发生变化，调用回调函数 */
                if (status != power->battery_status) {
                    power->battery_status = status;
                }
            }
        }
        
        /* 延时指定时间 */
        rtos_task_delay(power->config.battery_monitor_interval_ms);
    }
    
    /* 任务结束 */
    rtos_task_delete(NULL);
}
#endif

/**
 * @brief 调用电源回调函数
 * 
 * @param power 电源设备
 * @param mode 电源模式
 * @param source 唤醒�?
 */
static void call_power_callbacks(esp32_power_t *power, power_mode_t mode, wakeup_source_t source)
{
    int i;
    
    for (i = 0; i < MAX_POWER_CALLBACKS; i++) {
        if (power->callbacks[i].used && power->callbacks[i].callback != NULL) {
            power->callbacks[i].callback(mode, source, power->callbacks[i].user_data);
        }
    }
}

/**
 * @brief 获取ESP32唤醒�?
 * 
 * @return uint32_t 唤醒�?
 */
static uint32_t get_esp32_wakeup_source(void)
{
    uint32_t source = WAKEUP_SOURCE_NONE;
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:
            source |= WAKEUP_SOURCE_PIN;
            break;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            source |= WAKEUP_SOURCE_RTC_ALARM;
            break;
            
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            source |= WAKEUP_SOURCE_PIN;
            break;
            
        case ESP_SLEEP_WAKEUP_ULP:
            source |= WAKEUP_SOURCE_PIN;
            break;
            
        default:
            break;
    }
    
    return source;
}

/**
 * @brief 配置ESP32唤醒�?
 * 
 * @param sources 唤醒�?
 * @param enable 是否使能
 * @return int 0表示成功，非0表示失败
 */
static int config_esp32_wakeup_sources(uint32_t sources, bool enable)
{
    if (sources & WAKEUP_SOURCE_PIN) {
        /* 使用EXT0作为引脚唤醒�?GPIO0) */
        if (enable) {
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 1);
        } else {
            /* ESP32不提供禁用特定唤醒源的API，只能通过不调用使能函数来禁用 */
        }
    }
    
    /* RTC闹钟唤醒 */
    if (sources & WAKEUP_SOURCE_RTC_ALARM) {
        if (enable) {
            /* 默认设置�?0秒后唤醒，实际应用中应该根据需要调�?*/
            esp_sleep_enable_timer_wakeup(60 * 1000000);
        } else {
            /* ESP32不提供禁用特定唤醒源的API，只能通过不调用使能函数来禁用 */
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
    esp32_power_t *power = &g_esp32_power;
    
    if (config == NULL || handle == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    if (g_is_power_initialized) {
        *handle = (power_handle_t)power;
        return 0;
    }
    
    /* 初始化电源设�?*/
    memset(power, 0, sizeof(esp32_power_t));
    
    /* 保存配置 */
    memcpy(&power->config, config, sizeof(power_config_t));
    
    /* 初始化变�?*/
    power->current_mode = POWER_MODE_ACTIVE;
    power->enabled_wakeup_sources = config->wakeup_sources;
    power->active_wakeup_sources = WAKEUP_SOURCE_NONE;
    power->auto_sleep_enabled = config->enable_auto_sleep;
    power->auto_sleep_timeout_ms = config->auto_sleep_timeout_ms;
    power->last_activity_time_ms = esp_timer_get_time() / 1000;
    power->battery_monitor_enabled = config->enable_battery_monitor;
    power->battery_status = BATTERY_STATUS_UNKNOWN;
    power->battery_health = BATTERY_HEALTH_UNKNOWN;
    power->charge_status = CHARGE_STATUS_UNKNOWN;
    power->power_state = POWER_STATE_UNKNOWN;
    
    /* 初始化ADC用于电池电压测量 */
    if (config->enable_battery_monitor) {
        adc1_config_width(BATTERY_ADC_WIDTH);
        adc1_config_channel_atten(BATTERY_ADC_CHANNEL, BATTERY_ADC_ATTEN);
        esp_adc_cal_characterize(ADC_UNIT_1, BATTERY_ADC_ATTEN, BATTERY_ADC_WIDTH, 
                              1100, &power->adc_chars);
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 创建互斥�?*/
    if (rtos_mutex_create(&power->mutex) != 0) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 创建自动睡眠定时�?*/
    if (config->enable_auto_sleep) {
        if (rtos_timer_create(&power->auto_sleep_timer, "AutoSleepTimer", 
                           1000, true, 0, auto_sleep_timer_callback) != 0) {
            rtos_mutex_delete(power->mutex);
            REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
            return -1;
        }
        
        /* 启动定时�?*/
        rtos_timer_start(power->auto_sleep_timer);
    }
    
    /* 创建电池监控任务 */
    if (config->enable_battery_monitor) {
        if (rtos_task_create(&power->battery_monitor_task, "BatteryMonitor", 
                          battery_monitor_task, power, 
                          configMINIMAL_STACK_SIZE * 2, 
                          RTOS_TASK_PRIORITY_LOW) != 0) {
            if (config->enable_auto_sleep) {
                rtos_timer_delete(power->auto_sleep_timer);
            }
            rtos_mutex_delete(power->mutex);
            REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
            return -1;
        }
    }
#endif
    
    /* 配置唤醒�?*/
    config_esp32_wakeup_sources(config->wakeup_sources, true);
    
    /* 设置为初始化状�?*/
    power->initialized = true;
    g_is_power_initialized = true;
    
    /* 返回句柄 */
    *handle = (power_handle_t)power;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 停止并删除自动睡眠定时器 */
    if (power->auto_sleep_timer != NULL) {
        rtos_timer_stop(power->auto_sleep_timer);
        rtos_timer_delete(power->auto_sleep_timer);
        power->auto_sleep_timer = NULL;
    }
    
    /* 停止电池监控任务 */
    if (power->battery_monitor_task != NULL) {
        power->battery_monitor_enabled = false;
        /* 任务会自行退�?*/
    }
    
    /* 删除互斥�?*/
    if (power->mutex != NULL) {
        rtos_mutex_delete(power->mutex);
        power->mutex = NULL;
    }
#endif
    
    /* 设置为未初始化状�?*/
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
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
    
    /* 根据模式执行相应操作 */
    switch (mode) {
        case POWER_MODE_ACTIVE:
            /* 无需特殊操作 */
            break;
            
        case POWER_MODE_SLEEP:
            /* 调用回调函数 */
            call_power_callbacks(power, mode, WAKEUP_SOURCE_NONE);
            
            /* 设置唤醒定时�?*/
            if (timeout_ms > 0) {
                esp_sleep_enable_timer_wakeup(timeout_ms * 1000);
            }
            
            /* 进入轻度睡眠模式 */
            esp_light_sleep_start();
            
            /* 睡眠结束后，获取唤醒�?*/
            power->active_wakeup_sources = get_esp32_wakeup_source();
            
            /* 恢复到活动模�?*/
            power->current_mode = POWER_MODE_ACTIVE;
            
            /* 调用回调函数 */
            call_power_callbacks(power, POWER_MODE_ACTIVE, power->active_wakeup_sources);
            break;
            
        case POWER_MODE_DEEP_SLEEP:
            /* 调用回调函数 */
            call_power_callbacks(power, mode, WAKEUP_SOURCE_NONE);
            
            /* 设置唤醒定时�?*/
            if (timeout_ms > 0) {
                esp_sleep_enable_timer_wakeup(timeout_ms * 1000);
            }
            
            /* 进入深度睡眠模式 */
            esp_deep_sleep_start();
            /* 该函数不会返�?*/
            break;
            
        case POWER_MODE_STANDBY:
        case POWER_MODE_SHUTDOWN:
            /* ESP32不直接支持这些模式，使用深度睡眠代替 */
            call_power_callbacks(power, mode, WAKEUP_SOURCE_NONE);
            
            /* 设置唤醒定时�?*/
            if (timeout_ms > 0) {
                esp_sleep_enable_timer_wakeup(timeout_ms * 1000);
            }
            
            /* 进入深度睡眠模式 */
            esp_deep_sleep_start();
            /* 该函数不会返�?*/
            break;
            
        default:
            break;
    }
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized || mode == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    *mode = power->current_mode;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized || source == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    *source = power->active_wakeup_sources;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 配置唤醒�?*/
    if (config_esp32_wakeup_sources(sources, enable) != 0) {
#if (CURRENT_RTOS != RTOS_NONE)
        rtos_mutex_unlock(power->mutex);
#endif
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_OPERATION | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 更新使能的唤醒源 */
    if (enable) {
        power->enabled_wakeup_sources |= sources;
    } else {
        power->enabled_wakeup_sources &= ~sources;
    }
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 清除激活的唤醒�?*/
    power->active_wakeup_sources &= ~sources;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    int i;
    
    if (power == NULL || !power->initialized || callback == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥�?*/
    rtos_mutex_lock(power->mutex, UINT32_MAX);
#endif
    
    /* 查找空闲回调�?*/
    for (i = 0; i < MAX_POWER_CALLBACKS; i++) {
        if (!power->callbacks[i].used) {
            /* 注册回调函数 */
            power->callbacks[i].callback = callback;
            power->callbacks[i].user_data = user_data;
            power->callbacks[i].used = true;
            
#if (CURRENT_RTOS != RTOS_NONE)
            /* 释放互斥�?*/
            rtos_mutex_unlock(power->mutex);
#endif
            
            return 0;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    /* 没有空闲回调�?*/
    REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
    return -1;
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
    esp32_power_t *power = (esp32_power_t *)handle;
    int i;
    
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
            /* 取消注册回调函数 */
            power->callbacks[i].callback = NULL;
            power->callbacks[i].user_data = NULL;
            power->callbacks[i].used = false;
            
#if (CURRENT_RTOS != RTOS_NONE)
            /* 释放互斥�?*/
            rtos_mutex_unlock(power->mutex);
#endif
            
            return 0;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#endif
    
    /* 未找到回调函�?*/
    REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
    return -1;
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
    esp32_power_t *power = (esp32_power_t *)handle;
    uint32_t adc_reading = 0;
    int i;
    
    if (power == NULL || !power->initialized || voltage == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    if (!power->config.enable_battery_monitor) {
        /* 电池监控未启�?*/
        *voltage = 0.0f;
        return 0;
    }
    
    /* 采集多次ADC值并平均 */
    for (i = 0; i < BATTERY_ADC_SAMPLES; i++) {
        adc_reading += adc1_get_raw(BATTERY_ADC_CHANNEL);
    }
    adc_reading /= BATTERY_ADC_SAMPLES;
    
    /* 将ADC值转换为电压�?*/
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &power->adc_chars);
    
    /* 考虑分压电阻 */
    *voltage = (voltage_mv / 1000.0f) * BATTERY_DIVIDER_RATIO;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    float voltage;
    
    if (power == NULL || !power->initialized || percentage == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    if (!power->config.enable_battery_monitor) {
        /* 电池监控未启�?*/
        *percentage = 0;
        return 0;
    }
    
    /* 获取电池电压 */
    if (power_get_battery_voltage(handle, &voltage) != 0) {
        return -1;
    }
    
    /* 根据电池类型计算电量百分�?*/
    switch (power->config.battery_type) {
        case BATTERY_TYPE_LIPO:
        case BATTERY_TYPE_LIION:
        case BATTERY_TYPE_LIPO_HV:
            /* 线性映射电压到百分�?*/
            *percentage = (uint8_t)((voltage - g_battery_voltage_min) / 
                         (g_battery_voltage_max - g_battery_voltage_min) * 100.0f);
            break;
            
        case BATTERY_TYPE_ALKALINE:
        case BATTERY_TYPE_NIMH:
        case BATTERY_TYPE_LEAD_ACID:
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
 * @brief 获取电源状�?
 * 
 * @param handle 电源设备句柄
 * @param state 电源状态指�?
 * @return int 0表示成功，非0表示失败
 */
int power_get_state(power_handle_t handle, power_state_t *state)
{
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized || state == NULL) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 检测充电状�?*/
    /* 注意：实际应用中，这里应该检测充电芯片的状态引�?*/
    /* 这里仅为示例，根据实际硬件修�?*/
    *state = POWER_STATE_BATTERY;
    
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
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
    
    /* 释放互斥�?*/
    rtos_mutex_unlock(power->mutex);
#else
    power->auto_sleep_enabled = enable;
    power->auto_sleep_timeout_ms = timeout_ms;
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 更新最后活动时�?*/
    power->last_activity_time_ms = esp_timer_get_time() / 1000;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 重置定时�?*/
    if (power->auto_sleep_enabled && power->auto_sleep_timer != NULL) {
        rtos_timer_reset(power->auto_sleep_timer);
    }
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 执行系统复位 */
    esp_restart();
    
    /* 该函数不会返�?*/
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
    esp32_power_t *power = (esp32_power_t *)handle;
    
    if (power == NULL || !power->initialized) {
        REPORT_ERROR(ERROR_MODULE_POWER | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 执行MCU复位 */
    esp_restart();
    
    /* 该函数不会返�?*/
    return 0;
}

