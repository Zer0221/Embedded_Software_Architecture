/**
 * @file sensor_api.h
 * @brief 环境传感器抽象接口定义
 *
 * 该头文件定义了环境传感器的统一抽象接口，支持温度、湿度、气压、光照等各类传感器
 */

#ifndef SENSOR_API_H
#define SENSOR_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 传感器类型 */
typedef enum {
    SENSOR_TYPE_TEMPERATURE,    /**< 温度传感器 */
    SENSOR_TYPE_HUMIDITY,       /**< 湿度传感器 */
    SENSOR_TYPE_PRESSURE,       /**< 气压传感器 */
    SENSOR_TYPE_LIGHT,          /**< 光照传感器 */
    SENSOR_TYPE_PROXIMITY,      /**< 接近传感器 */
    SENSOR_TYPE_MOTION,         /**< 运动传感器 */
    SENSOR_TYPE_ACCELEROMETER,  /**< 加速度传感器 */
    SENSOR_TYPE_GYROSCOPE,      /**< 陀螺仪 */
    SENSOR_TYPE_MAGNETOMETER,   /**< 磁力计 */
    SENSOR_TYPE_GAS,            /**< 气体传感器 */
    SENSOR_TYPE_CO2,            /**< 二氧化碳传感器 */
    SENSOR_TYPE_VOC,            /**< 挥发性有机物传感器 */
    SENSOR_TYPE_UV,             /**< 紫外线传感器 */
    SENSOR_TYPE_DUST,           /**< 灰尘传感器 */
    SENSOR_TYPE_SOUND,          /**< 声音传感器 */
    SENSOR_TYPE_ALTITUDE,       /**< 高度传感器 */
    SENSOR_TYPE_COLOR,          /**< 颜色传感器 */
    SENSOR_TYPE_GESTURE,        /**< 手势传感器 */
    SENSOR_TYPE_HEART_RATE,     /**< 心率传感器 */
    SENSOR_TYPE_ECG,            /**< 心电图传感器 */
    SENSOR_TYPE_CUSTOM          /**< 自定义传感器 */
} sensor_type_t;

/* 传感器状态 */
typedef enum {
    SENSOR_STATE_INACTIVE,      /**< 未激活 */
    SENSOR_STATE_ACTIVE,        /**< 已激活 */
    SENSOR_STATE_SUSPENDED,     /**< 已暂停 */
    SENSOR_STATE_ERROR          /**< 错误状态 */
} sensor_state_t;

/* 传感器接口类型 */
typedef enum {
    SENSOR_INTERFACE_I2C,       /**< I2C接口 */
    SENSOR_INTERFACE_SPI,       /**< SPI接口 */
    SENSOR_INTERFACE_ANALOG,    /**< 模拟接口 */
    SENSOR_INTERFACE_DIGITAL,   /**< 数字接口 */
    SENSOR_INTERFACE_UART,      /**< UART接口 */
    SENSOR_INTERFACE_ONEWIRE,   /**< 单总线接口 */
    SENSOR_INTERFACE_CUSTOM     /**< 自定义接口 */
} sensor_interface_t;

/* 传感器采样模式 */
typedef enum {
    SENSOR_MODE_ONESHOT,        /**< 单次模式 */
    SENSOR_MODE_CONTINUOUS      /**< 连续模式 */
} sensor_mode_t;

/* 传感器采样率 */
typedef enum {
    SENSOR_RATE_LOWEST,         /**< 最低采样率 */
    SENSOR_RATE_LOW,            /**< 低采样率 */
    SENSOR_RATE_MEDIUM,         /**< 中等采样率 */
    SENSOR_RATE_HIGH,           /**< 高采样率 */
    SENSOR_RATE_HIGHEST,        /**< 最高采样率 */
    SENSOR_RATE_CUSTOM          /**< 自定义采样率 */
} sensor_rate_t;

/* 传感器精度 */
typedef enum {
    SENSOR_ACCURACY_LOW,        /**< 低精度 */
    SENSOR_ACCURACY_MEDIUM,     /**< 中等精度 */
    SENSOR_ACCURACY_HIGH,       /**< 高精度 */
    SENSOR_ACCURACY_CUSTOM      /**< 自定义精度 */
} sensor_accuracy_t;

