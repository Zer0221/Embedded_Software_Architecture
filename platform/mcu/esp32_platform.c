/**
 * @file esp32_platform.c
 * @brief ESP32平台相关的实现
 *
 * 该文件包含ESP32平台相关的实现，实现了平台抽象层定义的接口
 */

#include "platform_api.h"
#include "esp32_platform.h"

// ESP-IDF头文件
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief ESP32平台初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_init(void)
{
    /* 初始化ESP系统 */
    esp_system_init();
    
    /* 初始化高精度计时器 */
    esp_timer_init();
    
    return 0;
}

/**
 * @brief ESP32平台去初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_deinit(void)
{
    /* 去初始化高精度计时器 */
    esp_timer_deinit();
    
    return 0;
}

/**
 * @brief 获取ESP32平台信息
 * 
 * @param info 平台信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int platform_get_info(void *info)
{
    esp32_platform_info_t *platform_info = (esp32_platform_info_t *)info;
    
    if (platform_info == NULL) {
        return -1;
    }
    
    /* 填充平台信息 */
    platform_info->cpu_type = ESP32_CPU_TYPE;
    platform_info->cpu_clock = esp_clk_cpu_freq();
    platform_info->flash_size = ESP32_FLASH_SIZE;
    platform_info->ram_size = ESP32_RAM_SIZE;
    
    return 0;
}

/**
 * @brief ESP32平台延时函数（毫秒）
 * 
 * @param ms 延时毫秒数
 */
void platform_delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

/**
 * @brief ESP32平台延时函数（微秒）
 * 
 * @param us 延时微秒数
 */
void platform_delay_us(uint32_t us)
{
    /* ESP32提供了微秒级延时函数 */
    ets_delay_us(us);
}

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t platform_get_time_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/**
 * @brief ESP32系统初始化
 * 
 * @note 这是一个ESP32平台特定的函数，不是平台抽象层的一部分
 */
void esp_system_init(void)
{
    /* 此处可以添加ESP32特定的初始化代码 */
    /* 例如Wi-Fi、蓝牙等外设的初始化 */
}
