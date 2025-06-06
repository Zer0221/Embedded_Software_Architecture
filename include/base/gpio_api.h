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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    GPIO_MODE_INPUT = 0,      /**< 输入模式 */
    GPIO_MODE_OUTPUT,         /**< 输出模式 */
    GPIO_MODE_AF,             /**< 复用功能模式 */
    GPIO_MODE_ANALOG          /**< 模拟模式 */
} gpio_mode_t;

/* GPIO输出类型定义 */
typedef enum {
    GPIO_OTYPE_PP = 0,        /**< 推挽输出 */
    GPIO_OTYPE_OD             /**< 开漏输出 */
} gpio_otype_t;

/* GPIO速度定义 */
typedef enum {
    GPIO_SPEED_LOW = 0,       /**< 低速 */
    GPIO_SPEED_MEDIUM,        /**< 中速 */
    GPIO_SPEED_HIGH,          /**< 高速 */
    GPIO_SPEED_VERY_HIGH      /**< 超高速 */
} gpio_speed_t;

/* GPIO上拉/下拉定义 */
typedef enum {
    GPIO_PULL_NONE = 0,       /**< 无上拉/下拉 */
    GPIO_PULL_UP,             /**< 上拉 */
    GPIO_PULL_DOWN            /**< 下拉 */
} gpio_pull_t;

/* GPIO引脚状态 */
typedef enum {
    GPIO_PIN_RESET = 0,       /**< 引脚低电平 */
    GPIO_PIN_SET              /**< 引脚高电平 */
} gpio_pin_state_t;

/* GPIO中断触发方式 */
typedef enum {
    GPIO_IT_NONE = 0,         /**< 无中断 */
    GPIO_IT_RISING,           /**< 上升沿触发 */
    GPIO_IT_FALLING,          /**< 下降沿触发 */
    GPIO_IT_BOTH              /**< 双边沿触发 */
} gpio_it_mode_t;

/* GPIO引脚配置 */
typedef struct {
    gpio_port_t port;         /**< GPIO端口 */
    gpio_pin_t pin;           /**< GPIO引脚 */
    gpio_mode_t mode;         /**< GPIO模式 */
    gpio_otype_t otype;       /**< 输出类型 */
    gpio_speed_t speed;       /**< 速度 */
    gpio_pull_t pull;         /**< 上拉/下拉 */
    uint8_t alternate;        /**< 复用功能编号，当mode为GPIO_MODE_AF时有效 */
} gpio_config_t;

/* GPIO中断回调函数类型 */
typedef void (*gpio_callback_t)(gpio_port_t port, gpio_pin_t pin, void *arg);

/**
 * @brief 初始化GPIO
 * 
 * @param config GPIO配置
 * @return int 0表示成功，非0表示失败
 */
int gpio_init(const gpio_config_t *config);

/**
 * @brief 去初始化GPIO
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_deinit(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief 设置GPIO输出电平
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @param state 引脚状态
 * @return int 0表示成功，非0表示失败
 */
int gpio_write(gpio_port_t port, gpio_pin_t pin, gpio_pin_state_t state);

/**
 * @brief 读取GPIO输入电平
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @param state 引脚状态指针
 * @return int 0表示成功，非0表示失败
 */
int gpio_read(gpio_port_t port, gpio_pin_t pin, gpio_pin_state_t *state);

/**
 * @brief 翻转GPIO输出电平
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_toggle(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief 配置GPIO中断
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @param mode 中断触发模式
 * @param callback 中断回调函数
 * @param arg 回调函数参数
 * @return int 0表示成功，非0表示失败
 */
int gpio_config_interrupt(gpio_port_t port, gpio_pin_t pin, gpio_it_mode_t mode, 
                           gpio_callback_t callback, void *arg);

/**
 * @brief 使能GPIO中断
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_enable_interrupt(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief 禁用GPIO中断
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return int 0表示成功，非0表示失败
 */
int gpio_disable_interrupt(gpio_port_t port, gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_API_H */