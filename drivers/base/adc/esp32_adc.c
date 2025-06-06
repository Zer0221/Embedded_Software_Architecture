/**
 * @file esp32_adc.c
 * @brief ESP32平台ADC驱动实现
 *
 * 该文件实现了ESP32平台的ADC驱动接口
 */

#include "base/adc_api.h"
#include "common/error_api.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <string.h>

/* 日志标签 */
static const char *TAG = "ESP32_ADC";

/* ESP32 ADC通道映射 */
static const adc1_channel_t ADC1_CHANNEL_MAP[] = {
    ADC1_CHANNEL_0,  /* ADC_CHANNEL_0 */
    ADC1_CHANNEL_1,  /* ADC_CHANNEL_1 */
    ADC1_CHANNEL_2,  /* ADC_CHANNEL_2 */
    ADC1_CHANNEL_3,  /* ADC_CHANNEL_3 */
    ADC1_CHANNEL_4,  /* ADC_CHANNEL_4 */
    ADC1_CHANNEL_5,  /* ADC_CHANNEL_5 */
    ADC1_CHANNEL_6,  /* ADC_CHANNEL_6 */
    ADC1_CHANNEL_7,  /* ADC_CHANNEL_7 */
};

/* ADC设备句柄结构�?*/
typedef struct {
    adc_config_t config;           /* ADC配置参数 */
    bool initialized;              /* 初始化标�?*/
    bool continuous_mode;          /* 连续模式标志 */
    adc_conversion_callback_t callback; /* 转换结果回调函数 */
    void *user_data;               /* 用户数据 */
    esp_adc_cal_characteristics_t adc_chars; /* ADC校准特�?*/
} esp32_adc_handle_t;

/* ADC设备句柄数组 */
static esp32_adc_handle_t g_adc_handles[ADC_CHANNEL_MAX];

/**
 * @brief 将抽象分辨率转换为ESP32分辨�?
 * 
 * @param resolution 抽象分辨�?
 * @return adc_bits_width_t ESP32分辨�?
 */
static adc_bits_width_t convert_resolution(adc_resolution_t resolution)
{
    switch (resolution) {
        case ADC_RESOLUTION_6BIT:
            // ESP32不支�?位分辨率，降级到9�?
            ESP_LOGW(TAG, "6-bit resolution not supported, using 9-bit");
            return ADC_WIDTH_9Bit;
        case ADC_RESOLUTION_8BIT:
            // ESP32不支�?位分辨率，降级到9�?
            ESP_LOGW(TAG, "8-bit resolution not supported, using 9-bit");
            return ADC_WIDTH_9Bit;
        case ADC_RESOLUTION_10BIT:
            return ADC_WIDTH_10Bit;
        case ADC_RESOLUTION_12BIT:
            return ADC_WIDTH_12Bit;
        case ADC_RESOLUTION_14BIT:
            // ESP32不支�?4位分辨率，降级到12�?
            ESP_LOGW(TAG, "14-bit resolution not supported, using 12-bit");
            return ADC_WIDTH_12Bit;
        case ADC_RESOLUTION_16BIT:
            // ESP32不支�?6位分辨率，降级到12�?
            ESP_LOGW(TAG, "16-bit resolution not supported, using 12-bit");
            return ADC_WIDTH_12Bit;
        default:
            return ADC_WIDTH_12Bit;
    }
}

/**
 * @brief 将抽象参考电压源转换为ESP32参考电压源
 * 
 * @param reference 抽象参考电压源
 * @return adc_atten_t ESP32参考电压源
 */
static adc_atten_t convert_reference(adc_reference_t reference)
{
    switch (reference) {
        case ADC_REFERENCE_INTERNAL:
            return ADC_ATTEN_DB_0;  /* 0dB衰减，满量程0.8V */
        case ADC_REFERENCE_EXTERNAL:
        case ADC_REFERENCE_VDDA:
            return ADC_ATTEN_DB_11; /* 11dB衰减，满量程3.3V */
        case ADC_REFERENCE_VREFINT:
            return ADC_ATTEN_DB_6;  /* 6dB衰减，满量程2.2V */
        default:
            return ADC_ATTEN_DB_11;
    }
}

