/**
 * @file stm32_gpio.c
 * @brief STM32 GPIO驱动实现
 *
 * 该文件实现了STM32平台的GPIO接口，符合gpio_api.h中定义的统一接口
 */

#include "base/gpio_api.h"
#include "stm32_platform.h"
#include "stm32f4xx_hal.h"

/* GPIO设备结构�?*/
typedef struct {
    GPIO_TypeDef *port;        /**< GPIO端口 */
    uint16_t pin;              /**< GPIO引脚 */
    gpio_port_t port_id;       /**< GPIO端口ID */
    gpio_pin_t pin_id;         /**< GPIO引脚ID */
    gpio_mode_t mode;          /**< GPIO模式 */
    bool initialized;          /**< 初始化标�?*/
    gpio_irq_callback_t irq_callback; /**< 中断回调函数 */
    void *user_data;           /**< 用户数据 */
} stm32_gpio_t;

/* 存储GPIO设备实例的数�?*/
static stm32_gpio_t gpio_devices[GPIO_PORT_MAX * GPIO_PIN_MAX];
static uint32_t gpio_device_count = 0;

/**
 * @brief 获取STM32 GPIO端口指针
 * 
 * @param port GPIO端口
 * @return GPIO_TypeDef* GPIO端口指针，失败返回NULL
 */
static GPIO_TypeDef* get_gpio_port(gpio_port_t port)
{
    switch (port) {
        case GPIO_PORT_A:
            return GPIOA;
        case GPIO_PORT_B:
            return GPIOB;
        case GPIO_PORT_C:
            return GPIOC;
        case GPIO_PORT_D:
            return GPIOD;
        case GPIO_PORT_E:
            return GPIOE;
        case GPIO_PORT_F:
            return GPIOF;
        case GPIO_PORT_G:
            return GPIOG;
        case GPIO_PORT_H:
            return GPIOH;
        case GPIO_PORT_I:
            return GPIOI;
        default:
            return NULL;
    }
}

/**
 * @brief 获取STM32 GPIO引脚
 * 
 * @param pin GPIO引脚
 * @return uint16_t HAL GPIO引脚，失败返�?
 */
static uint16_t get_gpio_pin(gpio_pin_t pin)
{
    if (pin >= GPIO_PIN_MAX) {
        return 0;
    }
    
    return (1 << pin);
}

/**
 * @brief 使能GPIO端口时钟
 * 
 * @param port GPIO端口
 */
static void enable_gpio_clock(gpio_port_t port)
{
    switch (port) {
        case GPIO_PORT_A:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case GPIO_PORT_B:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case GPIO_PORT_C:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
        case GPIO_PORT_D:
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
        case GPIO_PORT_E:
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
        case GPIO_PORT_F:
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
        case GPIO_PORT_G:
            __HAL_RCC_GPIOG_CLK_ENABLE();
            break;
        case GPIO_PORT_H:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        case GPIO_PORT_I:
            __HAL_RCC_GPIOI_CLK_ENABLE();
            break;
        default:
            break;
    }
}

/**
 * @brief 将抽象GPIO模式转换为STM32 HAL模式
 * 
 * @param mode 抽象GPIO模式
 * @return uint32_t STM32 HAL模式
 */
static uint32_t convert_gpio_mode(gpio_mode_t mode)
{
    switch (mode) {
        case GPIO_MODE_INPUT:
            return GPIO_MODE_INPUT;
        case GPIO_MODE_OUTPUT_PP:
            return GPIO_MODE_OUTPUT_PP;
        case GPIO_MODE_OUTPUT_OD:
            return GPIO_MODE_OUTPUT_OD;
        case GPIO_MODE_AF_PP:
            return GPIO_MODE_AF_PP;
        case GPIO_MODE_AF_OD:
            return GPIO_MODE_AF_OD;
        case GPIO_MODE_ANALOG:
            return GPIO_MODE_ANALOG;
        case GPIO_MODE_IT_RISING:
            return GPIO_MODE_IT_RISING;
        case GPIO_MODE_IT_FALLING:
            return GPIO_MODE_IT_FALLING;
        case GPIO_MODE_IT_RISING_FALLING:
            return GPIO_MODE_IT_RISING_FALLING;
        default:
            return GPIO_MODE_INPUT;
    }
}