/* 传感器过滤模式 */
typedef enum {
    SENSOR_FILTER_NONE,         /**< 无过滤 */
    SENSOR_FILTER_AVERAGE,      /**< 平均值过滤 */
    SENSOR_FILTER_MEDIAN,       /**< 中值过滤 */
    SENSOR_FILTER_KALMAN,       /**< 卡尔曼过滤 */
    SENSOR_FILTER_CUSTOM        /**< 自定义过滤 */
} sensor_filter_t;

/* 传感器值类型 */
typedef enum {
    SENSOR_VALUE_TYPE_INT,      /**< 整数值 */
    SENSOR_VALUE_TYPE_FLOAT,    /**< 浮点值 */
    SENSOR_VALUE_TYPE_RAW,      /**< 原始值 */
    SENSOR_VALUE_TYPE_BOOL,     /**< 布尔值 */
    SENSOR_VALUE_TYPE_STRING,   /**< 字符串值 */
    SENSOR_VALUE_TYPE_VECTOR,   /**< 向量值 */
    SENSOR_VALUE_TYPE_MATRIX,   /**< 矩阵值 */
    SENSOR_VALUE_TYPE_CUSTOM    /**< 自定义值 */
} sensor_value_type_t;

/* 传感器接口配置 */
typedef struct {
    sensor_interface_t type;    /**< 接口类型 */
    union {
        struct {
            uint8_t bus;        /**< I2C总线号 */
            uint8_t addr;       /**< I2C设备地址 */
            uint32_t speed;     /**< I2C速度 */
        } i2c;
        struct {
            uint8_t bus;        /**< SPI总线号 */
            uint8_t cs_pin;     /**< SPI片选引脚 */
            uint32_t speed;     /**< SPI速度 */
            uint8_t mode;       /**< SPI模式 */
        } spi;
        struct {
            uint8_t adc_ch;     /**< ADC通道 */
            uint8_t resolution; /**< ADC分辨率 */
        } analog;
        struct {
            uint8_t gpio_pin;   /**< GPIO引脚 */
            bool active_high;   /**< 是否高电平有效 */
        } digital;
        struct {
            uint8_t uart_port;  /**< UART端口 */
            uint32_t baud_rate; /**< 波特率 */
        } uart;
        struct {
            uint8_t pin;        /**< 单总线引脚 */
        } onewire;
        void* custom;           /**< 自定义接口配置 */
    } config;
} sensor_interface_config_t;

/* 传感器配置 */
typedef struct {
    sensor_type_t type;                /**< 传感器类型 */
    sensor_interface_config_t interface;/**< 接口配置 */
    sensor_mode_t mode;                /**< 采样模式 */
    sensor_rate_t rate;                /**< 采样率 */
    uint32_t custom_rate_hz;           /**< 自定义采样率(Hz) */
    sensor_accuracy_t accuracy;        /**< 精度 */
    sensor_filter_t filter;            /**< 过滤模式 */
    uint8_t filter_depth;              /**< 过滤深度 */
    int16_t threshold;                 /**< 阈值 */
    uint16_t hysteresis;               /**< 滞后 */
    bool low_power;                    /**< 是否低功耗模式 */
    const char* model;                 /**< 传感器型号 */
    void* driver_config;               /**< 驱动特定配置 */
} sensor_config_t;

/* 传感器值 */
typedef struct {
    sensor_value_type_t type;    /**< 值类型 */
    union {
        int32_t int_val;         /**< 整数值 */
        float float_val;         /**< 浮点值 */
        struct {
            uint8_t* data;       /**< 原始数据 */
            uint16_t len;        /**< 数据长度 */
        } raw;
        bool bool_val;           /**< 布尔值 */
        char string_val[32];     /**< 字符串值 */
        float vector_val[3];     /**< 向量值(x,y,z) */
        float matrix_val[3][3];  /**< 矩阵值 */
        void* custom_val;        /**< 自定义值 */
    } value;
    uint32_t timestamp;          /**< 时间戳(毫秒) */
    uint8_t accuracy;            /**< 准确性(0-100) */
} sensor_value_t;

