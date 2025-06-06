/**
 * @file gpio_api.h
 * @brief GPIO接口抽象层定义
 *
 * 该头文件定义了GPIO接口的统一抽象，使上层应用能够与底层GPIO硬件实现解耦
 */

#ifndef GPIO_API_H
#define GPIO_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* GPIO端口定义 */
typedef enum {
    GPIO_PORT_A = 0,    /**< 端口A */
    GPIO_PORT_B,        /**< 端口B */
    GPIO_PORT_C,        /**< 端口C */
    GPIO_PORT_D,        /**< 端口D */
    GPIO_PORT_E,        /**< 端口E */
    GPIO_PORT_F,        /**< 端口F */
    GPIO_PORT_G,        /**< 端口G */
    GPIO_PORT_H,        /**< 端口H */
    GPIO_PORT_I,        /**< 端口I */
    GPIO_PORT_MAX       /**< 最大端口数量 */
} gpio_port_t;

/* GPIO引脚定义 */
typedef enum {
    GPIO_PIN_0 = 0,      /**< 引脚0 */
    GPIO_PIN_1,          /**< 引脚1 */
    GPIO_PIN_2,          /**< 引脚2 */
    GPIO_PIN_3,          /**< 引脚3 */
    GPIO_PIN_4,          /**< 引脚4 */
    GPIO_PIN_5,          /**< 引脚5 */
    GPIO_PIN_6,          /**< 引脚6 */
    GPIO_PIN_7,          /**< 引脚7 */
    GPIO_PIN_8,          /**< 引脚8 */
    GPIO_PIN_9,          /**< 引脚9 */
    GPIO_PIN_10,         /**< 引脚10 */
    GPIO_PIN_11,         /**< 引脚11 */
    GPIO_PIN_12,         /**< 引脚12 */
    GPIO_PIN_13,         /**< 引脚13 */
    GPIO_PIN_14,         /**< 引脚14 */
    GPIO_PIN_15,         /**< 引脚15 */
    GPIO_PIN_MAX         /**< 最大引脚数量 */
} gpio_pin_t;

/* GPIO模式定义 */
typedef enum {
    GPIO_MODE_INPUT = 0,         /**< 输入模式 */
    GPIO_MODE_OUTPUT_PP,         /**< 推挽输出模式 */
    GPIO_MODE_OUTPUT_OD,         /**< 开漏输出模式 */
    GPIO_MODE_AF_PP,             /**< 推挽复用功能模式 */
    GPIO_MODE_AF_OD,             /**< 开漏复用功能模式 */
    GPIO_MODE_ANALOG,            /**< 模拟模式 */
    GPIO_MODE_IT_RISING,         /**< 上升沿触发中断模式 */
    GPIO_MODE_IT_FALLING,        /**< 下降沿触发中断模式 */
    GPIO_MODE_IT_RISING_FALLING  /**< 上升和下降沿触发中断模式 */
} gpio_mode_t;

/* GPIO上拉/下拉定义 */
typedef enum {
    GPIO_PULL_NONE = 0,  /**< 无上拉/下拉 */
    GPIO_PULL_UP,        /**< 上拉 */
    GPIO_PULL_DOWN       /**< 下拉 */
} gpio_pull_t;

/* GPIO速度定义 */
typedef enum {
    GPIO_SPEED_LOW = 0,      /**< 低速 */
    GPIO_SPEED_MEDIUM,       /**< 中速 */
    GPIO_SPEED_HIGH,         /**< 高速 */
    GPIO_SPEED_VERY_HIGH     /**< 超高速 */
} gpio_speed_t;

/* GPIO复用功能定义 */
typedef enum {
    GPIO_AF_0 = 0,     /**< 复用功能0 */
    GPIO_AF_1,         /**< 复用功能1 */
    GPIO_AF_2,         /**< 复用功能2 */
    GPIO_AF_3,         /**< 复用功能3 */
    GPIO_AF_4,         /**< 复用功能4 */
    GPIO_AF_5,         /**< 复用功能5 */
    GPIO_AF_6,         /**< 复用功能6 */
    GPIO_AF_7,         /**< 复用功能7 */
    GPIO_AF_8,         /**< 复用功能8 */
    GPIO_AF_9,         /**< 复用功能9 */
    GPIO_AF_10,        /**< 复用功能10 */
    GPIO_AF_11,        /**< 复用功能11 */
    GPIO_AF_12,        /**< 复用功能12 */
    GPIO_AF_13,        /**< 复用功能13 */
    GPIO_AF_14,        /**< 复用功能14 */
    GPIO_AF_15         /**< 复用功能15 */
} gpio_af_t;

/* GPIO状态定义 */
typedef enum {
    GPIO_PIN_RESET = 0,  /**< GPIO低电平 */
    GPIO_PIN_SET         /**< GPIO高电平 */
} gpio_state_t;

/* GPIO中断回调函数类型 */
typedef void (*gpio_irq_callback_t)(gpio_port_t port, gpio_pin_t pin, void *user_data);

/* GPIO配置参数 */
typedef struct {
    gpio_port_t port;     /**< GPIO端口 */
    gpio_pin_t pin;       /**< GPIO引脚 */
    gpio_mode_t mode;     /**< GPIO模式 */
    gpio_pull_t pull;     /**< GPIO上拉/下拉 */
    gpio_speed_t speed;   /**< GPIO速度 */
    gpio_af_t af;         /**< 复用功能（当mode为GPIO_MODE_AF_PP或GPIO_MODE_AF_OD时有效） */
} gpio_config_t;

/* GPIO设备句柄 */
typedef driver_handle_t gpio_handle_t;

/**
 * @brief 初始化GPIO
 * 
 * @param config GPIO配置参数
 * @param handle GPIO设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int gpio_init(const gpio_config_t *config, gpio_handle_t *handle);

/**
 * @brief 去初始化GPIO
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_deinit(gpio_handle_t handle);

/**
 * @brief 设置GPIO引脚状态
 * 
 * @param handle GPIO设备句柄
 * @param state GPIO状态
 * @return int 0表示成功，非0表示失败
 */
int gpio_write(gpio_handle_t handle, gpio_state_t state);

/**
 * @brief 读取GPIO引脚状态
 * 
 * @param handle GPIO设备句柄
 * @return gpio_state_t GPIO状态
 */
gpio_state_t gpio_read(gpio_handle_t handle);

/**
 * @brief 翻转GPIO引脚状态
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_toggle(gpio_handle_t handle);

/**
 * @brief 注册GPIO中断回调函数
 * 
 * @param handle GPIO设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int gpio_register_irq_callback(gpio_handle_t handle, gpio_irq_callback_t callback, void *user_data);

/**
 * @brief 使能GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_enable_irq(gpio_handle_t handle);

/**
 * @brief 禁用GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_disable_irq(gpio_handle_t handle);

#endif /* GPIO_API_H */