/**
 * @brief 将抽象GPIO上拉/下拉转换为STM32 HAL上拉/下拉
 * 
 * @param pull 抽象GPIO上拉/下拉
 * @return uint32_t STM32 HAL上拉/下拉
 */
static uint32_t convert_gpio_pull(gpio_pull_t pull)
{
    switch (pull) {
        case GPIO_PULL_NONE:
            return GPIO_NOPULL;
        case GPIO_PULL_UP:
            return GPIO_PULLUP;
        case GPIO_PULL_DOWN:
            return GPIO_PULLDOWN;
        default:
            return GPIO_NOPULL;
    }
}

/**
 * @brief 将抽象GPIO速度转换为STM32 HAL速度
 * 
 * @param speed 抽象GPIO速度
 * @return uint32_t STM32 HAL速度
 */
static uint32_t convert_gpio_speed(gpio_speed_t speed)
{
    switch (speed) {
        case GPIO_SPEED_LOW:
            return GPIO_SPEED_FREQ_LOW;
        case GPIO_SPEED_MEDIUM:
            return GPIO_SPEED_FREQ_MEDIUM;
        case GPIO_SPEED_HIGH:
            return GPIO_SPEED_FREQ_HIGH;
        case GPIO_SPEED_VERY_HIGH:
            return GPIO_SPEED_FREQ_VERY_HIGH;
        default:
            return GPIO_SPEED_FREQ_LOW;
    }
}

/**
 * @brief 查找已初始化的GPIO设备
 * 
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return stm32_gpio_t* GPIO设备指针，未找到返回NULL
 */
