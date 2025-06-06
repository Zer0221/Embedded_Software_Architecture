/**
 * @file fm33lc0xx_timer.c
 * @brief FM33LC0xx平台定时器驱动实�?
 *
 * 该文件实现了FM33LC0xx平台的定时器驱动接口
 */

#include "base/timer_api.h"
#include "common/error_api.h"
#include "fm33lc0xx_fl.h"
#include <string.h>

/* 驱动版本�?*/
#define FM33LC0XX_TIMER_DRIVER_VERSION "1.0.0"

/* BSTIM定时�?*/
#define BSTIM32      BSTIM32 /* 基本定时�?*/

/* GPTIM定时�?*/
#define GPTIM0       GPTIM0  /* 通用定时�? */
#define GPTIM1       GPTIM1  /* 通用定时�? */

/* ATIM定时�?*/
#define ATIM         ATIM    /* 高级定时�?*/

/* 定时器ID */
typedef enum {
    TIMER_ID_BSTIM32 = 0,  /* 基本定时�?*/
    TIMER_ID_GPTIM0,       /* 通用定时�? */
    TIMER_ID_GPTIM1,       /* 通用定时�? */
    TIMER_ID_ATIM,         /* 高级定时�?*/
    TIMER_ID_MAX           /* 定时器数�?*/
} timer_id_enum_t;

/* 定时器设备结构体 */
typedef struct {
    timer_config_t config;         /* 定时器配�?*/
    timer_callback_t callback;     /* 回调函数 */
    void *arg;                     /* 回调参数 */
    bool initialized;              /* 初始化标�?*/
    bool running;                  /* 运行标志 */
    void *timer_instance;          /* 定时器实例指�?*/
    uint32_t timer_id;             /* 定时器ID */
} fm33lc0xx_timer_device_t;

/* 定时器设备数�?*/
static fm33lc0xx_timer_device_t g_timer_devices[TIMER_ID_MAX];

/* 定时器实例指针数�?*/
static void* const TIMER_INSTANCES[] = {
    BSTIM32, /* 基本定时�?*/
    GPTIM0,  /* 通用定时�? */
    GPTIM1,  /* 通用定时�? */
    ATIM     /* 高级定时�?*/
};

/* 定时器中断处理函数声�?*/
static void BSTIM32_IRQHandler_Impl(void);
static void GPTIM0_IRQHandler_Impl(void);
static void GPTIM1_IRQHandler_Impl(void);
static void ATIM_IRQHandler_Impl(void);

/* BSTIM32中断处理函数 */
void BSTIM32_IRQHandler(void)
{
    BSTIM32_IRQHandler_Impl();
}

/* GPTIM0中断处理函数 */
void GPTIM0_IRQHandler(void)
{
    GPTIM0_IRQHandler_Impl();
}

/* GPTIM1中断处理函数 */
void GPTIM1_IRQHandler(void)
{
    GPTIM1_IRQHandler_Impl();
}

/* ATIM中断处理函数 */
void ATIM_IRQHandler(void)
{
    ATIM_IRQHandler_Impl();
}

/* BSTIM32中断处理实现 */
static void BSTIM32_IRQHandler_Impl(void)
{
    /* 检查溢出中�?*/
    if (FL_BSTIM32_IsEnabledIT_Update(BSTIM32) && FL_BSTIM32_IsActiveFlag_Update(BSTIM32)) {
        /* 清除中断标志 */
        FL_BSTIM32_ClearFlag_Update(BSTIM32);
        
        /* 获取设备实例 */
        fm33lc0xx_timer_device_t *device = &g_timer_devices[TIMER_ID_BSTIM32];
        
        /* 如果是单次触发模式，停止定时�?*/
        if (device->config.mode == TIMER_MODE_ONE_SHOT) {
            FL_BSTIM32_Disable(BSTIM32);
            device->running = false;
        }
        
        /* 调用回调函数 */
        if (device->callback != NULL) {
            device->callback(device->arg);
        }
    }
}

