/**
 * @file sensor_app.c
 * @brief 传感器应用示例
 *
 * 该文件实现了一个传感器数据采集和处理的示例应用
 */

#include "common/app_framework.h"
#include "base/adc_api.h"
#include "base/i2c_api.h"
#include "base/power_api.h"
#include "base/gpio_api.h"
#include "common/error_api.h"
#include <stdio.h>
#include <string.h>

/* 传感器数据采集间隔（毫秒） */
#define SENSOR_SAMPLE_INTERVAL_MS  1000

/* 传感器类型 */
typedef enum {
    SENSOR_TYPE_TEMPERATURE = 0,
    SENSOR_TYPE_HUMIDITY,
    SENSOR_TYPE_PRESSURE,
    SENSOR_TYPE_LIGHT,
    SENSOR_TYPE_MAX
} sensor_type_t;

/* 传感器数据结构体 */
typedef struct {
    sensor_type_t type;
    float value;
    uint32_t timestamp;
    bool valid;
} sensor_data_t;

/* 应用状态 */
static app_state_t g_app_state = APP_STATE_UNINITIALIZED;

/* 句柄 */
static adc_handle_t g_adc_handle;
static i2c_handle_t g_i2c_handle;
static power_handle_t g_power_handle;
static gpio_handle_t g_gpio_handle;

/* 传感器数据 */
static sensor_data_t g_sensor_data[SENSOR_TYPE_MAX];

/* 采样任务 */
#if (CURRENT_RTOS != RTOS_NONE)
static rtos_task_t g_sample_task;
static bool g_sampling_enabled = false;
#endif

/**
 * @brief 传感器采样任务
 * 
 * @param arg 任务参数
 */
#if (CURRENT_RTOS != RTOS_NONE)
static void sensor_sample_task(void *arg)
{
    uint32_t adc_value;
    uint8_t i2c_data[4];
    uint32_t timestamp;
    
    while (g_sampling_enabled) {
        /* 获取时间戳 */
        timestamp = platform_get_time_ms();
        
        /* 采集温度数据（通过ADC） */
        if (adc_read(g_adc_handle, 0, &adc_value) == 0) {
            /* 将ADC值转换为温度 */
            g_sensor_data[SENSOR_TYPE_TEMPERATURE].value = (float)adc_value * 0.1f - 50.0f;
            g_sensor_data[SENSOR_TYPE_TEMPERATURE].timestamp = timestamp;
            g_sensor_data[SENSOR_TYPE_TEMPERATURE].valid = true;
        }
        
        /* 采集湿度和压力数据（通过I2C） */
        if (i2c_read(g_i2c_handle, 0x40, 0x00, i2c_data, 4) == 0) {
            /* 解析湿度数据 */
            g_sensor_data[SENSOR_TYPE_HUMIDITY].value = 
                (float)((i2c_data[0] << 8) | i2c_data[1]) * 0.1f;
            g_sensor_data[SENSOR_TYPE_HUMIDITY].timestamp = timestamp;
            g_sensor_data[SENSOR_TYPE_HUMIDITY].valid = true;
            
            /* 解析压力数据 */
            g_sensor_data[SENSOR_TYPE_PRESSURE].value = 
                (float)((i2c_data[2] << 8) | i2c_data[3]) * 0.1f;
            g_sensor_data[SENSOR_TYPE_PRESSURE].timestamp = timestamp;
            g_sensor_data[SENSOR_TYPE_PRESSURE].valid = true;
        }
        
        /* 采集光照数据（通过I2C） */
        if (i2c_read(g_i2c_handle, 0x23, 0x00, i2c_data, 2) == 0) {
            /* 解析光照数据 */
            g_sensor_data[SENSOR_TYPE_LIGHT].value = 
                (float)((i2c_data[0] << 8) | i2c_data[1]) * 1.2f;
            g_sensor_data[SENSOR_TYPE_LIGHT].timestamp = timestamp;
            g_sensor_data[SENSOR_TYPE_LIGHT].valid = true;
        }
        
        /* 打印数据 */
        printf("Sensor Data:\n");
        for (int i = 0; i < SENSOR_TYPE_MAX; i++) {
            if (g_sensor_data[i].valid) {
                printf("  Type: %d, Value: %.2f, Time: %u\n", 
                       g_sensor_data[i].type, g_sensor_data[i].value, 
                       g_sensor_data[i].timestamp);
            }
        }
        
        /* 延时指定时间 */
        rtos_task_delay(SENSOR_SAMPLE_INTERVAL_MS);
    }
    
    /* 任务结束 */
    rtos_task_delete(NULL);
}
#endif

