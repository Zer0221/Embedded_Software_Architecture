/**
 * @file esp32_adc_pwm_example.c
 * @brief ESP32平台ADC和PWM驱动示例
 *
 * 该示例演示了如何使用ADC读取传感器数据，并使用PWM控制LED亮度
 */

#include "base/adc_api.h"
#include "base/pwm_api.h"
#include "base/gpio_api.h"
#include "common/rtos_api.h"
#include "base/uart_api.h"
#include "common/error_api.h"
#include <stdio.h>
#include <string.h>

/* 任务堆栈大小 */
#define STACK_SIZE     (4096)

/* 任务优先级 */
#define TASK_PRIORITY_ADC    (3)
#define TASK_PRIORITY_PWM    (2)

/* LED GPIO配置 */
#define LED_PORT            GPIO_PORT_A
#define LED_PIN             GPIO_PIN_2

/* ADC通道配置 */
#define ADC_CHANNEL_SENSOR  ADC_CHANNEL_0

/* PWM通道配置 */
#define PWM_CHANNEL_LED     PWM_CHANNEL_0

/* 任务句柄 */
static rtos_task_t adc_task_handle;
static rtos_task_t pwm_task_handle;

/* 设备句柄 */
static adc_handle_t adc_handle;
static pwm_handle_t pwm_handle;
static uart_handle_t uart_handle;

/* 数据共享互斥锁 */
static rtos_mutex_t data_mutex;

/* 全局数据 */
static volatile float g_sensor_voltage = 0.0f;
static volatile float g_pwm_duty = 0.5f;

/**
 * @brief ADC回调函数
 * 
 * @param value 转换结果
 * @param user_data 用户数据
 */
static void adc_callback(uint32_t value, void *user_data)
{
    float voltage;
    
    /* 将ADC值转换为电压 */
    adc_convert_to_voltage(adc_handle, value, &voltage);
    
    /* 更新全局电压值 */
    rtos_mutex_lock(data_mutex, RTOS_WAIT_FOREVER);
    g_sensor_voltage = voltage;
    rtos_mutex_unlock(data_mutex);
}

/**
 * @brief PWM周期事件回调函数
 * 
 * @param user_data 用户数据
 */
static void pwm_period_callback(void *user_data)
{
    /* 周期事件回调，可以在这里添加PWM周期相关的处理 */
}

/**
 * @brief ADC任务函数
 * 
 * @param arg 任务参数
 */
static void adc_task(void *arg)
{
    adc_config_t adc_config;
    uint32_t adc_value;
    float voltage;
    char str_buf[64];
    
    /* 初始化ADC */
    memset(&adc_config, 0, sizeof(adc_config));
    adc_config.channel = ADC_CHANNEL_SENSOR;
    adc_config.resolution = ADC_RESOLUTION_12BIT;
    adc_config.reference = ADC_REFERENCE_VDDA;
    adc_config.sample_rate = ADC_SAMPLE_RATE_MEDIUM;
    adc_config.reference_voltage = 3.3f;
    
    if (adc_init(&adc_config, &adc_handle) != DRIVER_OK) {
        printf("ADC初始化失败\n");
        return;
    }
    
    /* 启动连续ADC转换 */
    adc_start_continuous(adc_handle, adc_callback, NULL);
    
    while (1) {
        /* 读取ADC值 */
        if (adc_read(adc_handle, &adc_value) == DRIVER_OK) {
            /* 转换为电压 */
            adc_convert_to_voltage(adc_handle, adc_value, &voltage);
            
            /* 获取当前电压值 */
            rtos_mutex_lock(data_mutex, RTOS_WAIT_FOREVER);
            float current_voltage = g_sensor_voltage;
            rtos_mutex_unlock(data_mutex);
            
            /* 计算新的PWM占空比 (将电压映射到0.1-0.9范围) */
            float new_duty = 0.1f + (current_voltage / 3.3f) * 0.8f;
            
            /* 更新PWM占空比 */
            rtos_mutex_lock(data_mutex, RTOS_WAIT_FOREVER);
            g_pwm_duty = new_duty;
            rtos_mutex_unlock(data_mutex);
            
            /* 将数据通过UART发送 */
            snprintf(str_buf, sizeof(str_buf), 
                    "ADC: %lu, 电压: %.2fV, 占空比: %.2f%%\r\n", 
                    adc_value, current_voltage, new_duty * 100.0f);
            uart_write(uart_handle, (const uint8_t *)str_buf, strlen(str_buf), 100);
        }
        
        /* 等待100ms */
        rtos_delay(100);
    }
}

