/**
 * @file adc_api.h
 * @brief ADC接口抽象层定义
 *
 * 该头文件定义了ADC接口的统一抽象，使上层应用能够与底层ADC硬件实现解耦
 */

#ifndef ADC_API_H
#define ADC_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* ADC通道ID定义 */
typedef enum {
    ADC_CHANNEL_0 = 0,    /**< ADC通道0 */
    ADC_CHANNEL_1,        /**< ADC通道1 */
    ADC_CHANNEL_2,        /**< ADC通道2 */
    ADC_CHANNEL_3,        /**< ADC通道3 */
    ADC_CHANNEL_4,        /**< ADC通道4 */
    ADC_CHANNEL_5,        /**< ADC通道5 */
    ADC_CHANNEL_6,        /**< ADC通道6 */
    ADC_CHANNEL_7,        /**< ADC通道7 */
    ADC_CHANNEL_8,        /**< ADC通道8 */
    ADC_CHANNEL_9,        /**< ADC通道9 */
    ADC_CHANNEL_10,       /**< ADC通道10 */
    ADC_CHANNEL_11,       /**< ADC通道11 */
    ADC_CHANNEL_12,       /**< ADC通道12 */
    ADC_CHANNEL_13,       /**< ADC通道13 */
    ADC_CHANNEL_14,       /**< ADC通道14 */
    ADC_CHANNEL_15,       /**< ADC通道15 */
    ADC_CHANNEL_MAX       /**< ADC通道最大数量 */
} adc_channel_t;

/* ADC分辨率定义 */
typedef enum {
    ADC_RESOLUTION_6BIT = 0,  /**< 6位分辨率 */
    ADC_RESOLUTION_8BIT,      /**< 8位分辨率 */
    ADC_RESOLUTION_10BIT,     /**< 10位分辨率 */
    ADC_RESOLUTION_12BIT,     /**< 12位分辨率 */
    ADC_RESOLUTION_14BIT,     /**< 14位分辨率 */
    ADC_RESOLUTION_16BIT      /**< 16位分辨率 */
} adc_resolution_t;

/* ADC参考电压源定义 */
typedef enum {
    ADC_REFERENCE_INTERNAL = 0,  /**< 内部参考电压 */
    ADC_REFERENCE_EXTERNAL,      /**< 外部参考电压 */
    ADC_REFERENCE_VDDA,          /**< VDDA作为参考电压 */
    ADC_REFERENCE_VREFINT        /**< 内部参考电压 VREFINT */
} adc_reference_t;

/* ADC采样速率定义 */
typedef enum {
    ADC_SAMPLE_RATE_SLOW = 0,    /**< 低速采样 */
    ADC_SAMPLE_RATE_MEDIUM,      /**< 中速采样 */
    ADC_SAMPLE_RATE_FAST,        /**< 高速采样 */
    ADC_SAMPLE_RATE_VERY_FAST    /**< 超高速采样 */
} adc_sample_rate_t;

/* ADC配置参数 */
typedef struct {
    adc_channel_t channel;         /**< ADC通道 */
    adc_resolution_t resolution;   /**< 分辨率 */
    adc_reference_t reference;     /**< 参考电压源 */
    adc_sample_rate_t sample_rate; /**< 采样速率 */
    float reference_voltage;       /**< 参考电压值(伏特)，用于电压转换 */
} adc_config_t;

/* ADC转换结果回调函数类型 */
typedef void (*adc_conversion_callback_t)(uint32_t value, void *user_data);

/* ADC设备句柄 */
typedef driver_handle_t adc_handle_t;

/**
 * @brief 初始化ADC
 * 
 * @param config ADC配置参数
 * @param handle ADC设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int adc_init(const adc_config_t *config, adc_handle_t *handle);

/**
 * @brief 去初始化ADC
 * 
 * @param handle ADC设备句柄
 * @return int 0表示成功，非0表示失败
 */
int adc_deinit(adc_handle_t handle);

/**
 * @brief 执行单次ADC转换
 * 
 * @param handle ADC设备句柄
 * @param value 转换结果
 * @return int 0表示成功，非0表示失败
 */
int adc_read(adc_handle_t handle, uint32_t *value);

/**
 * @brief 启动连续ADC转换
 * 
 * @param handle ADC设备句柄
 * @param callback 转换结果回调函数
 * @param user_data 用户数据，传递给回调函数
 * @return int 0表示成功，非0表示失败
 */
int adc_start_continuous(adc_handle_t handle, adc_conversion_callback_t callback, void *user_data);

/**
 * @brief 停止连续ADC转换
 * 
 * @param handle ADC设备句柄
 * @return int 0表示成功，非0表示失败
 */
int adc_stop_continuous(adc_handle_t handle);

/**
 * @brief 将ADC原始值转换为电压值
 * 
 * @param handle ADC设备句柄
 * @param raw_value 原始ADC值
 * @param voltage 电压值(伏特)
 * @return int 0表示成功，非0表示失败
 */
int adc_convert_to_voltage(adc_handle_t handle, uint32_t raw_value, float *voltage);

/**
 * @brief 获取ADC分辨率对应的最大原始值
 * 
 * @param resolution ADC分辨率
 * @return uint32_t 最大原始值
 */
uint32_t adc_get_max_value(adc_resolution_t resolution);

/**
 * @brief 设置ADC转换触发源
 * 
 * @param handle ADC设备句柄
 * @param trigger_source 触发源ID（与平台相关）
 * @return int 0表示成功，非0表示失败
 */
int adc_set_trigger_source(adc_handle_t handle, uint32_t trigger_source);

#endif /* ADC_API_H */
