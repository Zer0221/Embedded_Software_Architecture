/**
 * @file stm32_spi.c
 * @brief STM32平台的SPI驱动实现
 *
 * 该文件实现了STM32平台的SPI驱动，符合SPI抽象接口的规�?
 */

#include "base/spi_api.h"
#include "base/platform_api.h"
#include "common/error_api.h"
#include "stm32_platform.h"

#if (STM32_CURRENT_SERIES == STM32_SERIES_F4)
#include "stm32f4xx_hal.h"
#elif (STM32_CURRENT_SERIES == STM32_SERIES_F7)
#include "stm32f7xx_hal.h"
#endif

/* SPI句柄映射�?*/
static SPI_HandleTypeDef spi_handles[SPI_CHANNEL_MAX];

/* SPI资源使用标志 */
static bool spi_used[SPI_CHANNEL_MAX] = {false};

/* SPI内部结构�?*/
typedef struct {
    SPI_HandleTypeDef *hspi;          /* HAL SPI句柄 */
    spi_channel_t channel;            /* SPI通道 */
    bool initialized;                  /* 初始化标�?*/
    spi_event_callback_t callback;    /* 事件回调函数 */
    void *user_data;                  /* 用户数据 */
} stm32_spi_t;

/**
 * @brief 获取STM32 SPI实例
 * 
 * @param channel SPI通道
 * @return SPI_TypeDef* SPI实例指针
 */
static SPI_TypeDef* get_spi_instance(spi_channel_t channel)
{
    switch (channel) {
        case SPI_CHANNEL_0:
            return SPI1;
        case SPI_CHANNEL_1:
            return SPI2;
        case SPI_CHANNEL_2:
            return SPI3;
        default:
            return NULL;
    }
}

/**
 * @brief 获取STM32 SPI时钟预分频�?
 * 
 * @param speed SPI速率
 * @return uint32_t 预分频�?
 */
static uint32_t get_spi_prescaler(spi_speed_t speed)
{
    switch (speed) {
        case SPI_SPEED_LOW:
            return SPI_BAUDRATEPRESCALER_256; /* 系统时钟/256 */
        case SPI_SPEED_MEDIUM:
            return SPI_BAUDRATEPRESCALER_32;  /* 系统时钟/32 */
        case SPI_SPEED_HIGH:
            return SPI_BAUDRATEPRESCALER_8;   /* 系统时钟/8 */
        case SPI_SPEED_VERY_HIGH:
            return SPI_BAUDRATEPRESCALER_2;   /* 系统时钟/2 */
        default:
            return SPI_BAUDRATEPRESCALER_32;
    }
}

/**
 * @brief SPI接口初始�?
 * 
 * @param config SPI配置参数
 * @param handle SPI设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int spi_init(const spi_config_t *config, spi_handle_t *handle)
{
    SPI_TypeDef *spi_instance;
    SPI_HandleTypeDef *hspi;
    stm32_spi_t *spi_dev;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= SPI_CHANNEL_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已被使用 */
    if (spi_used[config->channel]) {
        return ERROR_RESOURCE_BUSY;
    }
    
    /* 获取SPI实例 */
    spi_instance = get_spi_instance(config->channel);
    if (spi_instance == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 分配内部结构�?*/
    spi_dev = (stm32_spi_t *)malloc(sizeof(stm32_spi_t));
    if (spi_dev == NULL) {
        return ERROR_NO_MEMORY;
    }
    
    /* 初始化内部结构体 */
    hspi = &spi_handles[config->channel];
    spi_dev->hspi = hspi;
    spi_dev->channel = config->channel;
    spi_dev->initialized = true;
    spi_dev->callback = NULL;
    spi_dev->user_data = NULL;
    
    /* 配置SPI参数 */
    hspi->Instance = spi_instance;
    hspi->Init.Mode = (config->mode == SPI_MODE_MASTER) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi->Init.Direction = SPI_DIRECTION_2LINES;
    
    /* 根据SPI模式设置CPOL和CPHA */
    switch (config->format) {
        case SPI_FORMAT_CPOL0_CPHA0:
            hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
            break;
        case SPI_FORMAT_CPOL0_CPHA1:
            hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
            break;
        case SPI_FORMAT_CPOL1_CPHA0:
            hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
            break;
        case SPI_FORMAT_CPOL1_CPHA1:
            hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
            break;
        default:
            hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
            break;
    }
    
    hspi->Init.DataSize = (config->data_bits == SPI_DATA_8BIT) ? SPI_DATASIZE_8BIT : SPI_DATASIZE_16BIT;
    hspi->Init.NSS = (config->nss == SPI_NSS_SOFT) ? SPI_NSS_SOFT : SPI_NSS_HARD_OUTPUT;
    hspi->Init.BaudRatePrescaler = get_spi_prescaler(config->speed);
    
    /* 根据位序设置MSB/LSB */
    hspi->Init.FirstBit = (config->bit_order == SPI_MSB_FIRST) ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial = 7;
    
    /* 初始化SPI外设 */
    if (HAL_SPI_Init(hspi) != HAL_OK) {
        free(spi_dev);
        return ERROR_HARDWARE;
    }
    
    /* 更新使用标志 */
    spi_used[config->channel] = true;
    
    /* 返回句柄 */
    *handle = (spi_handle_t)spi_dev;
    
    return ERROR_NONE;
}