/* 传感器事件类型 */
typedef enum {
    SENSOR_EVENT_DATA_READY,     /**< 数据就绪 */
    SENSOR_EVENT_THRESHOLD_HIGH, /**< 高于阈值 */
    SENSOR_EVENT_THRESHOLD_LOW,  /**< 低于阈值 */
    SENSOR_EVENT_ERROR,          /**< 错误事件 */
    SENSOR_EVENT_CUSTOM          /**< 自定义事件 */
} sensor_event_type_t;

/* 传感器事件 */
typedef struct {
    sensor_event_type_t type;    /**< 事件类型 */
    sensor_value_t value;        /**< 传感器值 */
    void* event_data;            /**< 事件相关数据 */
} sensor_event_t;

/* 传感器句柄 */
typedef void* sensor_handle_t;

/* 传感器回调函数 */
typedef void (*sensor_callback_t)(sensor_handle_t handle, sensor_event_t* event, void* user_data);

/**
 * @brief 初始化传感器
 * 
 * @param config 传感器配置
 * @param callback 回调函数
 * @param user_data 用户数据
 * @param handle 返回的传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_init(const sensor_config_t* config, sensor_callback_t callback, void* user_data, sensor_handle_t* handle);

/**
 * @brief 反初始化传感器
 * 
 * @param handle 传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_deinit(sensor_handle_t handle);

/**
 * @brief 启动传感器
 * 
 * @param handle 传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_start(sensor_handle_t handle);

/**
 * @brief 停止传感器
 * 
 * @param handle 传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_stop(sensor_handle_t handle);

/**
 * @brief 读取传感器值
 * 
 * @param handle 传感器句柄
 * @param value 返回的传感器值
 * @return int 成功返回0，失败返回错误码
 */
int sensor_read(sensor_handle_t handle, sensor_value_t* value);

/**
 * @brief 设置传感器采样率
 * 
 * @param handle 传感器句柄
 * @param rate 采样率
 * @param custom_rate_hz 自定义采样率(Hz)
 * @return int 成功返回0，失败返回错误码
 */
int sensor_set_rate(sensor_handle_t handle, sensor_rate_t rate, uint32_t custom_rate_hz);

/**
 * @brief 设置传感器模式
 * 
 * @param handle 传感器句柄
 * @param mode 模式
 * @return int 成功返回0，失败返回错误码
 */
int sensor_set_mode(sensor_handle_t handle, sensor_mode_t mode);

/**
 * @brief 设置传感器阈值
 * 
 * @param handle 传感器句柄
 * @param threshold 阈值
 * @param hysteresis 滞后
 * @return int 成功返回0，失败返回错误码
 */
int sensor_set_threshold(sensor_handle_t handle, int16_t threshold, uint16_t hysteresis);

/**
 * @brief 设置传感器过滤器
 * 
 * @param handle 传感器句柄
 * @param filter 过滤器类型
 * @param depth 过滤深度
 * @return int 成功返回0，失败返回错误码
 */
int sensor_set_filter(sensor_handle_t handle, sensor_filter_t filter, uint8_t depth);

/**
 * @brief 获取传感器状态
 * 
 * @param handle 传感器句柄
 * @param state 返回的状态
 * @return int 成功返回0，失败返回错误码
 */
int sensor_get_state(sensor_handle_t handle, sensor_state_t* state);

/**
 * @brief 校准传感器
 * 
 * @param handle 传感器句柄
 * @param params 校准参数，NULL表示使用默认参数
 * @return int 成功返回0，失败返回错误码
 */
int sensor_calibrate(sensor_handle_t handle, void* params);

/**
 * @brief 自检传感器
 * 
 * @param handle 传感器句柄
 * @param result 自检结果，0表示通过
 * @return int 成功返回0，失败返回错误码
 */
int sensor_self_test(sensor_handle_t handle, uint32_t* result);

/**
 * @brief 进入低功耗模式
 * 
 * @param handle 传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_enter_low_power(sensor_handle_t handle);

/**
 * @brief 退出低功耗模式
 * 
 * @param handle 传感器句柄
 * @return int 成功返回0，失败返回错误码
 */
int sensor_exit_low_power(sensor_handle_t handle);

/**
 * @brief 执行传感器特定命令
 * 
 * @param handle 传感器句柄
 * @param cmd 命令
 * @param arg 参数
 * @return int 成功返回0，失败返回错误码
 */
int sensor_ioctl(sensor_handle_t handle, uint32_t cmd, void* arg);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_API_H */
