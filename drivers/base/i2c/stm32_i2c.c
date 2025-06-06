/**
 * @file stm32_i2c.c
 * @brief STM32 I2C驱动实现
 *
 * 该文件实现了STM32平台的I2C接口，符合i2c_api.h中定义的统一接口
 */

#include "base/i2c_api.h"
#include "stm32_platform.h"
#include "stm32f4xx_hal.h"

/* I2C设备结构�?*/
typedef struct {
    I2C_HandleTypeDef hi2c;          /**< HAL I2C句柄 */
    i2c_channel_t channel;           /**< I2C通道 */
    bool initialized;                /**< 初始化标�?*/
} stm32_i2c_t;

/* I2C设备实例 */
static stm32_i2c_t stm32_i2c_devices[I2C_CHANNEL_MAX];

/**
 * @brief 获取STM32 I2C外设
 * 
 * @param channel I2C通道
 * @return I2C_TypeDef* I2C外设指针，失败返回NULL
 */
static I2C_TypeDef* get_i2c_instance(i2c_channel_t channel)
{
    switch (channel) {
        case I2C_CHANNEL_0:
            return I2C1;
        case I2C_CHANNEL_1:
            return I2C2;
        case I2C_CHANNEL_2:
            return I2C3;
        default:
            return NULL;
    }
}

/**
 * @brief 初始化I2C GPIO
 * 
 * @param channel I2C通道
 * @return int 0表示成功，非0表示失败
 */