static stm32_gpio_t* find_gpio_device(gpio_port_t port, gpio_pin_t pin)
{
    for (uint32_t i = 0; i < gpio_device_count; i++) {
        if (gpio_devices[i].port_id == port && gpio_devices[i].pin_id == pin) {
            return &gpio_devices[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 获取GPIO IRQ编号
 * 
 * @param pin GPIO引脚
 * @return IRQn_Type IRQ编号
 */
static IRQn_Type get_gpio_irq_num(gpio_pin_t pin)
{
    switch (pin) {
        case GPIO_PIN_0:
            return EXTI0_IRQn;
        case GPIO_PIN_1:
            return EXTI1_IRQn;
        case GPIO_PIN_2:
            return EXTI2_IRQn;
        case GPIO_PIN_3:
            return EXTI3_IRQn;
        case GPIO_PIN_4:
            return EXTI4_IRQn;
        case GPIO_PIN_5:
        case GPIO_PIN_6:
        case GPIO_PIN_7:
        case GPIO_PIN_8:
        case GPIO_PIN_9:
            return EXTI9_5_IRQn;
        case GPIO_PIN_10:
        case GPIO_PIN_11:
        case GPIO_PIN_12:
        case GPIO_PIN_13:
        case GPIO_PIN_14:
        case GPIO_PIN_15:
            return EXTI15_10_IRQn;
        default:
            return EXTI0_IRQn;
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
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    stm32_gpio_t *stm32_gpio;
    
    if (config == NULL || handle == NULL || 
        config->port >= GPIO_PORT_MAX || config->pin >= GPIO_PIN_MAX) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查是否已经初始化 */
    stm32_gpio = find_gpio_device(config->port, config->pin);
    if (stm32_gpio != NULL) {
        *handle = stm32_gpio;
        return DRIVER_OK;
    }
    
    /* 检查是否超出设备数量限�?*/
    if (gpio_device_count >= GPIO_PORT_MAX * GPIO_PIN_MAX) {
        return DRIVER_ERROR;
    }
    
    /* 获取STM32 GPIO端口和引�?*/
    port = get_gpio_port(config->port);
    pin = get_gpio_pin(config->pin);
    
    if (port == NULL || pin == 0) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 使能GPIO时钟 */
    enable_gpio_clock(config->port);
    
    /* 配置GPIO */
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = convert_gpio_mode(config->mode);
    GPIO_InitStruct.Pull = convert_gpio_pull(config->pull);
    GPIO_InitStruct.Speed = convert_gpio_speed(config->speed);
    
    /* 配置复用功能 */
    if (config->mode == GPIO_MODE_AF_PP || config->mode == GPIO_MODE_AF_OD) {
        GPIO_InitStruct.Alternate = config->af;
    }
    
    /* 初始化GPIO */
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    
    /* 配置中断 */
    if (config->mode == GPIO_MODE_IT_RISING || 
        config->mode == GPIO_MODE_IT_FALLING || 
        config->mode == GPIO_MODE_IT_RISING_FALLING) {
        
        /* 使能SYSCFG时钟 */
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        
        /* 初始时禁用中�?*/
        IRQn_Type irq_num = get_gpio_irq_num(config->pin);
        HAL_NVIC_DisableIRQ(irq_num);
    }
    
    /* 填充设备结构�?*/
    stm32_gpio = &gpio_devices[gpio_device_count++];
    stm32_gpio->port = port;
    stm32_gpio->pin = pin;
    stm32_gpio->port_id = config->port;
    stm32_gpio->pin_id = config->pin;
    stm32_gpio->mode = config->mode;
    stm32_gpio->initialized = true;
    stm32_gpio->irq_callback = NULL;
    stm32_gpio->user_data = NULL;
    
    /* 返回句柄 */
    *handle = stm32_gpio;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化GPIO
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_deinit(gpio_handle_t handle)
{
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 去初始化GPIO */
    HAL_GPIO_DeInit(stm32_gpio->port, stm32_gpio->pin);
    
    /* 禁用中断 */
    if (stm32_gpio->mode == GPIO_MODE_IT_RISING || 
        stm32_gpio->mode == GPIO_MODE_IT_FALLING || 
        stm32_gpio->mode == GPIO_MODE_IT_RISING_FALLING) {
        
        IRQn_Type irq_num = get_gpio_irq_num(stm32_gpio->pin_id);
        HAL_NVIC_DisableIRQ(irq_num);
    }
    
    /* 重置结构体字�?*/
    stm32_gpio->initialized = false;
    stm32_gpio->irq_callback = NULL;
    stm32_gpio->user_data = NULL;
    
    return DRIVER_OK;
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
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查模式是否为输出 */
    if (stm32_gpio->mode != GPIO_MODE_OUTPUT_PP && 
        stm32_gpio->mode != GPIO_MODE_OUTPUT_OD) {
        return DRIVER_NOT_SUPPORTED;
    }
    
    /* 设置GPIO状�?*/
    HAL_GPIO_WritePin(stm32_gpio->port, stm32_gpio->pin, (GPIO_PinState)state);
    
    return DRIVER_OK;
}

/**
 * @brief 读取GPIO引脚状�?
 * 
 * @param handle GPIO设备句柄
 * @return gpio_state_t GPIO状�?
 */
gpio_state_t gpio_read(gpio_handle_t handle)
{
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return GPIO_PIN_RESET;
    }
    
    /* 读取GPIO状�?*/
    return (gpio_state_t)HAL_GPIO_ReadPin(stm32_gpio->port, stm32_gpio->pin);
}

/**
 * @brief 翻转GPIO引脚状�?
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_toggle(gpio_handle_t handle)
{
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查模式是否为输出 */
    if (stm32_gpio->mode != GPIO_MODE_OUTPUT_PP && 
        stm32_gpio->mode != GPIO_MODE_OUTPUT_OD) {
        return DRIVER_NOT_SUPPORTED;
    }
    
    /* 翻转GPIO状�?*/
    HAL_GPIO_TogglePin(stm32_gpio->port, stm32_gpio->pin);
    
    return DRIVER_OK;
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
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized || callback == NULL) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查模式是否为中断 */
    if (stm32_gpio->mode != GPIO_MODE_IT_RISING && 
        stm32_gpio->mode != GPIO_MODE_IT_FALLING && 
        stm32_gpio->mode != GPIO_MODE_IT_RISING_FALLING) {
        return DRIVER_NOT_SUPPORTED;
    }
    
    /* 注册回调函数 */
    stm32_gpio->irq_callback = callback;
    stm32_gpio->user_data = user_data;
    
    return DRIVER_OK;
}

/**
 * @brief 使能GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_enable_irq(gpio_handle_t handle)
{
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查模式是否为中断 */
    if (stm32_gpio->mode != GPIO_MODE_IT_RISING && 
        stm32_gpio->mode != GPIO_MODE_IT_FALLING && 
        stm32_gpio->mode != GPIO_MODE_IT_RISING_FALLING) {
        return DRIVER_NOT_SUPPORTED;
    }
    
    /* 使能中断 */
    IRQn_Type irq_num = get_gpio_irq_num(stm32_gpio->pin_id);
    HAL_NVIC_SetPriority(irq_num, 5, 0);
    HAL_NVIC_EnableIRQ(irq_num);
    
    return DRIVER_OK;
}

/**
 * @brief 禁用GPIO中断
 * 
 * @param handle GPIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int gpio_disable_irq(gpio_handle_t handle)
{
    stm32_gpio_t *stm32_gpio = (stm32_gpio_t *)handle;
    
    if (stm32_gpio == NULL || !stm32_gpio->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查模式是否为中断 */
    if (stm32_gpio->mode != GPIO_MODE_IT_RISING && 
        stm32_gpio->mode != GPIO_MODE_IT_FALLING && 
        stm32_gpio->mode != GPIO_MODE_IT_RISING_FALLING) {
        return DRIVER_NOT_SUPPORTED;
    }
    
    /* 禁用中断 */
    IRQn_Type irq_num = get_gpio_irq_num(stm32_gpio->pin_id);
    HAL_NVIC_DisableIRQ(irq_num);
    
    return DRIVER_OK;
}

/**
 * @brief EXTI中断处理函数
 * 
 * @param pin GPIO引脚
 */
static void gpio_irq_handler(gpio_pin_t pin)
{
    stm32_gpio_t *stm32_gpio;
    
    /* 遍历所有GPIO设备，查找对应的引脚 */
    for (uint32_t i = 0; i < gpio_device_count; i++) {
        stm32_gpio = &gpio_devices[i];
        
        if (stm32_gpio->initialized && stm32_gpio->pin_id == pin && 
            stm32_gpio->irq_callback != NULL) {
            
            /* 调用用户回调函数 */
            stm32_gpio->irq_callback(stm32_gpio->port_id, stm32_gpio->pin_id, stm32_gpio->user_data);
            break;
        }
    }
}

/**
 * @brief EXTI0中断处理函数
 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    gpio_irq_handler(GPIO_PIN_0);
}

/**
 * @brief EXTI1中断处理函数
 */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    gpio_irq_handler(GPIO_PIN_1);
}

/**
 * @brief EXTI2中断处理函数
 */
void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    gpio_irq_handler(GPIO_PIN_2);
}

/**
 * @brief EXTI3中断处理函数
 */
void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
    gpio_irq_handler(GPIO_PIN_3);
}

/**
 * @brief EXTI4中断处理函数
 */
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    gpio_irq_handler(GPIO_PIN_4);
}

/**
 * @brief EXTI9_5中断处理函数
 */
void EXTI9_5_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
        gpio_irq_handler(GPIO_PIN_5);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
        gpio_irq_handler(GPIO_PIN_6);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
        gpio_irq_handler(GPIO_PIN_7);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
        gpio_irq_handler(GPIO_PIN_8);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
        gpio_irq_handler(GPIO_PIN_9);
    }
}

/**
 * @brief EXTI15_10中断处理函数
 */
void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
        gpio_irq_handler(GPIO_PIN_10);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
        gpio_irq_handler(GPIO_PIN_11);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
        gpio_irq_handler(GPIO_PIN_12);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
        gpio_irq_handler(GPIO_PIN_13);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
        gpio_irq_handler(GPIO_PIN_14);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
        gpio_irq_handler(GPIO_PIN_15);
    }
}