/* GPTIM0中断处理实现 */
static void GPTIM0_IRQHandler_Impl(void)
{
    /* 检查溢出中�?*/
    if (FL_GPTIM_IsEnabledIT_Update(GPTIM0) && FL_GPTIM_IsActiveFlag_Update(GPTIM0)) {
        /* 清除中断标志 */
        FL_GPTIM_ClearFlag_Update(GPTIM0);
        
        /* 获取设备实例 */
        fm33lc0xx_timer_device_t *device = &g_timer_devices[TIMER_ID_GPTIM0];
        
        /* 如果是单次触发模式，停止定时�?*/
        if (device->config.mode == TIMER_MODE_ONE_SHOT) {
            FL_GPTIM_Disable(GPTIM0);
            device->running = false;
        }
        
        /* 调用回调函数 */
        if (device->callback != NULL) {
            device->callback(device->arg);
        }
    }
}

/* GPTIM1中断处理实现 */
static void GPTIM1_IRQHandler_Impl(void)
{
    /* 检查溢出中�?*/
    if (FL_GPTIM_IsEnabledIT_Update(GPTIM1) && FL_GPTIM_IsActiveFlag_Update(GPTIM1)) {
        /* 清除中断标志 */
        FL_GPTIM_ClearFlag_Update(GPTIM1);
        
        /* 获取设备实例 */
        fm33lc0xx_timer_device_t *device = &g_timer_devices[TIMER_ID_GPTIM1];
        
        /* 如果是单次触发模式，停止定时�?*/
        if (device->config.mode == TIMER_MODE_ONE_SHOT) {
            FL_GPTIM_Disable(GPTIM1);
            device->running = false;
        }
        
        /* 调用回调函数 */
        if (device->callback != NULL) {
            device->callback(device->arg);
        }
    }
}

/* ATIM中断处理实现 */
static void ATIM_IRQHandler_Impl(void)
{
    /* 检查溢出中�?*/
    if (FL_ATIM_IsEnabledIT_Update(ATIM) && FL_ATIM_IsActiveFlag_Update(ATIM)) {
        /* 清除中断标志 */
        FL_ATIM_ClearFlag_Update(ATIM);
        
        /* 获取设备实例 */
        fm33lc0xx_timer_device_t *device = &g_timer_devices[TIMER_ID_ATIM];
        
        /* 如果是单次触发模式，停止定时�?*/
        if (device->config.mode == TIMER_MODE_ONE_SHOT) {
            FL_ATIM_Disable(ATIM);
            device->running = false;
        }
        
        /* 调用回调函数 */
        if (device->callback != NULL) {
            device->callback(device->arg);
        }
    }
}

/* 将周�?微秒)转换为计数值和预分频�?*/
static void convert_period_to_counter(uint32_t period_us, uint32_t *counter, uint32_t *prescaler)
{
    uint32_t clock_freq = SystemCoreClock; /* 系统时钟频率 */
    uint32_t max_counter = 0xFFFF; /* 16位计数器 */
    
    /* 计算总时钟周期数 */
    uint64_t total_clocks = ((uint64_t)period_us * (uint64_t)clock_freq) / 1000000UL;
    
    /* 计算预分频值和计数�?*/
    *prescaler = 1;
    while (total_clocks > max_counter) {
        (*prescaler)++;
        total_clocks = ((uint64_t)period_us * (uint64_t)clock_freq) / (1000000UL * (*prescaler));
    }
    
    /* 确保预分频值不超过最大�?*/
    if (*prescaler > 0xFFFF) {
        *prescaler = 0xFFFF;
        total_clocks = ((uint64_t)period_us * (uint64_t)clock_freq) / (1000000UL * (*prescaler));
    }
    
    /* 设置计数�?*/
    *counter = (uint32_t)total_clocks;
    
    /* 确保计数值至少为1 */
    if (*counter == 0) {
        *counter = 1;
    }
}

/**
 * @brief 初始化定时器
 * 
 * @param timer_id 定时器ID
 * @param config 定时器配置参�?
 * @param callback 定时器回调函�?
 * @param arg 传递给回调函数的参�?
 * @param handle 定时器句柄指�?
 * @return int 0表示成功，非0表示失败
 */
