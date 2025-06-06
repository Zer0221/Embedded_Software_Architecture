/**
 * @file stm32_dma.c
 * @brief STM32平台DMA驱动实现
 *
 * 该文件实现了STM32平台的DMA驱动接口
 */

#include "base/dma_api.h"
#include "common/error_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* DMA通道最大数�?*/
#define STM32_DMA_MAX_CHANNELS 16

/* DMA流数�?*/
#define STM32_DMA_STREAMS_PER_CONTROLLER 8

/* STM32 DMA控制�?*/
#define STM32_DMA1_BASE DMA1
#define STM32_DMA2_BASE DMA2

/* STM32 DMA设备结构�?*/
typedef struct {
    DMA_HandleTypeDef hdma;             /* HAL DMA句柄 */
    uint32_t channel;                   /* DMA通道 */
    dma_config_t config;                /* DMA配置 */
    dma_callback_t callback;            /* 回调函数 */
    void *arg;                          /* 回调参数 */
    volatile dma_status_t status;       /* 传输状�?*/
    bool initialized;                   /* 初始化标�?*/
} stm32_dma_device_t;

/* DMA设备数组 */
static stm32_dma_device_t g_dma_devices[STM32_DMA_MAX_CHANNELS];

/* 获取DMA控制器和�?*/
static DMA_Stream_TypeDef *get_dma_stream(uint32_t dma_channel)
{
    if (dma_channel < STM32_DMA_STREAMS_PER_CONTROLLER) {
        /* DMA1 Stream 0-7 */
        return ((DMA_Stream_TypeDef *)((uint32_t)STM32_DMA1_BASE + 0x10 + 0x18 * dma_channel));
    } else if (dma_channel < STM32_DMA_MAX_CHANNELS) {
        /* DMA2 Stream 0-7 */
        return ((DMA_Stream_TypeDef *)((uint32_t)STM32_DMA2_BASE + 0x10 + 0x18 * (dma_channel - STM32_DMA_STREAMS_PER_CONTROLLER)));
    } else {
        return NULL;
    }
}

/* 获取DMA控制器基址 */
static DMA_TypeDef *get_dma_instance(uint32_t dma_channel)
{
    if (dma_channel < STM32_DMA_STREAMS_PER_CONTROLLER) {
        return DMA1;
    } else if (dma_channel < STM32_DMA_MAX_CHANNELS) {
        return DMA2;
    } else {
        return NULL;
    }
}

/* 转换数据宽度 */
static uint32_t convert_data_width(dma_data_width_t width)
{
    switch (width) {
        case DMA_DATA_WIDTH_8BIT:
            return DMA_PDATAALIGN_BYTE;
        case DMA_DATA_WIDTH_16BIT:
            return DMA_PDATAALIGN_HALFWORD;
        case DMA_DATA_WIDTH_32BIT:
            return DMA_PDATAALIGN_WORD;
        default:
            return DMA_PDATAALIGN_BYTE;
    }
}

/* 转换传输方向 */
static uint32_t convert_direction(dma_direction_t direction)
{
    switch (direction) {
        case DMA_DIR_MEM_TO_MEM:
            return DMA_MEMORY_TO_MEMORY;
        case DMA_DIR_MEM_TO_PERIPH:
            return DMA_MEMORY_TO_PERIPH;
        case DMA_DIR_PERIPH_TO_MEM:
            return DMA_PERIPH_TO_MEMORY;
        case DMA_DIR_PERIPH_TO_PERIPH:
            /* STM32 DMA不支持外设到外设，使用内存到内存代替 */
            return DMA_MEMORY_TO_MEMORY;
        default:
            return DMA_PERIPH_TO_MEMORY;
    }
}

/* 转换传输模式 */
static uint32_t convert_mode(dma_mode_t mode)
{
    switch (mode) {
        case DMA_MODE_NORMAL:
            return DMA_NORMAL;
        case DMA_MODE_CIRCULAR:
            return DMA_CIRCULAR;
        default:
            return DMA_NORMAL;
    }
}

