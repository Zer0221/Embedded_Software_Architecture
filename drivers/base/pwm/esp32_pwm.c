/**
 * @file esp32_pwm.c
 * @brief ESP32平台PWM驱动实现
 *
 * 该文件实现了ESP32平台的PWM驱动接口，基于ESP32的LEDC功能
 */

#include "base/pwm_api.h"
#include "common/error_api.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

/* 日志标签 */
static const char *TAG = "ESP32_PWM";

/* ESP32 LEDC通道映射 */
static const ledc_channel_t LEDC_CHANNEL_MAP[] = {
    LEDC_CHANNEL_0,  /* PWM_CHANNEL_0 */
    LEDC_CHANNEL_1,  /* PWM_CHANNEL_1 */
    LEDC_CHANNEL_2,  /* PWM_CHANNEL_2 */
    LEDC_CHANNEL_3,  /* PWM_CHANNEL_3 */
    LEDC_CHANNEL_4,  /* PWM_CHANNEL_4 */
    LEDC_CHANNEL_5,  /* PWM_CHANNEL_5 */
    LEDC_CHANNEL_6,  /* PWM_CHANNEL_6 */
    LEDC_CHANNEL_7,  /* PWM_CHANNEL_7 */
};

/* ESP32 LEDC定时器映�?*/
static const ledc_timer_t LEDC_TIMER_MAP[] = {
    LEDC_TIMER_0,  /* 用于通道0�? */
    LEDC_TIMER_0,  /* 用于通道0�? */
    LEDC_TIMER_1,  /* 用于通道2�? */
    LEDC_TIMER_1,  /* 用于通道2�? */
    LEDC_TIMER_2,  /* 用于通道4�? */
    LEDC_TIMER_2,  /* 用于通道4�? */
    LEDC_TIMER_3,  /* 用于通道6�? */
    LEDC_TIMER_3,  /* 用于通道6�? */
};

/* PWM设备句柄结构�?*/
typedef struct {
    pwm_config_t config;       /* PWM配置参数 */
    bool initialized;          /* 初始化标�?*/
    bool running;              /* 运行标志 */
    uint32_t duty_value;       /* 占空比计数�?*/
    TimerHandle_t pulse_timer; /* 脉冲计数定时�?*/
    uint32_t pulse_count;      /* 剩余脉冲数量 */
    pwm_callback_t callbacks[3]; /* 回调函数数组 */
    void *user_data[3];          /* 用户数据数组 */
} esp32_pwm_handle_t;

/* PWM设备句柄数组 */
static esp32_pwm_handle_t g_pwm_handles[PWM_CHANNEL_MAX];

/* 转换为ESP32 PWM占空�?*/
static uint32_t convert_duty_cycle(float duty_cycle, uint32_t resolution_bits)
{
    uint32_t max_duty = (1 << resolution_bits) - 1;
    
    /* 限制占空比在有效范围�?*/
    if (duty_cycle < 0.0f) {
        duty_cycle = 0.0f;
    } else if (duty_cycle > 1.0f) {
        duty_cycle = 1.0f;
    }
    
    return (uint32_t)(duty_cycle * max_duty);
}

/* 脉冲定时器回�?*/
static void pulse_timer_callback(TimerHandle_t timer)
{
    esp32_pwm_handle_t *esp32_handle = pvTimerGetTimerID(timer);
    
    /* 减少脉冲计数 */
    if (esp32_handle->pulse_count > 0) {
        esp32_handle->pulse_count--;
        
        /* 如果脉冲完成 */
        if (esp32_handle->pulse_count == 0) {
            /* 停止PWM输出 */
            ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_MAP[esp32_handle->config.channel], 0);
            esp32_handle->running = false;
            
            /* 调用脉冲完成回调 */
            if (esp32_handle->callbacks[PWM_EVENT_PULSE_FINISHED]) {
                esp32_handle->callbacks[PWM_EVENT_PULSE_FINISHED](
                    esp32_handle->user_data[PWM_EVENT_PULSE_FINISHED]);
            }
        }
    }
}