int timer_init(uint32_t timer_id, const timer_config_t *config, 
               timer_callback_t callback, void *arg, timer_handle_t *handle)
{
    /* 检查参数有效�?*/
    if (config == NULL || handle == NULL || timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 获取设备实例 */
    fm33lc0xx_timer_device_t *device = &g_timer_devices[timer_id];
    
    /* 检查是否已初始�?*/
    if (device->initialized) {
        return DRIVER_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置和回�?*/
    memcpy(&device->config, config, sizeof(timer_config_t));
    device->callback = callback;
    device->arg = arg;
    device->timer_instance = TIMER_INSTANCES[timer_id];
    device->timer_id = timer_id;
    device->running = false;
    
    /* 使能定时器时�?*/
    switch (timer_id) {
        case TIMER_ID_BSTIM32:
            FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_BSTIM);
            break;
        case TIMER_ID_GPTIM0:
            FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 计算计数值和预分频�?*/
    uint32_t counter, prescaler;
    convert_period_to_counter(config->period_us, &counter, &prescaler);
    
    /* 根据定时器类型初始化 */
    switch (timer_id) {
        case TIMER_ID_BSTIM32:
        {
            /* BSTIM32初始化结构体 */
            FL_BSTIM32_InitTypeDef BSTIM32_InitStruct;
            
            /* 基本配置 */
            BSTIM32_InitStruct.prescaler = prescaler - 1;
            BSTIM32_InitStruct.autoReload = counter - 1;
            BSTIM32_InitStruct.autoReloadState = FL_ENABLE;
            BSTIM32_InitStruct.clockSource = FL_RCC_BSTIM32_CLK_SOURCE_APBCLK;
            
            /* 初始化BSTIM32 */
            FL_BSTIM32_Init(BSTIM32, &BSTIM32_InitStruct);
            
            /* 清除更新标志 */
            FL_BSTIM32_ClearFlag_Update(BSTIM32);
            
            /* 使能更新中断 */
            FL_BSTIM32_EnableIT_Update(BSTIM32);
            
            /* 配置NVIC */
            NVIC_EnableIRQ(BSTIM_IRQn);
            NVIC_SetPriority(BSTIM_IRQn, 3);
            
            break;
        }
        case TIMER_ID_GPTIM0:
        case TIMER_ID_GPTIM1:
        {
            /* 获取GPTIM实例 */
            GPTIM_Type *GPTIMx = (GPTIM_Type *)device->timer_instance;
            
            /* GPTIM初始化结构体 */
            FL_GPTIM_InitTypeDef GPTIM_InitStruct;
            
            /* 基本配置 */
            GPTIM_InitStruct.prescaler = prescaler - 1;
            GPTIM_InitStruct.counterMode = FL_GPTIM_COUNTER_MODE_UP;
            GPTIM_InitStruct.autoReload = counter - 1;
            GPTIM_InitStruct.clockDivision = FL_GPTIM_CLK_DIVISION_DIV1;
            GPTIM_InitStruct.autoReloadState = FL_ENABLE;
            
            /* 初始化GPTIM */
            FL_GPTIM_Init(GPTIMx, &GPTIM_InitStruct);
            
            /* 清除更新标志 */
            FL_GPTIM_ClearFlag_Update(GPTIMx);
            
            /* 使能更新中断 */
            FL_GPTIM_EnableIT_Update(GPTIMx);
            
            /* 配置NVIC */
            if (timer_id == TIMER_ID_GPTIM0) {
                NVIC_EnableIRQ(GPTIM0_IRQn);
                NVIC_SetPriority(GPTIM0_IRQn, 3);
            } else {
                NVIC_EnableIRQ(GPTIM1_IRQn);
                NVIC_SetPriority(GPTIM1_IRQn, 3);
            }
            
            break;
        }
        case TIMER_ID_ATIM:
        {
            /* ATIM初始化结构体 */
            FL_ATIM_InitTypeDef ATIM_InitStruct;
            
            /* 基本配置 */
            ATIM_InitStruct.prescaler = prescaler - 1;
            ATIM_InitStruct.counterMode = FL_ATIM_COUNTER_MODE_UP;
            ATIM_InitStruct.autoReload = counter - 1;
            ATIM_InitStruct.clockDivision = FL_ATIM_CLK_DIVISION_DIV1;
            ATIM_InitStruct.autoReloadState = FL_ENABLE;
            ATIM_InitStruct.repCounterState = FL_DISABLE;
            ATIM_InitStruct.repCounter = 0;
            
            /* 初始化ATIM */
            FL_ATIM_Init(ATIM, &ATIM_InitStruct);
            
            /* 清除更新标志 */
            FL_ATIM_ClearFlag_Update(ATIM);
            
            /* 使能更新中断 */
            FL_ATIM_EnableIT_Update(ATIM);
            
            /* 配置NVIC */
            NVIC_EnableIRQ(ATIM_IRQn);
            NVIC_SetPriority(ATIM_IRQn, 3);
            
            break;
        }
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 标记为已初始�?*/
    device->initialized = true;
    
    /* 返回句柄 */
    *handle = (timer_handle_t)device;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化定时�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_deinit(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 停止定时�?*/
    timer_stop(handle);
    
    /* 禁用中断 */
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_DisableIT_Update(BSTIM32);
            NVIC_DisableIRQ(BSTIM_IRQn);
            FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_BSTIM);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_DisableIT_Update(GPTIM0);
            NVIC_DisableIRQ(GPTIM0_IRQn);
            FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_DisableIT_Update(GPTIM1);
            NVIC_DisableIRQ(GPTIM1_IRQn);
            FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_DisableIT_Update(ATIM);
            NVIC_DisableIRQ(ATIM_IRQn);
            FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 清除设备状�?*/
    device->initialized = false;
    device->running = false;
    device->callback = NULL;
    device->arg = NULL;
    
    return DRIVER_OK;
}

/**
 * @brief 启动定时�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_start(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 如果已经运行，直接返�?*/
    if (device->running) {
        return DRIVER_OK;
    }
    
    /* 根据定时器类型启�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_Enable(BSTIM32);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_Enable(GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_Enable(GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_Enable(ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 标记为运行状�?*/
    device->running = true;
    
    return DRIVER_OK;
}

/**
 * @brief 停止定时�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_stop(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 如果已经停止，直接返�?*/
    if (!device->running) {
        return DRIVER_OK;
    }
    
    /* 根据定时器类型停�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_Disable(BSTIM32);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_Disable(GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_Disable(GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_Disable(ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 标记为停止状�?*/
    device->running = false;
    
    return DRIVER_OK;
}

/**
 * @brief 设置定时器周�?
 * 
 * @param handle 定时器句�?
 * @param period_us 定时周期(微秒)
 * @return int 0表示成功，非0表示失败
 */
int timer_set_period(timer_handle_t handle, uint32_t period_us)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 计算计数值和预分频�?*/
    uint32_t counter, prescaler;
    convert_period_to_counter(period_us, &counter, &prescaler);
    
    /* 是否需要暂停定时器 */
    bool was_running = device->running;
    if (was_running) {
        timer_stop(handle);
    }
    
    /* 根据定时器类型设置周�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_WritePrescaler(BSTIM32, prescaler - 1);
            FL_BSTIM32_WriteAutoReload(BSTIM32, counter - 1);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_WritePrescaler(GPTIM0, prescaler - 1);
            FL_GPTIM_WriteAutoReload(GPTIM0, counter - 1);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_WritePrescaler(GPTIM1, prescaler - 1);
            FL_GPTIM_WriteAutoReload(GPTIM1, counter - 1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_WritePrescaler(ATIM, prescaler - 1);
            FL_ATIM_WriteAutoReload(ATIM, counter - 1);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 更新配置 */
    device->config.period_us = period_us;
    
    /* 如果之前在运行，重新启动 */
    if (was_running) {
        timer_start(handle);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取定时器计数�?
 * 
 * @param handle 定时器句�?
 * @param value 计数值指�?
 * @return int 0表示成功，非0表示失败
 */
int timer_get_count(timer_handle_t handle, uint32_t *value)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX || value == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 根据定时器类型获取计数�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            *value = FL_BSTIM32_ReadCounter(BSTIM32);
            break;
        case TIMER_ID_GPTIM0:
            *value = FL_GPTIM_ReadCounter(GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            *value = FL_GPTIM_ReadCounter(GPTIM1);
            break;
        case TIMER_ID_ATIM:
            *value = FL_ATIM_ReadCounter(ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置定时器计数�?
 * 
 * @param handle 定时器句�?
 * @param value 计数�?
 * @return int 0表示成功，非0表示失败
 */
int timer_set_count(timer_handle_t handle, uint32_t value)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 根据定时器类型设置计数�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_WriteCounter(BSTIM32, value);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_WriteCounter(GPTIM0, value);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_WriteCounter(GPTIM1, value);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_WriteCounter(ATIM, value);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置定时器预分频�?
 * 
 * @param handle 定时器句�?
 * @param prescaler 预分频�?
 * @return int 0表示成功，非0表示失败
 */
int timer_set_prescaler(timer_handle_t handle, uint32_t prescaler)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查预分频�?*/
    if (prescaler == 0 || prescaler > 0xFFFF) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 是否需要暂停定时器 */
    bool was_running = device->running;
    if (was_running) {
        timer_stop(handle);
    }
    
    /* 根据定时器类型设置预分频�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_WritePrescaler(BSTIM32, prescaler - 1);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_WritePrescaler(GPTIM0, prescaler - 1);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_WritePrescaler(GPTIM1, prescaler - 1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_WritePrescaler(ATIM, prescaler - 1);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 如果之前在运行，重新启动 */
    if (was_running) {
        timer_start(handle);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置定时器回调函�?
 * 
 * @param handle 定时器句�?
 * @param callback 定时器回调函�?
 * @param arg 传递给回调函数的参�?
 * @return int 0表示成功，非0表示失败
 */
int timer_set_callback(timer_handle_t handle, timer_callback_t callback, void *arg)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 设置回调函数和参�?*/
    device->callback = callback;
    device->arg = arg;
    
    return DRIVER_OK;
}

/**
 * @brief 使能定时器中�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_enable_interrupt(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 根据定时器类型使能中�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_EnableIT_Update(BSTIM32);
            NVIC_EnableIRQ(BSTIM_IRQn);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_EnableIT_Update(GPTIM0);
            NVIC_EnableIRQ(GPTIM0_IRQn);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_EnableIT_Update(GPTIM1);
            NVIC_EnableIRQ(GPTIM1_IRQn);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_EnableIT_Update(ATIM);
            NVIC_EnableIRQ(ATIM_IRQn);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 禁用定时器中�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_disable_interrupt(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 根据定时器类型禁用中�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_DisableIT_Update(BSTIM32);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_DisableIT_Update(GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_DisableIT_Update(GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_DisableIT_Update(ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 清除定时器中断标�?
 * 
 * @param handle 定时器句�?
 * @return int 0表示成功，非0表示失败
 */
int timer_clear_interrupt_flag(timer_handle_t handle)
{
    fm33lc0xx_timer_device_t *device = (fm33lc0xx_timer_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device == NULL || !device->initialized || device->timer_id >= TIMER_ID_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 根据定时器类型清除中断标�?*/
    switch (device->timer_id) {
        case TIMER_ID_BSTIM32:
            FL_BSTIM32_ClearFlag_Update(BSTIM32);
            break;
        case TIMER_ID_GPTIM0:
            FL_GPTIM_ClearFlag_Update(GPTIM0);
            break;
        case TIMER_ID_GPTIM1:
            FL_GPTIM_ClearFlag_Update(GPTIM1);
            break;
        case TIMER_ID_ATIM:
            FL_ATIM_ClearFlag_Update(ATIM);
            break;
        default:
            return DRIVER_ERROR_UNSUPPORTED;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 延时指定微秒�?
 * 
 * @param us 微秒�?
 * @return int 0表示成功，非0表示失败
 */
int timer_delay_us(uint32_t us)
{
    uint32_t start_tick = FL_BSTIM32_ReadCounter(BSTIM32);
    uint32_t delay_ticks = (us * (SystemCoreClock / 1000000));
    uint32_t current_tick;
    
    while (1) {
        current_tick = FL_BSTIM32_ReadCounter(BSTIM32);
        if ((current_tick - start_tick) >= delay_ticks) {
            break;
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 延时指定毫秒�?
 * 
 * @param ms 毫秒�?
 * @return int 0表示成功，非0表示失败
 */
int timer_delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        timer_delay_us(1000);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取定时器驱动版�?
 * 
 * @return const char* 版本字符�?
 */
const char* timer_get_version(void)
{
    return FM33LC0XX_TIMER_DRIVER_VERSION;
}