/* 转换传输优先�?*/
static uint32_t convert_priority(dma_priority_t priority)
{
    switch (priority) {
        case DMA_PRIORITY_LOW:
            return DMA_PRIORITY_LOW;
        case DMA_PRIORITY_MEDIUM:
            return DMA_PRIORITY_MEDIUM;
        case DMA_PRIORITY_HIGH:
            return DMA_PRIORITY_HIGH;
        case DMA_PRIORITY_VERY_HIGH:
            return DMA_PRIORITY_VERY_HIGH;
        default:
            return DMA_PRIORITY_MEDIUM;
    }
}

/* DMA中断处理函数 */
static void dma_irq_handler(stm32_dma_device_t *dev)
{
    /* 检查传输完成标�?*/
    if (__HAL_DMA_GET_FLAG(&dev->hdma, __HAL_DMA_GET_TC_FLAG_INDEX(&dev->hdma))) {
        /* 清除标志 */
        __HAL_DMA_CLEAR_FLAG(&dev->hdma, __HAL_DMA_GET_TC_FLAG_INDEX(&dev->hdma));
        
        /* 更新状�?*/
        dev->status = DMA_STATUS_COMPLETE;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, DMA_STATUS_COMPLETE);
        }
    }
    
    /* 检查传输错误标�?*/
    if (__HAL_DMA_GET_FLAG(&dev->hdma, __HAL_DMA_GET_TE_FLAG_INDEX(&dev->hdma))) {
        /* 清除标志 */
        __HAL_DMA_CLEAR_FLAG(&dev->hdma, __HAL_DMA_GET_TE_FLAG_INDEX(&dev->hdma));
        
        /* 更新状�?*/
        dev->status = DMA_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, DMA_STATUS_ERROR);
        }
    }
}

/**
 * @brief 初始化DMA
 * 
 * @param dma_channel DMA通道
 * @param config DMA配置参数
 * @param callback DMA传输完成回调函数
 * @param arg 传递给回调函数的参�?
 * @param handle DMA句柄指针
 * @return int 0表示成功，非0表示失败
 */