/**
 * @brief 传感器应用消息处理函数
 * 
 * @param msg 消息
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
static int sensor_message_handler(app_message_t *msg, void *user_data)
{
    if (msg == NULL) {
        return -1;
    }
    
    /* 处理消息 */
    switch (msg->msg_id) {
        case 0x1001:  /* 获取传感器数据 */
            if (msg->data != NULL && msg->data_len >= sizeof(sensor_data_t) * SENSOR_TYPE_MAX) {
                memcpy(msg->data, g_sensor_data, sizeof(sensor_data_t) * SENSOR_TYPE_MAX);
            }
            break;
            
        case 0x1002:  /* 重置传感器 */
            /* 重置传感器数据 */
            memset(g_sensor_data, 0, sizeof(g_sensor_data));
            for (int i = 0; i < SENSOR_TYPE_MAX; i++) {
                g_sensor_data[i].type = (sensor_type_t)i;
                g_sensor_data[i].valid = false;
            }
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

/**
 * @brief 传感器应用初始化函数
 * 
 * @param params 初始化参数
 * @return int 0表示成功，非0表示失败
 */
static int sensor_app_init(void *params)
{
    adc_config_t adc_config;
    i2c_config_t i2c_config;
    power_config_t power_config;
    gpio_config_t gpio_config;
    
    /* 初始化ADC */
    memset(&adc_config, 0, sizeof(adc_config));
    adc_config.resolution = ADC_RESOLUTION_12BIT;
    adc_config.reference = ADC_REFERENCE_INTERNAL;
    if (adc_init(&adc_config, &g_adc_handle) != 0) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 初始化I2C */
    memset(&i2c_config, 0, sizeof(i2c_config));
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.speed = I2C_SPEED_STANDARD;
    i2c_config.scl_pin = 22;  /* 根据实际硬件配置 */
    i2c_config.sda_pin = 21;  /* 根据实际硬件配置 */
    if (i2c_init(&i2c_config, &g_i2c_handle) != 0) {
        adc_deinit(g_adc_handle);
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 初始化电源管理 */
    memset(&power_config, 0, sizeof(power_config));
    power_config.enable_auto_sleep = false;
    power_config.enable_battery_monitor = true;
    power_config.battery_monitor_interval_ms = 60000;
    power_config.battery_type = BATTERY_TYPE_LIPO;
    if (power_init(&power_config, &g_power_handle) != 0) {
        i2c_deinit(g_i2c_handle);
        adc_deinit(g_adc_handle);
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 初始化GPIO */
    memset(&gpio_config, 0, sizeof(gpio_config));
    /* 配置LED引脚为输出 */
    gpio_config.mode = GPIO_MODE_OUTPUT;
    gpio_config.pull = GPIO_PULL_NONE;
    gpio_config.pin = 2;  /* 根据实际硬件配置 */
    if (gpio_init(&gpio_config, &g_gpio_handle) != 0) {
        power_deinit(g_power_handle);
        i2c_deinit(g_i2c_handle);
        adc_deinit(g_adc_handle);
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 初始化传感器数据 */
    memset(g_sensor_data, 0, sizeof(g_sensor_data));
    for (int i = 0; i < SENSOR_TYPE_MAX; i++) {
        g_sensor_data[i].type = (sensor_type_t)i;
        g_sensor_data[i].valid = false;
    }
    
    /* 更新应用状态 */
    g_app_state = APP_STATE_INITIALIZED;
    
    return 0;
}

/**
 * @brief 传感器应用启动函数
 * 
 * @return int 0表示成功，非0表示失败
 */
static int sensor_app_start(void)
{
    if (g_app_state != APP_STATE_INITIALIZED) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_STATE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 创建采样任务 */
    g_sampling_enabled = true;
    if (rtos_task_create(&g_sample_task, "SensorSample", 
                       sensor_sample_task, NULL, 
                       configMINIMAL_STACK_SIZE * 2, 
                       RTOS_TASK_PRIORITY_NORMAL) != 0) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
#endif
    
    /* 更新应用状态 */
    g_app_state = APP_STATE_RUNNING;
    
    return 0;
}

/**
 * @brief 传感器应用停止函数
 * 
 * @return int 0表示成功，非0表示失败
 */
static int sensor_app_stop(void)
{
    if (g_app_state != APP_STATE_RUNNING) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_STATE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 停止采样任务 */
    g_sampling_enabled = false;
    /* 任务会自行退出 */
#endif
    
    /* 更新应用状态 */
    g_app_state = APP_STATE_STOPPED;
    
    return 0;
}

/**
 * @brief 传感器应用去初始化函数
 * 
 * @return int 0表示成功，非0表示失败
 */
static int sensor_app_deinit(void)
{
    if (g_app_state != APP_STATE_STOPPED && g_app_state != APP_STATE_INITIALIZED) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_STATE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 去初始化外设 */
    gpio_deinit(g_gpio_handle);
    power_deinit(g_power_handle);
    i2c_deinit(g_i2c_handle);
    adc_deinit(g_adc_handle);
    
    /* 更新应用状态 */
    g_app_state = APP_STATE_UNINITIALIZED;
    
    return 0;
}

/* 传感器应用定义 */
static application_t g_sensor_app = {
    .name = "SensorApp",
    .priority = APP_PRIORITY_NORMAL,
    .state = APP_STATE_UNINITIALIZED,
    .init = sensor_app_init,
    .start = sensor_app_start,
    .pause = NULL,
    .resume = NULL,
    .stop = sensor_app_stop,
    .deinit = sensor_app_deinit,
    .msg_handler = sensor_message_handler,
    .user_data = NULL
};

/* 传感器应用注册函数 */
int sensor_app_register(void)
{
    return app_register(&g_sensor_app);
}
