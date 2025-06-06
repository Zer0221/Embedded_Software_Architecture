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
#include "common/driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

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
} adc_channel_id_t;

/* ADC分辨率定义 */
typedef enum {
    ADC_RESOLUTION_6BIT = 6,   /**< 6位分辨率 */
    ADC_RESOLUTION_8BIT = 8,   /**< 8位分辨率 */
    ADC_RESOLUTION_10BIT = 10, /**< 10位分辨率 */
    ADC_RESOLUTION_12BIT = 12, /**< 12位分辨率 */
    ADC_RESOLUTION_14BIT = 14, /**< 14位分辨率 */
    ADC_RESOLUTION_16BIT = 16  /**< 16位分辨率 */
} adc_resolution_t;

/* ADC采样率定义 */
typedef enum {
    ADC_SAMPLERATE_1KSPS = 1000,        /**< 1KSPS采样率 */
    ADC_SAMPLERATE_10KSPS = 10000,      /**< 10KSPS采样率 */
    ADC_SAMPLERATE_100KSPS = 100000,    /**< 100KSPS采样率 */
    ADC_SAMPLERATE_500KSPS = 500000,    /**< 500KSPS采样率 */
    ADC_SAMPLERATE_1MSPS = 1000000,     /**< 1MSPS采样率 */
    ADC_SAMPLERATE_2MSPS = 2000000      /**< 2MSPS采样率 */
} adc_samplerate_t;

/* ADC参考电压源 */
typedef enum {
    ADC_REFVOLT_INTERNAL,    /**< 内部参考电压 */
    ADC_REFVOLT_EXTERNAL,    /**< 外部参考电压 */
    ADC_REFVOLT_VDD          /**< 使用VDD作为参考电压 */
} adc_ref_voltage_t;

/* ADC工作模式 */
typedef enum {
    ADC_MODE_SINGLE_CONV,    /**< 单次转换模式 */
    ADC_MODE_CONTINUOUS,     /**< 连续转换模式 */
    ADC_MODE_SCAN,           /**< 扫描模式 */
    ADC_MODE_DMA             /**< DMA模式 */
} adc_mode_t;

/* ADC配置结构体 */
typedef struct {
    adc_channel_id_t   channel;      /**< ADC通道 */
    adc_resolution_t   resolution;   /**< 分辨率 */
    adc_samplerate_t   samplerate;   /**< 采样率 */
    adc_ref_voltage_t  ref_voltage;  /**< 参考电压源 */
    adc_mode_t         mode;         /**< 工作模式 */
    bool               use_dma;      /**< 是否使用DMA */
    uint32_t           buffer_size;  /**< 缓冲区大小 */
} adc_config_t;

/* ADC事件类型 */
typedef enum {
    ADC_EVENT_CONVERSION_DONE,    /**< 转换完成 */
    ADC_EVENT_BUFFER_FULL,        /**< 缓冲区满 */
    ADC_EVENT_DATA_OVERRUN,       /**< 数据溢出 */
    ADC_EVENT_ERROR               /**< 错误 */
} adc_event_type_t;

/* ADC事件结构体 */
typedef struct {
    adc_event_type_t type;        /**< 事件类型 */
    union {
        uint32_t  value;          /**< 采样值 */
        struct {
            uint32_t *data;       /**< 数据指针 */
            uint32_t size;        /**< 数据大小 */
        } buffer;
    } data;
} adc_event_t;

/* ADC事件回调函数类型 */
typedef void (*adc_event_callback_t)(adc_event_t *event, void *user_data);

/**
 * @brief 初始化ADC设备
 *
 * @param config 配置参数
 * @return 成功返回ADC句柄，失败返回NULL
 */
driver_handle_t adc_init(adc_config_t *config);

/**
 * @brief 关闭ADC设备
 *
 * @param handle ADC句柄
 * @return 成功返回0，失败返回负数错误码
 */
int adc_deinit(driver_handle_t handle);

/**
 * @brief 开始ADC采样
 *
 * @param handle ADC句柄
 * @return 成功返回0，失败返回负数错误码
 */
int adc_start(driver_handle_t handle);

/**
 * @brief 停止ADC采样
 *
 * @param handle ADC句柄
 * @return 成功返回0，失败返回负数错误码
 */
int adc_stop(driver_handle_t handle);

/**
 * @brief 读取ADC采样值
 *
 * @param handle ADC句柄
 * @param value 采样值存储地址
 * @param timeout_ms 超时时间(毫秒)，0表示非阻塞，UINT32_MAX表示永久阻塞
 * @return 成功返回0，失败返回负数错误码
 */
int adc_read(driver_handle_t handle, uint32_t *value, uint32_t timeout_ms);

/**
 * @brief 读取多个ADC采样值
 *
 * @param handle ADC句柄
 * @param buffer 数据缓冲区
 * @param size 要读取的采样点数
 * @param timeout_ms 超时时间(毫秒)，0表示非阻塞，UINT32_MAX表示永久阻塞
 * @return 成功返回读取的采样点数，失败返回负数错误码
 */
int adc_read_multi(driver_handle_t handle, uint32_t *buffer, uint32_t size, uint32_t timeout_ms);

/**
 * @brief 注册ADC事件回调函数
 *
 * @param handle ADC句柄
 * @param callback 回调函数
 * @param user_data 用户数据，会在回调中传递
 * @return 成功返回0，失败返回负数错误码
 */
int adc_register_event_callback(driver_handle_t handle, adc_event_callback_t callback, void *user_data);

/**
 * @brief 将ADC采样值转换为电压值
 *
 * @param handle ADC句柄
 * @param adc_value ADC采样值
 * @return 对应的电压值(mV)
 */
uint32_t adc_convert_to_voltage(driver_handle_t handle, uint32_t adc_value);

#ifdef __cplusplus
}
#endif

#endif /* ADC_API_H */