/**
 * @brief 初始化ADC
 * 
 * @param config ADC配置参数
 * @param handle ADC设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int adc_init(const adc_config_t *config, adc_handle_t *handle)
{
    esp32_adc_handle_t *esp32_handle;
    adc_bits_width_t width;
    adc_atten_t atten;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= ADC_CHANNEL_MAX || config->channel > ADC1_CHANNEL_7) {
        ESP_LOGE(TAG, "Invalid ADC channel: %d", config->channel);
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已初始化 */
    esp32_handle = &g_adc_handles[config->channel];
    if (esp32_handle->initialized) {
        return ERROR_BUSY;
    }
    
    /* 初始化句�?*/
    memset(esp32_handle, 0, sizeof(esp32_adc_handle_t));
    esp32_handle->config = *config;
    
    /* 配置ADC1 */
    width = convert_resolution(config->resolution);
    atten = convert_reference(config->reference);
    
    /* 配置ADC1分辨�?*/
    ret = adc1_config_width(width);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC1 config width failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 配置ADC1通道 */
    ret = adc1_config_channel_atten(ADC1_CHANNEL_MAP[config->channel], atten);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC1 config channel failed: %d", ret);
        return ERROR_DRIVER;
    }
    
    /* 特性校�?- 获取更准确的电压读数 */
    esp_adc_cal_characterize(ADC_UNIT_1, atten, width, 
                          config->reference_voltage * 1000, /* mV */
                          &esp32_handle->adc_chars);
    
    /* 标记为已初始�?*/
    esp32_handle->initialized = true;
    *handle = (adc_handle_t)esp32_handle;
    
    ESP_LOGI(TAG, "ADC initialized: channel=%d, resolution=%d, reference=%d",
           config->channel, config->resolution, config->reference);
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化ADC
 * 
 * @param handle ADC设备句柄
 * @return int 0表示成功，非0表示失败
 */
int adc_deinit(adc_handle_t handle)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 如果处于连续模式，先停止 */
    if (esp32_handle->continuous_mode) {
        adc_stop_continuous(handle);
    }
    
    /* 重置句柄 */
    memset(esp32_handle, 0, sizeof(esp32_adc_handle_t));
    
    return DRIVER_OK;
}

/**
 * @brief 执行单次ADC转换
 * 
 * @param handle ADC设备句柄
 * @param value 转换结果
 * @return int 0表示成功，非0表示失败
 */
int adc_read(adc_handle_t handle, uint32_t *value)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 执行ADC转换 */
    *value = adc1_get_raw(ADC1_CHANNEL_MAP[esp32_handle->config.channel]);
    
    return DRIVER_OK;
}

/**
 * @brief 连续ADC转换任务
 * 
 * @param arg 任务参数
 */
