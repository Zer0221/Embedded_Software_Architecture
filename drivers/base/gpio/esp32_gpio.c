/**
 * @file esp32_gpio.c
 * @brief ESP32平台的GPIO驱动实现
 *
 * 该文件实现了ESP32平台的GPIO驱动，符合GPIO抽象接口的规�?
 */

#include "base/gpio_api.h"
#include "base/platform_api.h"
#include "common/error_api.h"
#include "esp32_platform.h"

// ESP-IDF头文�?
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

/* GPIO内部结构�?*/
typedef struct {
    gpio_port_t port;             /* GPIO端口 */
    gpio_pin_t pin;               /* GPIO引脚 */
    gpio_mode_t mode;             /* GPIO模式 */
    bool initialized;             /* 初始化标�?*/
    gpio_irq_callback_t callback; /* 中断回调函数 */
    void *user_data;              /* 用户数据 */
} esp32_gpio_t;

/* ESP32实际GPIO引脚映射�?*/
static const int gpio_pin_map[GPIO_PORT_MAX][GPIO_PIN_MAX] = {
    /* PORT A */
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    /* PORT B */
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
    /* PORT C - 在ESP32上，我们仅映射部分引�?*/
    {32, 33, 34, 35, 36, 37, 38, 39, -1, -1, -1, -1, -1, -1, -1, -1}
};

/* GPIO中断处理函数数组 */
static esp32_gpio_t *gpio_irq_handlers[GPIO_PORT_MAX][GPIO_PIN_MAX] = {0};

/**
 * @brief 转换为ESP32 GPIO引脚编号
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return int ESP32 GPIO引脚编号�?1表示无效
 */
static int get_esp32_gpio_pin(gpio_port_t port, gpio_pin_t pin)
{
    if (port >= GPIO_PORT_MAX || pin >= GPIO_PIN_MAX) {
        return -1;
    }
    
    return gpio_pin_map[port][pin];
}

/**
 * @brief ESP32 GPIO中断处理函数
 * 
 * @param arg 用户参数
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    
    /* 查找对应的端口和引脚 */
    for (int port = 0; port < GPIO_PORT_MAX; port++) {
        for (int pin = 0; pin < GPIO_PIN_MAX; pin++) {
            if (gpio_pin_map[port][pin] == gpio_num) {
                /* 找到对应的处理器并调用回�?*/
                esp32_gpio_t *handler = gpio_irq_handlers[port][pin];
                if (handler != NULL && handler->callback != NULL) {
                    handler->callback((gpio_port_t)port, (gpio_pin_t)pin, handler->user_data);
                }
                return;
            }
        }
    }
}

/**
 * @brief 初始化GPIO
 * 
 * @param config GPIO配置参数
 * @param handle GPIO设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int gpio_init(const gpio_config_t *config, gpio_handle_t *handle)
{
    esp32_gpio_t *gpio_dev;
    int esp32_pin;
    gpio_config_t esp32_gpio_config = {0};
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(config->port, config->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 分配内部结构�?*/
    gpio_dev = (esp32_gpio_t *)malloc(sizeof(esp32_gpio_t));
    if (gpio_dev == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 初始化内部结构体 */
    gpio_dev->port = config->port;
    gpio_dev->pin = config->pin;
    gpio_dev->mode = config->mode;
    gpio_dev->initialized = true;
    gpio_dev->callback = NULL;
    gpio_dev->user_data = NULL;
    
    /* 配置ESP32 GPIO */
    switch (config->mode) {
        case GPIO_MODE_INPUT:
            esp32_gpio_config.mode = GPIO_MODE_INPUT;
            break;
        case GPIO_MODE_OUTPUT_PP:
        case GPIO_MODE_OUTPUT_OD:
            esp32_gpio_config.mode = GPIO_MODE_OUTPUT;
            break;
        case GPIO_MODE_IT_RISING:
            esp32_gpio_config.mode = GPIO_MODE_INPUT;
            esp32_gpio_config.intr_type = GPIO_INTR_POSEDGE;
            break;
        case GPIO_MODE_IT_FALLING:
            esp32_gpio_config.mode = GPIO_MODE_INPUT;
            esp32_gpio_config.intr_type = GPIO_INTR_NEGEDGE;
            break;
        case GPIO_MODE_IT_RISING_FALLING:
            esp32_gpio_config.mode = GPIO_MODE_INPUT;
            esp32_gpio_config.intr_type = GPIO_INTR_ANYEDGE;
            break;
        default:
            esp32_gpio_config.mode = GPIO_MODE_INPUT;
            break;
    }
    
    /* 设置上拉/下拉 */
    switch (config->pull) {
        case GPIO_PULL_UP:
            esp32_gpio_config.pull_up_en = 1;
            break;
        case GPIO_PULL_DOWN:
            esp32_gpio_config.pull_down_en = 1;
            break;
        case GPIO_PULL_NONE:
        default:
            esp32_gpio_config.pull_up_en = 0;
            esp32_gpio_config.pull_down_en = 0;
            break;
    }
    
    /* 设置开漏模�?*/
    if (config->mode == GPIO_MODE_OUTPUT_OD) {
        esp32_gpio_config.mode = GPIO_MODE_OUTPUT_OD;
    }
    
    /* 设置GPIO位掩�?*/
    esp32_gpio_config.pin_bit_mask = (1ULL << esp32_pin);
    
    /* 配置GPIO */
    if (gpio_config(&esp32_gpio_config) != ESP_OK) {
        free(gpio_dev);
        return ERROR_HARDWARE;
    }
    
    /* 如果是中断模式，注册中断处理函数 */
    if (config->mode == GPIO_MODE_IT_RISING || 
        config->mode == GPIO_MODE_IT_FALLING || 
        config->mode == GPIO_MODE_IT_RISING_FALLING) {
        /* 安装GPIO中断服务 */
        gpio_install_isr_service(0);
        
        /* 保存处理�?*/
        gpio_irq_handlers[config->port][config->pin] = gpio_dev;
    }
    
    /* 返回句柄 */
    *handle = (gpio_handle_t)gpio_dev;
    
    return ERROR_NONE;
}

