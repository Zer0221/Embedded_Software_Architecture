/**
 * @file fm33lc0xx_adc.c
 * @brief FM33LC0xx平台ADC驱动实现
 *
 * 该文件实现了FM33LC0xx平台的ADC驱动接口
 */

#include "base/adc_api.h"
#include "common/error_api.h"
#include "fm33lc0xx_fl.h"
#include <string.h>

/* 驱动版本�?*/
#define FM33LC0XX_ADC_DRIVER_VERSION "1.0.0"

/* ADC通道映射�?*/
static const uint32_t ADC_CHANNEL_MAP[] = {
    FL_ADC_EXTERNAL_CH0,  /* ADC_CHANNEL_0 - PA0 */
    FL_ADC_EXTERNAL_CH1,  /* ADC_CHANNEL_1 - PA1 */
    FL_ADC_EXTERNAL_CH2,  /* ADC_CHANNEL_2 - PA2 */
    FL_ADC_EXTERNAL_CH3,  /* ADC_CHANNEL_3 - PA3 */
    FL_ADC_EXTERNAL_CH4,  /* ADC_CHANNEL_4 - PA4 */
    FL_ADC_EXTERNAL_CH5,  /* ADC_CHANNEL_5 - PA5 */
    FL_ADC_EXTERNAL_CH6,  /* ADC_CHANNEL_6 - PA6 */
    FL_ADC_EXTERNAL_CH7,  /* ADC_CHANNEL_7 - PA7 */
    FL_ADC_EXTERNAL_CH8,  /* ADC_CHANNEL_8 - PB0 */
    FL_ADC_EXTERNAL_CH9,  /* ADC_CHANNEL_9 - PB1 */
    FL_ADC_EXTERNAL_CH10, /* ADC_CHANNEL_10 - PC0 */
    FL_ADC_EXTERNAL_CH11, /* ADC_CHANNEL_11 - PC1 */
    FL_ADC_EXTERNAL_CH12, /* ADC_CHANNEL_12 - PD6 */
    FL_ADC_EXTERNAL_CH13, /* ADC_CHANNEL_13 - PD7 */
    FL_ADC_INTERNAL_CH0,  /* ADC_CHANNEL_14 - 内部温度传感�?*/
    FL_ADC_INTERNAL_CH1   /* ADC_CHANNEL_15 - 内部参考电�?*/
};

/* ADC分辨率映射表 */
static const uint32_t ADC_RESOLUTION_MAP[] = {
    0,                      /* ADC_RESOLUTION_6BIT - 不支�?*/
    0,                      /* ADC_RESOLUTION_8BIT - 不支�?*/
    0,                      /* ADC_RESOLUTION_10BIT - 不支�?*/
    FL_ADC_RESOLUTION_12B,  /* ADC_RESOLUTION_12BIT */
    0,                      /* ADC_RESOLUTION_14BIT - 不支�?*/
    0                       /* ADC_RESOLUTION_16BIT - 不支�?*/
};

/* ADC参考电压映射表 */
static const uint32_t ADC_REFERENCE_MAP[] = {
    FL_ADC_REF_SOURCE_INTERNAL, /* ADC_REFERENCE_INTERNAL */
    FL_ADC_REF_SOURCE_EXTERNAL, /* ADC_REFERENCE_EXTERNAL */
    FL_ADC_REF_SOURCE_VDD,      /* ADC_REFERENCE_VDDA */
    FL_ADC_REF_SOURCE_VREF      /* ADC_REFERENCE_VREFINT */
};

/* ADC采样时间映射�?*/
static const uint32_t ADC_SAMPLE_TIME_MAP[] = {
    FL_ADC_SAMPLING_TIME_3_ADCCLK,    /* ADC_SAMPLE_RATE_SLOW */
    FL_ADC_SAMPLING_TIME_6_ADCCLK,    /* ADC_SAMPLE_RATE_MEDIUM */
    FL_ADC_SAMPLING_TIME_12_ADCCLK,   /* ADC_SAMPLE_RATE_FAST */
    FL_ADC_SAMPLING_TIME_24_ADCCLK    /* ADC_SAMPLE_RATE_VERY_FAST */
};