static void continuous_adc_task(void *arg)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)arg;
    uint32_t value;
    
    while (esp32_handle->continuous_mode) {
        /* 执行ADC转换 */
        value = adc1_get_raw(ADC1_CHANNEL_MAP[esp32_handle->config.channel]);
        
        /* 调用回调函数 */
        if (esp32_handle->callback) {
            esp32_handle->callback(value, esp32_handle->user_data);
        }
        
        /* 根据采样速率延时 */
        switch (esp32_handle->config.sample_rate) {
            case ADC_SAMPLE_RATE_SLOW:
                vTaskDelay(pdMS_TO_TICKS(100)); /* 10Hz */
                break;
            case ADC_SAMPLE_RATE_MEDIUM:
                vTaskDelay(pdMS_TO_TICKS(10));  /* 100Hz */
                break;
            case ADC_SAMPLE_RATE_FAST:
                vTaskDelay(pdMS_TO_TICKS(1));   /* 1000Hz */
                break;
            case ADC_SAMPLE_RATE_VERY_FAST:
                vTaskDelay(1);                  /* 最快速率 */
                break;
            default:
                vTaskDelay(pdMS_TO_TICKS(10));  /* 默认100Hz */
                break;
        }
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief 启动连续ADC转换
 * 
 * @param handle ADC设备句柄
 * @param callback 转换结果回调函数
 * @param user_data 用户数据，传递给回调函数
 * @return int 0表示成功，非0表示失败
 */
int adc_start_continuous(adc_handle_t handle, adc_conversion_callback_t callback, void *user_data)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)handle;
    BaseType_t ret;
    
    /* 参数检�?*/
    if (esp32_handle == NULL || callback == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查是否已处于连续模式 */
    if (esp32_handle->continuous_mode) {
        return ERROR_BUSY;
    }
    
    /* 设置回调函数和用户数�?*/
    esp32_handle->callback = callback;
    esp32_handle->user_data = user_data;
    esp32_handle->continuous_mode = true;
    
    /* 创建ADC转换任务 */
    ret = xTaskCreate(continuous_adc_task, "adc_task", 2048, 
                    esp32_handle, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ADC task");
        esp32_handle->continuous_mode = false;
        return ERROR_RESOURCE;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 停止连续ADC转换
 * 
 * @param handle ADC设备句柄
 * @return int 0表示成功，非0表示失败
 */
int adc_stop_continuous(adc_handle_t handle)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)handle;
    
    /* 参数检�?*/
    if (esp32_handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 检查是否处于连续模�?*/
    if (!esp32_handle->continuous_mode) {
        return DRIVER_OK;
    }
    
    /* 停止连续模式 */
    esp32_handle->continuous_mode = false;
    
    /* 给任务一些时间来退�?*/
    vTaskDelay(pdMS_TO_TICKS(10));
    
    return DRIVER_OK;
}

/**
 * @brief 将ADC原始值转换为电压�?
 * 
 * @param handle ADC设备句柄
 * @param raw_value 原始ADC�?
 * @param voltage 电压�?伏特)
 * @return int 0表示成功，非0表示失败
 */
int adc_convert_to_voltage(adc_handle_t handle, uint32_t raw_value, float *voltage)
{
    esp32_adc_handle_t *esp32_handle = (esp32_adc_handle_t *)handle;
    uint32_t voltage_mv;
    
    /* 参数检�?*/
    if (esp32_handle == NULL || voltage == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (!esp32_handle->initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* 使用特性校准转换为毫伏 */
    voltage_mv = esp_adc_cal_raw_to_voltage(raw_value, &esp32_handle->adc_chars);
    *voltage = voltage_mv / 1000.0f;
    
    return DRIVER_OK;
}

/**
 * @brief 获取ADC分辨率对应的最大原始�?
 * 
 * @param resolution ADC分辨�?
 * @return uint32_t 最大原始�?
 */
uint32_t adc_get_max_value(adc_resolution_t resolution)
{
    switch (resolution) {
        case ADC_RESOLUTION_6BIT:
            return (1 << 6) - 1;
        case ADC_RESOLUTION_8BIT:
            return (1 << 8) - 1;
        case ADC_RESOLUTION_10BIT:
            return (1 << 10) - 1;
        case ADC_RESOLUTION_12BIT:
            return (1 << 12) - 1;
        case ADC_RESOLUTION_14BIT:
            return (1 << 14) - 1;
        case ADC_RESOLUTION_16BIT:
            return (1 << 16) - 1;
        default:
            return (1 << 12) - 1;
    }
}

/**
 * @brief 设置ADC转换触发�?
 * 
 * @param handle ADC设备句柄
 * @param trigger_source 触发源ID（与平台相关�?
 * @return int 0表示成功，非0表示失败
 */
int adc_set_trigger_source(adc_handle_t handle, uint32_t trigger_source)
{
    /* ESP32 ADC驱动不支持外部触�?*/
    return ERROR_NOT_SUPPORTED;
}