/**
 * @brief SPI接口去初始化
 * 
 * @param handle SPI设备句柄
 * @return int 0表示成功，非0表示失败
 */
int spi_deinit(spi_handle_t handle)
{
    stm32_spi_t *spi_dev = (stm32_spi_t *)handle;
    
    /* 参数检�?*/
    if (spi_dev == NULL || !spi_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 去初始化SPI外设 */
    if (HAL_SPI_DeInit(spi_dev->hspi) != HAL_OK) {
        return ERROR_HARDWARE;
    }
    
    /* 更新使用标志 */
    spi_used[spi_dev->channel] = false;
    
    /* 释放内部结构�?*/
    spi_dev->initialized = false;
    free(spi_dev);
    
    return ERROR_NONE;
}

/**
 * @brief SPI传输数据（发送和接收�?
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功传输的字节数，负值表示错�?
 */
int spi_transfer(spi_handle_t handle, const void *tx_data, void *rx_data, uint32_t len, uint32_t timeout_ms)
{
    stm32_spi_t *spi_dev = (stm32_spi_t *)handle;
    HAL_StatusTypeDef status;
    
    /* 参数检�?*/
    if (spi_dev == NULL || !spi_dev->initialized || (tx_data == NULL && rx_data == NULL) || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 根据发送和接收缓冲区情况选择传输模式 */
    if (tx_data != NULL && rx_data != NULL) {
        /* 全双工传�?*/
        status = HAL_SPI_TransmitReceive(spi_dev->hspi, (uint8_t *)tx_data, (uint8_t *)rx_data, len, timeout_ms);
    } else if (tx_data != NULL) {
        /* 只发�?*/
        status = HAL_SPI_Transmit(spi_dev->hspi, (uint8_t *)tx_data, len, timeout_ms);
    } else {
        /* 只接�?*/
        status = HAL_SPI_Receive(spi_dev->hspi, (uint8_t *)rx_data, len, timeout_ms);
    }
    
    /* 检查传输状�?*/
    if (status == HAL_OK) {
        return len;
    } else if (status == HAL_TIMEOUT) {
        return ERROR_TIMEOUT;
    } else {
        return ERROR_HARDWARE;
    }
}

/**
 * @brief 注册SPI事件回调函数
 * 
 * @param handle SPI设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int spi_register_event_callback(spi_handle_t handle, spi_event_callback_t callback, void *user_data)
{
    stm32_spi_t *spi_dev = (stm32_spi_t *)handle;
    
    /* 参数检�?*/
    if (spi_dev == NULL || !spi_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 注册回调函数 */
    spi_dev->callback = callback;
    spi_dev->user_data = user_data;
    
    return ERROR_NONE;
}

/**
 * @brief 启动异步传输
 * 
 * @param handle SPI设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲�?
 * @param len 数据长度
 * @return int 0表示成功，非0表示失败
 */
int spi_transfer_async(spi_handle_t handle, const void *tx_data, void *rx_data, uint32_t len)
{
    stm32_spi_t *spi_dev = (stm32_spi_t *)handle;
    HAL_StatusTypeDef status;
    
    /* 参数检�?*/
    if (spi_dev == NULL || !spi_dev->initialized || (tx_data == NULL && rx_data == NULL) || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否注册了回调函数 */
    if (spi_dev->callback == NULL) {
        return ERROR_NOT_READY;
    }
    
    /* 根据发送和接收缓冲区情况选择传输模式 */
    if (tx_data != NULL && rx_data != NULL) {
        /* 全双工传�?*/
        status = HAL_SPI_TransmitReceive_IT(spi_dev->hspi, (uint8_t *)tx_data, (uint8_t *)rx_data, len);
    } else if (tx_data != NULL) {
        /* 只发�?*/
        status = HAL_SPI_Transmit_IT(spi_dev->hspi, (uint8_t *)tx_data, len);
    } else {
        /* 只接�?*/
        status = HAL_SPI_Receive_IT(spi_dev->hspi, (uint8_t *)rx_data, len);
    }
    
    /* 检查传输状�?*/
    if (status == HAL_OK) {
        return ERROR_NONE;
    } else {
        return ERROR_HARDWARE;
    }
}

/**
 * @brief 设置片选引脚状�?
 * 
 * @param handle SPI设备句柄
 * @param state 片选状态，0表示有效，非0表示无效
 * @return int 0表示成功，非0表示失败
 */
int spi_set_cs(spi_handle_t handle, int state)
{
    stm32_spi_t *spi_dev = (stm32_spi_t *)handle;
    
    /* 参数检�?*/
    if (spi_dev == NULL || !spi_dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 对于软件NSS，用户需要使用GPIO控制CS引脚 */
    /* 此处可以添加对GPIO的控制代码，或者在应用层实�?*/
    
    return ERROR_NONE;
}

/**
 * @brief SPI中断处理函数（在HAL库的SPI IRQ回调中调用）
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* 查找对应的SPI设备 */
    for (int i = 0; i < SPI_CHANNEL_MAX; i++) {
        if (&spi_handles[i] == hspi) {
            /* 遍历所有SPI设备，查找对应的句柄 */
            stm32_spi_t *spi_dev = NULL;
            /* 这里需要维护一个映射关系，此处简化处�?*/
            
            /* 如果找到设备并且有回调函数，则调用回�?*/
            if (spi_dev != NULL && spi_dev->callback != NULL) {
                spi_dev->callback(spi_dev, SPI_EVENT_TX_COMPLETE, spi_dev->user_data);
            }
            break;
        }
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* 查找对应的SPI设备 */
    for (int i = 0; i < SPI_CHANNEL_MAX; i++) {
        if (&spi_handles[i] == hspi) {
            /* 遍历所有SPI设备，查找对应的句柄 */
            stm32_spi_t *spi_dev = NULL;
            /* 这里需要维护一个映射关系，此处简化处�?*/
            
            /* 如果找到设备并且有回调函数，则调用回�?*/
            if (spi_dev != NULL && spi_dev->callback != NULL) {
                spi_dev->callback(spi_dev, SPI_EVENT_RX_COMPLETE, spi_dev->user_data);
            }
            break;
        }
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* 查找对应的SPI设备 */
    for (int i = 0; i < SPI_CHANNEL_MAX; i++) {
        if (&spi_handles[i] == hspi) {
            /* 遍历所有SPI设备，查找对应的句柄 */
            stm32_spi_t *spi_dev = NULL;
            /* 这里需要维护一个映射关系，此处简化处�?*/
            
            /* 如果找到设备并且有回调函数，则调用回�?*/
            if (spi_dev != NULL && spi_dev->callback != NULL) {
                spi_dev->callback(spi_dev, SPI_EVENT_TRANSFER_COMPLETE, spi_dev->user_data);
            }
            break;
        }
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    /* 查找对应的SPI设备 */
    for (int i = 0; i < SPI_CHANNEL_MAX; i++) {
        if (&spi_handles[i] == hspi) {
            /* 遍历所有SPI设备，查找对应的句柄 */
            stm32_spi_t *spi_dev = NULL;
            /* 这里需要维护一个映射关系，此处简化处�?*/
            
            /* 如果找到设备并且有回调函数，则调用回�?*/
            if (spi_dev != NULL && spi_dev->callback != NULL) {
                spi_dev->callback(spi_dev, SPI_EVENT_ERROR, spi_dev->user_data);
            }
            break;
        }
    }
}