/**
 * @brief 初始化PWM
 * 
 * @param config PWM配置参数
 * @param handle PWM设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int pwm_init(const pwm_config_t *config, pwm_handle_t *handle)
{
    esp32_pwm_handle_t *esp32_handle;
    ledc_timer_config_t timer_config = {0};
    ledc_channel_config_t channel_config = {0};
    esp_err_t ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= PWM_CHANNEL_MAX) {
        ESP_LOGE(TAG, "Invalid PWM channel: %d", config->channel);
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已初始化 */
    esp32_handle = &g_pwm_handles[config->channel];
    if (esp32_handle->initialized) {
        return ERROR_BUSY;
    }
    
    /* 初始化句�?*/
    memset(esp32_handle, 0, sizeof(esp32_pwm_handle_t));
    esp32_handle->config = *config;
    
    /* 配置LEDC定时�?*/
    timer_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_config.duty_resolution = LEDC_TIMER_13_BIT; /* 13位分辨率 */
    timer_config.timer_num = LEDC_TIMER_MAP[config->channel];
    timer_config.freq_hz = config->frequency;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    
    /* 计算占空比�?*/
    esp32_handle->duty_value = convert_duty_cycle(config->duty_cycle, 13);
    
    /* 配置LEDC定时�?*/
    ret = ledc_timer_config(&timer_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 配置LEDC通道 */
    channel_config.gpio_num = -1; /* 初始化时不绑定GPIO，由用户在使用时配置 */
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.channel = LEDC_CHANNEL_MAP[config->channel];
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.timer_sel = LEDC_TIMER_MAP[config->channel];
    channel_config.duty = esp32_handle->duty_value;
    channel_config.hpoint = 0;
    
    /* 配置LEDC通道 */
    ret = ledc_channel_config(&channel_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 创建脉冲定时�?*/
    esp32_handle->pulse_timer = xTimerCreate(
        "pwm_pulse_timer",
        pdMS_TO_TICKS(1000), /* 将根据频率更�?*/
        pdTRUE,
        esp32_handle,
        pulse_timer_callback
    );
    
    if (esp32_handle->pulse_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create pulse timer");
        return ERROR_RESOURCE;
    }
    
    /* 标记为已初始�?*/
    esp32_handle->initialized = true;
    *handle = (pwm_handle_t)esp32_handle;
    
    ESP_LOGI(TAG, "PWM initialized: channel=%d, frequency=%lu, duty_cycle=%.2f",
           config->channel, config->frequency, config->duty_cycle);
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化PWM
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_deinit(pwm_handle_t handle)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 如果PWM正在运行，停�?*/
    if (esp32_handle->running) {
        pwm_stop(handle);
    }
    
    /* 删除脉冲定时�?*/
    if (esp32_handle->pulse_timer != NULL) {
        xTimerDelete(esp32_handle->pulse_timer, pdMS_TO_TICKS(100));
        esp32_handle->pulse_timer = NULL;
    }
    
    /* 重置句柄 */
    memset(esp32_handle, 0, sizeof(esp32_pwm_handle_t));
    
    return DRIVER_OK;
}

/**
 * @brief 启动PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_start(pwm_handle_t handle)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查是否已运行 */
    if (esp32_handle->running) {
        return DRIVER_OK;
    }
    
    /* 启动PWM输出 */
    ret = ledc_update_duty(LEDC_HIGH_SPEED_MODE, 
                        LEDC_CHANNEL_MAP[esp32_handle->config.channel]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC update duty failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 标记为运�?*/
    esp32_handle->running = true;
    
    /* 调用周期开始回�?*/
    if (esp32_handle->callbacks[PWM_EVENT_PERIOD_ELAPSED]) {
        esp32_handle->callbacks[PWM_EVENT_PERIOD_ELAPSED](
            esp32_handle->user_data[PWM_EVENT_PERIOD_ELAPSED]);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 停止PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_stop(pwm_handle_t handle)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查是否已运行 */
    if (!esp32_handle->running) {
        return DRIVER_OK;
    }
    
    /* 停止脉冲定时�?*/
    if (esp32_handle->pulse_timer != NULL) {
        xTimerStop(esp32_handle->pulse_timer, pdMS_TO_TICKS(100));
    }
    
    /* 停止PWM输出 */
    ret = ledc_stop(LEDC_HIGH_SPEED_MODE, 
                 LEDC_CHANNEL_MAP[esp32_handle->config.channel], 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC stop failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 标记为停�?*/
    esp32_handle->running = false;
    
    return DRIVER_OK;
}

/**
 * @brief 设置PWM频率
 * 
 * @param handle PWM设备句柄
 * @param frequency 频率(Hz)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_frequency(pwm_handle_t handle, uint32_t frequency)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 更新频率 */
    ret = ledc_set_freq(LEDC_HIGH_SPEED_MODE, 
                      LEDC_TIMER_MAP[esp32_handle->config.channel], 
                      frequency);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC set frequency failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 更新配置 */
    esp32_handle->config.frequency = frequency;
    
    /* 如果脉冲定时器在运行，更新周�?*/
    if (esp32_handle->pulse_count > 0) {
        xTimerChangePeriod(esp32_handle->pulse_timer, 
                          pdMS_TO_TICKS(1000 / frequency), 
                          pdMS_TO_TICKS(100));
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置PWM占空�?
 * 
 * @param handle PWM设备句柄
 * @param duty_cycle 占空�?0.0-1.0)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_duty_cycle(pwm_handle_t handle, float duty_cycle)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 计算占空比�?*/
    esp32_handle->duty_value = convert_duty_cycle(duty_cycle, 13);
    
    /* 更新占空�?*/
    ret = ledc_set_duty(LEDC_HIGH_SPEED_MODE, 
                      LEDC_CHANNEL_MAP[esp32_handle->config.channel], 
                      esp32_handle->duty_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC set duty failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 更新占空�?*/
    ret = ledc_update_duty(LEDC_HIGH_SPEED_MODE, 
                         LEDC_CHANNEL_MAP[esp32_handle->config.channel]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC update duty failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 更新配置 */
    esp32_handle->config.duty_cycle = duty_cycle;
    
    return DRIVER_OK;
}

/**
 * @brief 设置PWM死区时间
 * 
 * @param handle PWM设备句柄
 * @param dead_time 死区时间(ns)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_dead_time(pwm_handle_t handle, uint32_t dead_time)
{
    /* ESP32 LEDC不支持死区时间控�?*/
    return ERROR_NOT_SUPPORTED;
}

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
                         pwm_callback_t callback, void *user_data)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL || callback == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查事件类�?*/
    if (event >= 3) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 注册回调函数 */
    esp32_handle->callbacks[event] = callback;
    esp32_handle->user_data[event] = user_data;
    
    return DRIVER_OK;
}

/**
 * @brief 取消注册PWM事件回调函数
 * 
 * @param handle PWM设备句柄
 * @param event 事件类型
 * @return int 0表示成功，非0表示失败
 */
int pwm_unregister_callback(pwm_handle_t handle, pwm_event_t event)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查事件类�?*/
    if (event >= 3) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 取消注册回调函数 */
    esp32_handle->callbacks[event] = NULL;
    esp32_handle->user_data[event] = NULL;
    
    return DRIVER_OK;
}

/**
 * @brief 生成PWM脉冲
 * 
 * @param handle PWM设备句柄
 * @param pulse_count 脉冲数量
 * @return int 0表示成功，非0表示失败
 */
int pwm_generate_pulse(pwm_handle_t handle, uint32_t pulse_count)
{
    esp32_pwm_handle_t *esp32_handle = (esp32_pwm_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 如果脉冲数量�?，返回成�?*/
    if (pulse_count == 0) {
        return DRIVER_OK;
    }
    
    /* 设置脉冲数量 */
    esp32_handle->pulse_count = pulse_count;
    
    /* 配置定时器周�?*/
    xTimerChangePeriod(esp32_handle->pulse_timer, 
                      pdMS_TO_TICKS(1000 / esp32_handle->config.frequency), 
                      pdMS_TO_TICKS(100));
    
    /* 启动PWM输出 */
    pwm_start(handle);
    
    /* 启动脉冲定时�?*/
    xTimerStart(esp32_handle->pulse_timer, pdMS_TO_TICKS(100));
    
    return DRIVER_OK;
}

