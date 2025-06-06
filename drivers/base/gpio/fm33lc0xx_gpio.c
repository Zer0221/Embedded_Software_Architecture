/**
 * @file fm33lc0xx_gpio.c
 * @brief FM33LC0xx平台GPIO驱动实现
 *
 * 该文件实现了FM33LC0xx平台的GPIO驱动接口
 */

#include "base/gpio_api.h"
#include "common/error_api.h"
#include "fm33lc0xx_fl.h"
#include <string.h>

/* GPIO端口映射 */
static GPIO_Type* const GPIO_PORT_MAP[] = {
    GPIOA, /* GPIO_PORT_A */
    GPIOB, /* GPIO_PORT_B */
    GPIOC, /* GPIO_PORT_C */
    GPIOD, /* GPIO_PORT_D */
    NULL,  /* GPIO_PORT_E - FM33LC0xx没有GPIOE */
    NULL,  /* GPIO_PORT_F - FM33LC0xx没有GPIOF */
    NULL,  /* GPIO_PORT_G - FM33LC0xx没有GPIOG */
    NULL,  /* GPIO_PORT_H - FM33LC0xx没有GPIOH */
    NULL   /* GPIO_PORT_I - FM33LC0xx没有GPIOI */
};

/**
 * @brief GPIO模式转换为FM33LC0xx模式
 * 
 * @param mode GPIO模式
 * @return uint32_t FM33LC0xx GPIO模式
 */
static uint32_t convert_gpio_mode(gpio_mode_t mode)
{
    switch (mode) {
        case GPIO_MODE_INPUT:
            return FL_GPIO_MODE_INPUT;
        case GPIO_MODE_OUTPUT_PP:
            return FL_GPIO_MODE_OUTPUT;
        case GPIO_MODE_OUTPUT_OD:
            return FL_GPIO_MODE_OPEN_DRAIN_OUTPUT;
        case GPIO_MODE_AF_PP:
            return FL_GPIO_MODE_DIGITAL;
        case GPIO_MODE_AF_OD:
            return FL_GPIO_MODE_DIGITAL;
        case GPIO_MODE_ANALOG:
            return FL_GPIO_MODE_ANALOG;
        case GPIO_MODE_IT_RISING:
            return FL_GPIO_MODE_INPUT;
        case GPIO_MODE_IT_FALLING:
            return FL_GPIO_MODE_INPUT;
        case GPIO_MODE_IT_RISING_FALLING:
            return FL_GPIO_MODE_INPUT;
        default:
            return FL_GPIO_MODE_INPUT;
    }
}

/**
 * @brief GPIO上拉/下拉转换为FM33LC0xx上拉/下拉
 * 
 * @param pull GPIO上拉/下拉
 * @return uint32_t FM33LC0xx GPIO上拉/下拉
 */
static uint32_t convert_gpio_pull(gpio_pull_t pull)
{
    switch (pull) {
        case GPIO_PULL_NONE:
            return FL_GPIO_PULL_DOWN; /* FM33LC0xx不支持无上拉/下拉，默认使用下�?*/
        case GPIO_PULL_UP:
            return FL_GPIO_PULL_UP;
        case GPIO_PULL_DOWN:
            return FL_GPIO_PULL_DOWN;
        default:
            return FL_GPIO_PULL_DOWN;
    }
}

/**
 * @brief GPIO中断触发方式转换为FM33LC0xx中断触发方式
 * 
 * @param mode GPIO模式
 * @return uint32_t FM33LC0xx GPIO中断触发方式
 */
static uint32_t convert_gpio_trigger(gpio_mode_t mode)
{
    switch (mode) {
        case GPIO_MODE_IT_RISING:
            return FL_GPIO_EXTI_TRIGGER_EDGE_RISING;
        case GPIO_MODE_IT_FALLING:
            return FL_GPIO_EXTI_TRIGGER_EDGE_FALLING;
        case GPIO_MODE_IT_RISING_FALLING:
            return FL_GPIO_EXTI_TRIGGER_EDGE_BOTH;
        default:
            return FL_GPIO_EXTI_TRIGGER_EDGE_RISING;
    }
}

/**
 * @brief 初始化GPIO
 * 
 * @param port GPIO端口
 * @param pin_mask 引脚掩码
 * @param config GPIO配置
 * @return int 0表示成功，非0表示失败
 */