int dma_init(uint32_t dma_channel, const dma_config_t *config, 
             dma_callback_t callback, void *arg, dma_handle_t *handle)
{
    stm32_dma_device_t *dev;
    DMA_HandleTypeDef *hdma;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道有效�?*/
    if (dma_channel >= STM32_DMA_MAX_CHANNELS) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取DMA设备 */
    dev = &g_dma_devices[dma_channel];
    
    /* 检查是否已初始�?*/
    if (dev->initialized) {
        return ERROR_ALREADY_INITIALIZED;
    }
    
    /* 初始化DMA设备 */
    memset(dev, 0, sizeof(stm32_dma_device_t));
    dev->channel = dma_channel;
    dev->callback = callback;
    dev->arg = arg;
    dev->status = DMA_STATUS_IDLE;
    
    /* 复制配置 */
    memcpy(&dev->config, config, sizeof(dma_config_t));
    
    /* 获取HAL DMA句柄 */
    hdma = &dev->hdma;
    
    /* 初始化DMA配置 */
    hdma->Instance = get_dma_stream(dma_channel);
    hdma->Init.Channel = 0; /* 通道由外设决定，这里设为0 */
    hdma->Init.Direction = convert_direction(config->direction);
    hdma->Init.PeriphInc = config->src_inc ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
    hdma->Init.MemInc = config->dst_inc ? DMA_MINC_ENABLE : DMA_MINC_DISABLE;
    hdma->Init.PeriphDataAlignment = convert_data_width(config->src_width);
    hdma->Init.MemDataAlignment = convert_data_width(config->dst_width);
    hdma->Init.Mode = convert_mode(config->mode);
    hdma->Init.Priority = convert_priority(config->priority);
    hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma->Init.MemBurst = DMA_MBURST_SINGLE;
    hdma->Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    /* 启用DMA时钟 */
    if (dma_channel < STM32_DMA_STREAMS_PER_CONTROLLER) {
        __HAL_RCC_DMA1_CLK_ENABLE();
    } else {
        __HAL_RCC_DMA2_CLK_ENABLE();
    }
    
    /* 初始化HAL DMA */
    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 设置源地址和目标地址 */
    if (config->direction == DMA_DIR_MEM_TO_PERIPH) {
        hdma->Instance->M0AR = config->src_addr;
        hdma->Instance->PAR = config->dst_addr;
    } else {
        hdma->Instance->PAR = config->src_addr;
        hdma->Instance->M0AR = config->dst_addr;
    }
    
    /* 设置数据大小 */
    hdma->Instance->NDTR = config->data_size;
    
    /* 设置初始化标�?*/
    dev->initialized = true;
    
    /* 返回句柄 */
    *handle = (dma_handle_t)dev;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化DMA
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_deinit(dma_handle_t handle)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 禁用DMA中断 */
    dma_disable_interrupt(handle);
    
    /* 停止DMA传输 */
    dma_stop(handle);
    
    /* 去初始化HAL DMA */
    if (HAL_DMA_DeInit(&dev->hdma) != HAL_OK) {
        return ERROR_DRIVER_DEINIT_FAILED;
    }
    
    /* 清除初始化标�?*/
    dev->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 开始DMA传输
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_start(dma_handle_t handle)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = DMA_STATUS_BUSY;
    
    /* 开始DMA传输 */
    if (dev->config.direction == DMA_DIR_MEM_TO_PERIPH) {
        if (HAL_DMA_Start(&dev->hdma, dev->config.src_addr, dev->config.dst_addr, dev->config.data_size) != HAL_OK) {
            dev->status = DMA_STATUS_ERROR;
            return ERROR_DRIVER_START_FAILED;
        }
    } else {
        if (HAL_DMA_Start(&dev->hdma, dev->config.src_addr, dev->config.dst_addr, dev->config.data_size) != HAL_OK) {
            dev->status = DMA_STATUS_ERROR;
            return ERROR_DRIVER_START_FAILED;
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 停止DMA传输
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_stop(dma_handle_t handle)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 停止DMA传输 */
    if (HAL_DMA_Abort(&dev->hdma) != HAL_OK) {
        return ERROR_DRIVER_STOP_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = DMA_STATUS_ABORT;
    
    return DRIVER_OK;
}

/**
 * @brief 获取DMA传输状�?
 * 
 * @param handle DMA句柄
 * @param status 状态指�?
 * @return int 0表示成功，非0表示失败
 */
int dma_get_status(dma_handle_t handle, dma_status_t *status)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || status == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取状�?*/
    *status = dev->status;
    
    return DRIVER_OK;
}

/**
 * @brief 获取DMA剩余传输数据大小
 * 
 * @param handle DMA句柄
 * @param remaining_size 剩余数据大小指针
 * @return int 0表示成功，非0表示失败
 */
int dma_get_remaining(dma_handle_t handle, uint32_t *remaining_size)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || remaining_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取剩余数据大小 */
    *remaining_size = __HAL_DMA_GET_COUNTER(&dev->hdma);
    
    return DRIVER_OK;
}

/**
 * @brief 设置DMA传输源地址
 * 
 * @param handle DMA句柄
 * @param src_addr 源地址
 * @return int 0表示成功，非0表示失败
 */
int dma_set_src_address(dma_handle_t handle, uint32_t src_addr)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新源地址 */
    dev->config.src_addr = src_addr;
    
    /* 设置源地址 */
    if (dev->config.direction == DMA_DIR_MEM_TO_PERIPH) {
        dev->hdma.Instance->M0AR = src_addr;
    } else {
        dev->hdma.Instance->PAR = src_addr;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置DMA传输目标地址
 * 
 * @param handle DMA句柄
 * @param dst_addr 目标地址
 * @return int 0表示成功，非0表示失败
 */
int dma_set_dst_address(dma_handle_t handle, uint32_t dst_addr)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新目标地址 */
    dev->config.dst_addr = dst_addr;
    
    /* 设置目标地址 */
    if (dev->config.direction == DMA_DIR_MEM_TO_PERIPH) {
        dev->hdma.Instance->PAR = dst_addr;
    } else {
        dev->hdma.Instance->M0AR = dst_addr;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置DMA传输数据大小
 * 
 * @param handle DMA句柄
 * @param data_size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int dma_set_data_size(dma_handle_t handle, uint32_t data_size)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新数据大小 */
    dev->config.data_size = data_size;
    
    /* 设置数据大小 */
    dev->hdma.Instance->NDTR = data_size;
    
    return DRIVER_OK;
}

/**
 * @brief 使能DMA中断
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_enable_interrupt(dma_handle_t handle)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 使能DMA传输完成中断 */
    __HAL_DMA_ENABLE_IT(&dev->hdma, DMA_IT_TC);
    
    /* 使能DMA传输错误中断 */
    __HAL_DMA_ENABLE_IT(&dev->hdma, DMA_IT_TE);
    
    return DRIVER_OK;
}

/**
 * @brief 禁用DMA中断
 * 
 * @param handle DMA句柄
 * @return int 0表示成功，非0表示失败
 */
int dma_disable_interrupt(dma_handle_t handle)
{
    stm32_dma_device_t *dev = (stm32_dma_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 禁用DMA传输完成中断 */
    __HAL_DMA_DISABLE_IT(&dev->hdma, DMA_IT_TC);
    
    /* 禁用DMA传输错误中断 */
    __HAL_DMA_DISABLE_IT(&dev->hdma, DMA_IT_TE);
    
    return DRIVER_OK;
}

/**
 * @brief DMA1 Stream0中断处理函数
 */
void DMA1_Stream0_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[0]);
}