static int init_i2c_gpio(i2c_channel_t channel)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    switch (channel) {
        case I2C_CHANNEL_0:
            /* I2C1 GPIO Configuration: PB6(SCL), PB7(SDA) */
            __HAL_RCC_GPIOB_CLK_ENABLE();
            
            GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            
            /* I2C1 clock enable */
            __HAL_RCC_I2C1_CLK_ENABLE();
            break;
            
        case I2C_CHANNEL_1:
            /* I2C2 GPIO Configuration: PB10(SCL), PB11(SDA) */
            __HAL_RCC_GPIOB_CLK_ENABLE();
            
            GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            
            /* I2C2 clock enable */
            __HAL_RCC_I2C2_CLK_ENABLE();
            break;
            
        case I2C_CHANNEL_2:
            /* I2C3 GPIO Configuration: PA8(SCL), PC9(SDA) */
            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_GPIOC_CLK_ENABLE();
            
            GPIO_InitStruct.Pin = GPIO_PIN_8;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            
            GPIO_InitStruct.Pin = GPIO_PIN_9;
            HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
            
            /* I2C3 clock enable */
            __HAL_RCC_I2C3_CLK_ENABLE();
            break;
            
        default:
            return DRIVER_INVALID_PARAM;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 初始化I2C总线
 * 
 * @param config I2C配置参数
 * @param handle I2C设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int i2c_init(const i2c_config_t *config, i2c_handle_t *handle)
{
    I2C_TypeDef *i2c_instance;
    stm32_i2c_t *stm32_i2c;
    int ret;
    
    if (config == NULL || handle == NULL || config->channel >= I2C_CHANNEL_MAX) {
        return DRIVER_INVALID_PARAM;
    }
    
    stm32_i2c = &stm32_i2c_devices[config->channel];
    
    if (stm32_i2c->initialized) {
        *handle = stm32_i2c;
        return DRIVER_OK;
    }
    
    i2c_instance = get_i2c_instance(config->channel);
    if (i2c_instance == NULL) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 初始化GPIO */
    ret = init_i2c_gpio(config->channel);
    if (ret != DRIVER_OK) {
        return ret;
    }
    
    /* 配置I2C参数 */
    stm32_i2c->hi2c.Instance = i2c_instance;
    stm32_i2c->hi2c.Init.ClockSpeed = 100000; /* 默认100kHz */
    stm32_i2c->hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    stm32_i2c->hi2c.Init.OwnAddress1 = 0;
    stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    stm32_i2c->hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    stm32_i2c->hi2c.Init.OwnAddress2 = 0;
    stm32_i2c->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    stm32_i2c->hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    /* 根据配置设置速率 */
    switch (config->speed) {
        case I2C_SPEED_STANDARD:
            stm32_i2c->hi2c.Init.ClockSpeed = 100000;
            break;
        case I2C_SPEED_FAST:
            stm32_i2c->hi2c.Init.ClockSpeed = 400000;
            break;
        case I2C_SPEED_FAST_PLUS:
            stm32_i2c->hi2c.Init.ClockSpeed = 1000000;
            break;
        case I2C_SPEED_HIGH:
            /* STM32 F4不支持高速模式，使用快速模�? */
            stm32_i2c->hi2c.Init.ClockSpeed = 1000000;
            break;
        default:
            return DRIVER_INVALID_PARAM;
    }
    
    /* 根据配置设置地址模式 */
    if (config->addr_10bit) {
        stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_10BIT;
    }
    
    /* 初始化I2C */
    if (HAL_I2C_Init(&stm32_i2c->hi2c) != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    /* 设置结构体字�?*/
    stm32_i2c->channel = config->channel;
    stm32_i2c->initialized = true;
    
    /* 返回句柄 */
    *handle = stm32_i2c;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化I2C总线
 * 
 * @param handle I2C设备句柄
 * @return int 0表示成功，非0表示失败
 */
int i2c_deinit(i2c_handle_t handle)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 去初始化I2C */
    if (HAL_I2C_DeInit(&stm32_i2c->hi2c) != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    /* 重置结构体字�?*/
    stm32_i2c->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief I2C主机发送数�?
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错�?
 */
int i2c_master_transmit(i2c_handle_t handle, uint16_t dev_addr, const uint8_t *data, 
                      uint32_t len, uint32_t flags, uint32_t timeout_ms)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    HAL_StatusTypeDef status;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized || data == NULL || len == 0) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 设置地址模式 */
    if (flags & I2C_FLAG_10BIT_ADDR) {
        stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_10BIT;
    } else {
        stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    }
    
    /* 发送数�?*/
    status = HAL_I2C_Master_Transmit(&stm32_i2c->hi2c, dev_addr, (uint8_t *)data, len, timeout_ms);
    
    if (status != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    return len;
}

/**
 * @brief I2C主机接收数据
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param flags 传输标志
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错�?
 */
int i2c_master_receive(i2c_handle_t handle, uint16_t dev_addr, uint8_t *data, 
                     uint32_t len, uint32_t flags, uint32_t timeout_ms)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    HAL_StatusTypeDef status;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized || data == NULL || len == 0) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 设置地址模式 */
    if (flags & I2C_FLAG_10BIT_ADDR) {
        stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_10BIT;
    } else {
        stm32_i2c->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    }
    
    /* 接收数据 */
    status = HAL_I2C_Master_Receive(&stm32_i2c->hi2c, dev_addr, data, len, timeout_ms);
    
    if (status != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    return len;
}

/**
 * @brief I2C内存写入
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小�?/2字节�?
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功写入的字节数，负值表示错�?
 */
int i2c_mem_write(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
                uint8_t mem_addr_size, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    HAL_StatusTypeDef status;
    uint16_t size;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized || data == NULL || len == 0) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 转换地址大小 */
    if (mem_addr_size == 1) {
        size = I2C_MEMADD_SIZE_8BIT;
    } else if (mem_addr_size == 2) {
        size = I2C_MEMADD_SIZE_16BIT;
    } else {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 写入内存 */
    status = HAL_I2C_Mem_Write(&stm32_i2c->hi2c, dev_addr, mem_addr, size, (uint8_t *)data, len, timeout_ms);
    
    if (status != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    return len;
}

/**
 * @brief I2C内存读取
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param mem_addr_size 内存地址大小�?/2字节�?
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功读取的字节数，负值表示错�?
 */
int i2c_mem_read(i2c_handle_t handle, uint16_t dev_addr, uint16_t mem_addr,
               uint8_t mem_addr_size, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    HAL_StatusTypeDef status;
    uint16_t size;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized || data == NULL || len == 0) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 转换地址大小 */
    if (mem_addr_size == 1) {
        size = I2C_MEMADD_SIZE_8BIT;
    } else if (mem_addr_size == 2) {
        size = I2C_MEMADD_SIZE_16BIT;
    } else {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 读取内存 */
    status = HAL_I2C_Mem_Read(&stm32_i2c->hi2c, dev_addr, mem_addr, size, data, len, timeout_ms);
    
    if (status != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    return len;
}

/**
 * @brief 检测I2C设备是否存在
 * 
 * @param handle I2C设备句柄
 * @param dev_addr 设备地址
 * @param retries 重试次数
 * @param timeout_ms 超时时间（毫秒）
 * @return int 0表示设备存在，非0表示设备不存在或错误
 */
int i2c_is_device_ready(i2c_handle_t handle, uint16_t dev_addr, uint32_t retries, uint32_t timeout_ms)
{
    stm32_i2c_t *stm32_i2c = (stm32_i2c_t *)handle;
    HAL_StatusTypeDef status;
    
    if (stm32_i2c == NULL || !stm32_i2c->initialized) {
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检测设备是否就�?*/
    status = HAL_I2C_IsDeviceReady(&stm32_i2c->hi2c, dev_addr, retries, timeout_ms);
    
    if (status != HAL_OK) {
        return DRIVER_ERROR;
    }
    
    return DRIVER_OK;
}