/**
 * @brief 去初始化GPIO
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_deinit(gpio_handle_t handle)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 如果是中断模式，移除中断处理函数 */
    if (gpio_dev->mode == GPIO_MODE_IT_RISING || 
        gpio_dev->mode == GPIO_MODE_IT_FALLING || 
        gpio_dev->mode == GPIO_MODE_IT_RISING_FALLING) {
        /* 移除GPIO ISR处理函数 */
        gpio_isr_handler_remove(esp32_pin);
        
        /* 清除处理�?*/
        gpio_irq_handlers[gpio_dev->port][gpio_dev->pin] = NULL;
    }
    
    /* 重置GPIO为输入模�?*/
    gpio_set_direction(esp32_pin, GPIO_MODE_INPUT);
    
    /* 释放内部结构�?*/
    gpio_dev->initialized = false;
    free(gpio_dev);
    
    return ERROR_NONE;
}

/**
 * @brief 设置GPIO引脚状�?
 * 
 * @param handle GPIO设备句柄
 * @param state GPIO状�?
 * @return int 0表示成功，非0表示失败
 */
int gpio_write(gpio_handle_t handle, gpio_state_t state)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 设置GPIO状�?*/
    gpio_set_level(esp32_pin, state == GPIO_PIN_SET ? 1 : 0);
    
    return ERROR_NONE;
}

/**
 * @brief 读取GPIO引脚状�?
 * 
 * @param handle GPIO设备句柄
 * @return gpio_state_t GPIO状�?
 */
gpio_state_t gpio_read(gpio_handle_t handle)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    int level;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return GPIO_PIN_RESET;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return GPIO_PIN_RESET;
    }
    
    /* 读取GPIO状�?*/
    level = gpio_get_level(esp32_pin);
    
    return level ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief 翻转GPIO引脚状�?
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_toggle(gpio_handle_t handle)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    int level;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 读取当前状态并翻转 */
    level = gpio_get_level(esp32_pin);
    gpio_set_level(esp32_pin, !level);
    
    return ERROR_NONE;
}

/**
 * @brief 注册GPIO中断回调函数
 * 
 * @param handle GPIO设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int gpio_register_irq_callback(gpio_handle_t handle, gpio_irq_callback_t callback, void *user_data)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized || callback == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否为中断模式 */
    if (gpio_dev->mode != GPIO_MODE_IT_RISING && 
        gpio_dev->mode != GPIO_MODE_IT_FALLING && 
        gpio_dev->mode != GPIO_MODE_IT_RISING_FALLING) {
        return ERROR_INVALID_MODE;
    }
    
    /* 注册回调函数 */
    gpio_dev->callback = callback;
    gpio_dev->user_data = user_data;
    
    /* 添加GPIO ISR处理函数 */
    gpio_isr_handler_add(esp32_pin, gpio_isr_handler, (void *)esp32_pin);
    
    return ERROR_NONE;
}

/**
 * @brief 使能GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_enable_irq(gpio_handle_t handle)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    gpio_int_type_t intr_type;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否为中断模式 */
    if (gpio_dev->mode != GPIO_MODE_IT_RISING && 
        gpio_dev->mode != GPIO_MODE_IT_FALLING && 
        gpio_dev->mode != GPIO_MODE_IT_RISING_FALLING) {
        return ERROR_INVALID_MODE;
    }
    
    /* 根据模式设置中断类型 */
    switch (gpio_dev->mode) {
        case GPIO_MODE_IT_RISING:
            intr_type = GPIO_INTR_POSEDGE;
            break;
        case GPIO_MODE_IT_FALLING:
            intr_type = GPIO_INTR_NEGEDGE;
            break;
        case GPIO_MODE_IT_RISING_FALLING:
            intr_type = GPIO_INTR_ANYEDGE;
            break;
        default:
            intr_type = GPIO_INTR_DISABLE;
            break;
    }
    
    /* 设置中断类型 */
    gpio_set_intr_type(esp32_pin, intr_type);
    
    return ERROR_NONE;
}

/**
 * @brief 禁用GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_disable_irq(gpio_handle_t handle)
{
    esp32_gpio_t *gpio_dev = (esp32_gpio_t *)handle;
    int esp32_pin;
    
    /* 参数检�?*/
    if (gpio_dev == NULL || !gpio_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取ESP32 GPIO引脚编号 */
    esp32_pin = get_esp32_gpio_pin(gpio_dev->port, gpio_dev->pin);
    if (esp32_pin < 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否为中断模式 */
    if (gpio_dev->mode != GPIO_MODE_IT_RISING && 
        gpio_dev->mode != GPIO_MODE_IT_FALLING && 
        gpio_dev->mode != GPIO_MODE_IT_RISING_FALLING) {
        return ERROR_INVALID_MODE;
    }
    
    /* 禁用中断 */
    gpio_set_intr_type(esp32_pin, GPIO_INTR_DISABLE);
    
    return ERROR_NONE;
}