/**
 * @brief PWM任务函数
 * 
 * @param arg 任务参数
 */
static void pwm_task(void *arg)
{
    pwm_config_t pwm_config;
    gpio_config_t gpio_config;
    uint32_t gpio_pin_mask;
    
    /* 初始化GPIO */
    memset(&gpio_config, 0, sizeof(gpio_config));
    gpio_config.mode = GPIO_MODE_OUTPUT_PP;
    gpio_config.pull = GPIO_PULL_NONE;
    gpio_config.speed = GPIO_SPEED_HIGH;
    
    gpio_pin_mask = (1 << LED_PIN);
    gpio_init(LED_PORT, gpio_pin_mask, &gpio_config);
    
    /* 初始化PWM */
    memset(&pwm_config, 0, sizeof(pwm_config));
    pwm_config.channel = PWM_CHANNEL_LED;
    pwm_config.frequency = 1000;  /* 1kHz */
    pwm_config.duty_cycle = 0.5f; /* 50% */
    pwm_config.align_mode = PWM_ALIGN_EDGE;
    pwm_config.polarity = PWM_POLARITY_NORMAL;
    pwm_config.counter_mode = PWM_COUNTER_UP;
    
    if (pwm_init(&pwm_config, &pwm_handle) != DRIVER_OK) {
        printf("PWM初始化失败\n");
        return;
    }
    
    /* 注册PWM周期回调 */
    pwm_register_callback(pwm_handle, PWM_EVENT_PERIOD_ELAPSED, 
                        pwm_period_callback, NULL);
    
    /* 启动PWM */
    pwm_start(pwm_handle);
    
    while (1) {
        /* 获取当前占空比 */
        rtos_mutex_lock(data_mutex, RTOS_WAIT_FOREVER);
        float current_duty = g_pwm_duty;
        rtos_mutex_unlock(data_mutex);
        
        /* 更新PWM占空比 */
        pwm_set_duty_cycle(pwm_handle, current_duty);
        
        /* 等待50ms */
        rtos_delay(50);
    }
}

/**
 * @brief 应用程序入口
 * 
 * @return int 
 */
int main(void)
{
    uart_config_t uart_config;
    
    /* 初始化UART */
    memset(&uart_config, 0, sizeof(uart_config));
    uart_config.channel = UART_CHANNEL_0;
    uart_config.baudrate = 115200;
    uart_config.data_bits = UART_DATA_BITS_8;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.parity = UART_PARITY_NONE;
    uart_config.mode = UART_MODE_TX_RX;
    uart_config.flow_ctrl = UART_FLOW_CTRL_NONE;
    
    if (uart_init(&uart_config, &uart_handle) != DRIVER_OK) {
        return -1;
    }
    
    /* 发送欢迎信息 */
    const char *welcome = "\r\n===== ESP32 ADC & PWM示例 =====\r\n";
    uart_write(uart_handle, (const uint8_t *)welcome, strlen(welcome), 100);
    
    /* 创建互斥锁 */
    data_mutex = rtos_mutex_create();
    if (data_mutex == NULL) {
        return -1;
    }
    
    /* 创建ADC任务 */
    if (rtos_task_create(&adc_task, "adc_task", STACK_SIZE, NULL, 
                        TASK_PRIORITY_ADC, &adc_task_handle) != RTOS_OK) {
        return -1;
    }
    
    /* 创建PWM任务 */
    if (rtos_task_create(&pwm_task, "pwm_task", STACK_SIZE, NULL, 
                        TASK_PRIORITY_PWM, &pwm_task_handle) != RTOS_OK) {
        return -1;
    }
    
    /* 启动调度器 */
    rtos_start_scheduler();
    
    /* 不应该到达这里 */
    return 0;
}