int gpio_init(gpio_port_t port, uint16_t pin_mask, const gpio_config_t *config)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    uint8_t pin;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX || config == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 配置每个引脚 */
    for (pin = 0; pin < 16; pin++) {
        pin_bit = 1 << pin;
        
        /* 检查引脚是否需要配�?*/
        if (!(pin_mask & pin_bit)) {
            continue;
        }
        
        /* 配置引脚模式 */
        FL_GPIO_SetPinMode(gpio_port, pin_bit, convert_gpio_mode(config->mode));
        
        /* 配置引脚上拉/下拉 */
        FL_GPIO_SetPinPull(gpio_port, pin_bit, convert_gpio_pull(config->pull));
        
        /* 配置输出速度 - FM33LC0xx不支持输出速度配置 */
        
        /* 配置中断 */
        if (config->mode >= GPIO_MODE_IT_RISING && config->mode <= GPIO_MODE_IT_RISING_FALLING) {
            /* 配置中断触发方式 */
            FL_GPIO_SetExtiTrigger(gpio_port, pin_bit, convert_gpio_trigger(config->mode));
            
            /* 清除中断标志 */
            FL_GPIO_ClearFlag_EXTI(gpio_port, pin_bit);
            
            /* 使能中断 */
            FL_GPIO_EnableExtiIT(gpio_port, pin_bit);
            
            /* 使能NVIC中断 */
            switch (port) {
                case GPIO_PORT_A:
                    NVIC_EnableIRQ(GPIOA_IRQn);
                    break;
                case GPIO_PORT_B:
                    NVIC_EnableIRQ(GPIOB_IRQn);
                    break;
                case GPIO_PORT_C:
                    NVIC_EnableIRQ(GPIOC_IRQn);
                    break;
                case GPIO_PORT_D:
                    NVIC_EnableIRQ(GPIOD_IRQn);
                    break;
                default:
                    break;
            }
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化GPIO
 * 
 * @param port GPIO端口
 * @param pin_mask 引脚掩码
 * @return int 0表示成功，非0表示失败
 */
int gpio_deinit(gpio_port_t port, uint16_t pin_mask)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    uint8_t pin;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 复位每个引脚 */
    for (pin = 0; pin < 16; pin++) {
        pin_bit = 1 << pin;
        
        /* 检查引脚是否需要复�?*/
        if (!(pin_mask & pin_bit)) {
            continue;
        }
        
        /* 禁用中断 */
        FL_GPIO_DisableExtiIT(gpio_port, pin_bit);
        
        /* 清除中断标志 */
        FL_GPIO_ClearFlag_EXTI(gpio_port, pin_bit);
        
        /* 复位引脚配置 */
        FL_GPIO_SetPinMode(gpio_port, pin_bit, FL_GPIO_MODE_INPUT);
        FL_GPIO_SetPinPull(gpio_port, pin_bit, FL_GPIO_PULL_DOWN);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 配置GPIO引脚复用功能
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @param af 复用功能
 * @return int 0表示成功，非0表示失败
 */
int gpio_set_af(gpio_port_t port, gpio_pin_t pin, gpio_af_t af)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX || pin >= GPIO_PIN_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算引脚掩码 */
    pin_bit = 1 << pin;
    
    /* FM33LC0xx不支持直接设置复用功能，需要根据具体外设和引脚配置 */
    /* 这里仅将引脚模式设置为数字模式，具体的复用功能由外设驱动配置 */
    FL_GPIO_SetPinMode(gpio_port, pin_bit, FL_GPIO_MODE_DIGITAL);
    
    return DRIVER_OK;
}

/**
 * @brief 设置GPIO引脚状�?
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @param state 引脚状�?
 * @return int 0表示成功，非0表示失败
 */
int gpio_write(gpio_port_t port, gpio_pin_t pin, gpio_state_t state)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX || pin >= GPIO_PIN_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算引脚掩码 */
    pin_bit = 1 << pin;
    
    /* 设置引脚状�?*/
    if (state == GPIO_PIN_SET) {
        FL_GPIO_SetOutputPin(gpio_port, pin_bit);
    } else {
        FL_GPIO_ResetOutputPin(gpio_port, pin_bit);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 读取GPIO引脚状�?
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @param state 引脚状态指�?
 * @return int 0表示成功，非0表示失败
 */
int gpio_read(gpio_port_t port, gpio_pin_t pin, gpio_state_t *state)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX || pin >= GPIO_PIN_MAX || state == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算引脚掩码 */
    pin_bit = 1 << pin;
    
    /* 读取引脚状�?*/
    if (FL_GPIO_GetInputPin(gpio_port, pin_bit) != 0) {
        *state = GPIO_PIN_SET;
    } else {
        *state = GPIO_PIN_RESET;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 切换GPIO引脚状�?
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_toggle(gpio_port_t port, gpio_pin_t pin)
{
    GPIO_Type *gpio_port;
    uint16_t pin_bit;
    
    /* 参数检�?*/
    if (port >= GPIO_PORT_MAX || pin >= GPIO_PIN_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取GPIO端口 */
    gpio_port = GPIO_PORT_MAP[port];
    if (gpio_port == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算引脚掩码 */
    pin_bit = 1 << pin;
    
    /* 切换引脚状�?*/
    FL_GPIO_ToggleOutputPin(gpio_port, pin_bit);
    
    return DRIVER_OK;
}

/**
 * @brief 锁定GPIO引脚配置
 * 
 * @param port GPIO端口
 * @param pin_mask 引脚掩码
 * @return int 0表示成功，非0表示失败
 */
int gpio_lock(gpio_port_t port, uint16_t pin_mask)
{
    /* FM33LC0xx不支持锁定GPIO配置 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 注册GPIO中断回调函数
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @param callback 回调函数
 * @param arg 回调函数参数
 * @return int 0表示成功，非0表示失败
 */
int gpio_register_callback(gpio_port_t port, gpio_pin_t pin, gpio_callback_t callback, void *arg)
{
    /* FM33LC0xx不支持单独为每个引脚注册回调函数，需要在中断处理函数中处�?*/
    /* 实际应用中，可以实现一个回调函数数组来保存每个引脚的回调函�?*/
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 取消注册GPIO中断回调函数
 * 
 * @param port GPIO端口
 * @param pin 引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_unregister_callback(gpio_port_t port, gpio_pin_t pin)
{
    /* FM33LC0xx不支持单独为每个引脚注册回调函数 */
    return ERROR_NOT_SUPPORTED;
}