/* ADC设备实例结构�?*/
typedef struct {
    adc_config_t config;                    /* ADC配置参数 */
    bool initialized;                       /* 初始化标�?*/
    adc_conversion_callback_t callback;     /* 转换完成回调函数 */
    void *user_data;                        /* 用户数据指针 */
    bool continuous_mode;                   /* 连续转换模式标志 */
} fm33lc0xx_adc_device_t;

/* 全局设备实例 */
static fm33lc0xx_adc_device_t g_adc_device;

/* ADC中断处理函数 */
void ADC_IRQHandler(void)
{
    /* 检查转换完成中�?*/
    if (FL_ADC_IsEnabledIT_EOC(ADC) && FL_ADC_IsActiveFlag_EOC(ADC)) {
        /* 清除中断标志 */
        FL_ADC_ClearFlag_EOC(ADC);
        
        /* 获取转换结果 */
        uint32_t value = FL_ADC_ReadConversionData(ADC);
        
        /* 如果不是连续模式，停止转�?*/
        if (!g_adc_device.continuous_mode) {
            FL_ADC_Disable(ADC);
        }
        
        /* 调用回调函数 */
        if (g_adc_device.callback != NULL) {
            g_adc_device.callback(value, g_adc_device.user_data);
        }
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
    if (config == NULL || handle == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查参数有效�?*/
    if (config->channel >= ADC_CHANNEL_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查分辨率是否支持 */
    if (config->resolution != ADC_RESOLUTION_12BIT) {
        return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 防止重复初始�?*/
    if (g_adc_device.initialized) {
        return DRIVER_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置参数 */
    memcpy(&g_adc_device.config, config, sizeof(adc_config_t));
    g_adc_device.callback = NULL;
    g_adc_device.user_data = NULL;
    g_adc_device.continuous_mode = false;
    
    /* 使能ADC时钟 */
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ADC);
    
    /* 初始化ADC配置 */
    FL_ADC_CommonInitTypeDef ADC_CommonInitStruct;
    FL_ADC_InitTypeDef ADC_InitStruct;
    
    /* ADC公共配置 */
    ADC_CommonInitStruct.referenceSource = ADC_REFERENCE_MAP[config->reference];
    ADC_CommonInitStruct.clockSource = FL_CMU_ADC_CLK_SOURCE_RCHF;
    ADC_CommonInitStruct.clockPrescaler = FL_CMU_ADC_PSC_DIV8; /* 48MHz / 8 = 6MHz */
    FL_ADC_CommonInit(&ADC_CommonInitStruct);
    
    /* ADC配置 */
    ADC_InitStruct.conversionMode = FL_ADC_CONV_MODE_SINGLE;
    ADC_InitStruct.autoMode = FL_DISABLE;
    ADC_InitStruct.scanDirection = FL_ADC_SEQ_SCAN_DIR_FORWARD;
    ADC_InitStruct.externalTrigConv = FL_ADC_TRIGGER_SOURCE_SOFTWARE;
    ADC_InitStruct.triggerEdge = FL_ADC_TRIGGER_EDGE_NONE;
    ADC_InitStruct.dataAlignment = FL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.overrunMode = FL_ENABLE;
    ADC_InitStruct.sampleTime = ADC_SAMPLE_TIME_MAP[config->sample_rate];
    
    /* 分辨率固定为12bit */
    ADC_InitStruct.resolution = FL_ADC_RESOLUTION_12B;
    
    FL_ADC_Init(ADC, &ADC_InitStruct);
    
    /* 配置通道 */
    FL_ADC_SetSequenceLength(ADC, FL_ADC_SEQ_LENGTH_1);
    FL_ADC_SetSequenceChannelOffset(ADC, 0, ADC_CHANNEL_MAP[config->channel]);
    
    /* 使能ADC */
    FL_ADC_Enable(ADC);
    
    /* 标记初始化完�?*/
    g_adc_device.initialized = true;
    
    /* 返回设备句柄 */
    *handle = (adc_handle_t)&g_adc_device;
    
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
    fm33lc0xx_adc_device_t *device = (fm33lc0xx_adc_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_adc_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 禁用ADC中断 */
    FL_ADC_DisableIT_EOC(ADC);
    NVIC_DisableIRQ(ADC_IRQn);
    
    /* 禁用ADC */
    FL_ADC_Disable(ADC);
    
    /* 关闭ADC时钟 */
    FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ADC);
    
    /* 重置设备状�?*/
    device->initialized = false;
    device->callback = NULL;
    device->user_data = NULL;
    device->continuous_mode = false;
    
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
    fm33lc0xx_adc_device_t *device = (fm33lc0xx_adc_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_adc_device || !device->initialized || value == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 确保ADC已使�?*/
    if (FL_ADC_IsEnabled(ADC) == 0) {
        FL_ADC_Enable(ADC);
    }
    
    /* 开始转�?*/
    FL_ADC_StartConversion(ADC);
    
    /* 等待转换完成 */
    while (FL_ADC_IsActiveFlag_EOC(ADC) == 0) {
        /* 等待 */
    }
    
    /* 获取转换结果 */
    *value = FL_ADC_ReadConversionData(ADC);
    
    /* 清除转换完成标志 */
    FL_ADC_ClearFlag_EOC(ADC);
    
    return DRIVER_OK;
}

/**
 * @brief 开始连续ADC转换
 * 
 * @param handle ADC设备句柄
 * @param callback 转换完成回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int adc_start_continuous(adc_handle_t handle, adc_conversion_callback_t callback, void *user_data)
{
    fm33lc0xx_adc_device_t *device = (fm33lc0xx_adc_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_adc_device || !device->initialized || callback == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 保存回调函数和用户数�?*/
    device->callback = callback;
    device->user_data = user_data;
    device->continuous_mode = true;
    
    /* 配置为连续转换模�?*/
    FL_ADC_SetConversionMode(ADC, FL_ADC_CONV_MODE_CONTINUOUS);
    
    /* 使能转换完成中断 */
    FL_ADC_EnableIT_EOC(ADC);
    NVIC_EnableIRQ(ADC_IRQn);
    NVIC_SetPriority(ADC_IRQn, 3);
    
    /* 确保ADC已使�?*/
    if (FL_ADC_IsEnabled(ADC) == 0) {
        FL_ADC_Enable(ADC);
    }
    
    /* 开始转�?*/
    FL_ADC_StartConversion(ADC);
    
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
    fm33lc0xx_adc_device_t *device = (fm33lc0xx_adc_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_adc_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 禁用转换完成中断 */
    FL_ADC_DisableIT_EOC(ADC);
    NVIC_DisableIRQ(ADC_IRQn);
    
    /* 停止ADC转换 */
    FL_ADC_Disable(ADC);
    
    /* 恢复为单次转换模�?*/
    FL_ADC_SetConversionMode(ADC, FL_ADC_CONV_MODE_SINGLE);
    
    /* 清除连续模式标志 */
    device->continuous_mode = false;
    
    return DRIVER_OK;
}

/**
 * @brief 将ADC原始值转换为电压�?
 * 
 * @param handle ADC设备句柄
 * @param raw_value ADC原始�?
 * @param voltage 转换后的电压�?伏特)
 * @return int 0表示成功，非0表示失败
 */
int adc_raw_to_voltage(adc_handle_t handle, uint32_t raw_value, float *voltage)
{
    fm33lc0xx_adc_device_t *device = (fm33lc0xx_adc_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_adc_device || !device->initialized || voltage == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 12位ADC的最大�?*/
    uint32_t max_value = (1 << 12) - 1;
    
    /* 计算电压�?*/
    *voltage = (float)raw_value * device->config.reference_voltage / max_value;
    
    return DRIVER_OK;
}

/**
 * @brief 获取ADC驱动版本
 * 
 * @return const char* 版本字符�?
 */
const char* adc_get_version(void)
{
    return FM33LC0XX_ADC_DRIVER_VERSION;
}