/**
 * @brief DMA1 Stream1中断处理函数
 */
void DMA1_Stream1_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[1]);
}

/**
 * @brief DMA1 Stream2中断处理函数
 */
void DMA1_Stream2_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[2]);
}

/**
 * @brief DMA1 Stream3中断处理函数
 */
void DMA1_Stream3_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[3]);
}

/**
 * @brief DMA1 Stream4中断处理函数
 */
void DMA1_Stream4_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[4]);
}

/**
 * @brief DMA1 Stream5中断处理函数
 */
void DMA1_Stream5_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[5]);
}

/**
 * @brief DMA1 Stream6中断处理函数
 */
void DMA1_Stream6_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[6]);
}

/**
 * @brief DMA1 Stream7中断处理函数
 */
void DMA1_Stream7_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[7]);
}

/**
 * @brief DMA2 Stream0中断处理函数
 */
void DMA2_Stream0_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[8]);
}

/**
 * @brief DMA2 Stream1中断处理函数
 */
void DMA2_Stream1_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[9]);
}

/**
 * @brief DMA2 Stream2中断处理函数
 */
void DMA2_Stream2_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[10]);
}

/**
 * @brief DMA2 Stream3中断处理函数
 */
void DMA2_Stream3_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[11]);
}

/**
 * @brief DMA2 Stream4中断处理函数
 */
void DMA2_Stream4_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[12]);
}

/**
 * @brief DMA2 Stream5中断处理函数
 */
void DMA2_Stream5_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[13]);
}

/**
 * @brief DMA2 Stream6中断处理函数
 */
void DMA2_Stream6_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[14]);
}

/**
 * @brief DMA2 Stream7中断处理函数
 */
void DMA2_Stream7_IRQHandler(void)
{
    dma_irq_handler(&g_dma_devices[15]);
}

