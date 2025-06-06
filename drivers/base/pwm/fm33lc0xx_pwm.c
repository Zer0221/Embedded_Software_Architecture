/**
 * @file fm33lc0xx_pwm.c
 * @brief FM33LC0xx平台PWM驱动实现
 *
 * 该文件实现了FM33LC0xx平台的PWM驱动接口
 */

#include "base/pwm_api.h"
#include "common/error_api.h"
#include "fm33lc0xx_fl.h"
#include <string.h>

/* 驱动版本�?*/
#define FM33LC0XX_PWM_DRIVER_VERSION "1.0.0"

/* PWM通道映射�?*/
static const struct {
    ATIM_Type *timer;
    uint32_t channel;
} PWM_CHANNEL_MAP[] = {
    {ATIM, FL_ATIM_CHANNEL_1},      /* PWM_CHANNEL_0 - ATIM通道1 */
    {ATIM, FL_ATIM_CHANNEL_2},      /* PWM_CHANNEL_1 - ATIM通道2 */
    {ATIM, FL_ATIM_CHANNEL_3},      /* PWM_CHANNEL_2 - ATIM通道3 */
    {ATIM, FL_ATIM_CHANNEL_4},      /* PWM_CHANNEL_3 - ATIM通道4 */
    {GPTIM0, FL_GPTIM_CHANNEL_1},   /* PWM_CHANNEL_4 - GPTIM0通道1 */
    {GPTIM0, FL_GPTIM_CHANNEL_2},   /* PWM_CHANNEL_5 - GPTIM0通道2 */
    {GPTIM1, FL_GPTIM_CHANNEL_1},   /* PWM_CHANNEL_6 - GPTIM1通道1 */
    {GPTIM1, FL_GPTIM_CHANNEL_2}    /* PWM_CHANNEL_7 - GPTIM1通道2 */
};

/* PWM对齐模式映射�?*/
static const uint32_t PWM_ALIGN_MODE_MAP[] = {
    FL_ATIM_COUNTER_ALIGNED_EDGE,    /* PWM_ALIGN_EDGE */
    FL_ATIM_COUNTER_ALIGNED_CENTER_DOWN  /* PWM_ALIGN_CENTER */
};

/* PWM极性映射表 */
static const uint32_t PWM_POLARITY_MAP[] = {
    FL_ATIM_OC_POLARITY_NORMAL,    /* PWM_POLARITY_NORMAL */
    FL_ATIM_OC_POLARITY_INVERT     /* PWM_POLARITY_INVERTED */
};

/* PWM计数器模式映射表 */
static const uint32_t PWM_COUNTER_MODE_MAP[] = {
    FL_ATIM_COUNTER_DIR_UP,        /* PWM_COUNTER_UP */
    FL_ATIM_COUNTER_DIR_DOWN,      /* PWM_COUNTER_DOWN */
    FL_ATIM_COUNTER_DIR_UP_DOWN    /* PWM_COUNTER_UP_DOWN */
};

/* PWM设备实例结构�?*/
typedef struct {
    pwm_config_t config;         /* PWM配置参数 */
    bool initialized;            /* 初始化标�?*/
    pwm_callback_t callback;     /* 事件回调函数 */
    void *user_data;             /* 用户数据指针 */
    uint32_t period;             /* 周期�?*/
    uint32_t pulse;              /* 脉冲宽度�?*/
    bool is_atim;                /* 是否为高级定时器标志 */
} fm33lc0xx_pwm_device_t;

/* 全局设备实例数组 */
static fm33lc0xx_pwm_device_t g_pwm_devices[PWM_CHANNEL_MAX];

/* 定时器中断处理函数原�?*/
static void atim_irq_handler(void);
static void gptim0_irq_handler(void);
static void gptim1_irq_handler(void);

/* 高级定时器中断处理函�?*/
void ATIM_IRQHandler(void)
{
    atim_irq_handler();
}

/* 通用定时�?中断处理函数 */
void GPTIM0_IRQHandler(void)
{
    gptim0_irq_handler();
}

/* 通用定时�?中断处理函数 */
void GPTIM1_IRQHandler(void)
{
    gptim1_irq_handler();
}

/* 高级定时器中断处理实�?*/
static void atim_irq_handler(void)
{
    /* 检查更新中�?*/
    if (FL_ATIM_IsEnabledIT_Update(ATIM) && FL_ATIM_IsActiveFlag_Update(ATIM)) {
        /* 清除中断标志 */
        FL_ATIM_ClearFlag_Update(ATIM);
        
        /* 调用通道0-3的回调函�?*/
        for (int i = 0; i < 4; i++) {
            if (g_pwm_devices[i].initialized && g_pwm_devices[i].callback != NULL) {
                g_pwm_devices[i].callback(g_pwm_devices[i].user_data);
            }
        }
    }
}

/* 通用定时�?中断处理实现 */
static void gptim0_irq_handler(void)
{
    /* 检查更新中�?*/
    if (FL_GPTIM_IsEnabledIT_Update(GPTIM0) && FL_GPTIM_IsActiveFlag_Update(GPTIM0)) {
        /* 清除中断标志 */
        FL_GPTIM_ClearFlag_Update(GPTIM0);
        
        /* 调用通道4-5的回调函�?*/
        for (int i = 4; i < 6; i++) {
            if (g_pwm_devices[i].initialized && g_pwm_devices[i].callback != NULL) {
                g_pwm_devices[i].callback(g_pwm_devices[i].user_data);
            }
        }
    }
}

/* 通用定时�?中断处理实现 */
static void gptim1_irq_handler(void)
{
    /* 检查更新中�?*/
    if (FL_GPTIM_IsEnabledIT_Update(GPTIM1) && FL_GPTIM_IsActiveFlag_Update(GPTIM1)) {
        /* 清除中断标志 */
        FL_GPTIM_ClearFlag_Update(GPTIM1);
        
        /* 调用通道6-7的回调函�?*/
        for (int i = 6; i < 8; i++) {
            if (g_pwm_devices[i].initialized && g_pwm_devices[i].callback != NULL) {
                g_pwm_devices[i].callback(g_pwm_devices[i].user_data);
            }
        }
    }
}

/**
 * @brief 计算定时器的分频和周期�?
 * 
 * @param frequency 目标频率(Hz)
 * @param prescaler 计算得到的分频�?
 * @param period 计算得到的周期�?
 * @return bool 是否计算成功
 */
static bool calculate_timer_params(uint32_t frequency, uint16_t *prescaler, uint32_t *period)
{
    if (frequency == 0) {
        return false;
    }
    
    uint32_t timer_clock = SystemCoreClock;  /* 默认使用系统时钟 */
    uint32_t max_period = 0xFFFF;           /* 16位定时器 */
    
    /* 计算可能的分频值和周期�?*/
    uint32_t cycles = timer_clock / frequency;
    
    /* 找到合适的分频�?*/
    *prescaler = 1;
    while (cycles > max_period) {
        (*prescaler)++;
        cycles = timer_clock / (frequency * (*prescaler));
        
        /* 防止分频值溢�?*/
        if (*prescaler >= 0xFFFF) {
            *prescaler = 0xFFFF;
            cycles = timer_clock / (frequency * (*prescaler));
            break;
        }
    }
    
    *period = cycles;
    
    return true;
}

/**
 * @brief 初始化PWM
 * 
 * @param config PWM配置参数
 * @param handle PWM设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int pwm_init(const pwm_config_t *config, pwm_handle_t *handle)
{
    if (config == NULL || handle == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查参数有效�?*/
    if (config->channel >= PWM_CHANNEL_MAX) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->frequency == 0 || config->duty_cycle < 0.0f || config->duty_cycle > 1.0f) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 获取设备实例 */
    fm33lc0xx_pwm_device_t *device = &g_pwm_devices[config->channel];
    
    /* 防止重复初始�?*/
    if (device->initialized) {
        return DRIVER_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置参数 */
    memcpy(&device->config, config, sizeof(pwm_config_t));
    device->callback = NULL;
    device->user_data = NULL;
    
    /* 判断是否为高级定时器 */
    device->is_atim = (config->channel < 4);
    
    /* 使能相应的时�?*/
    if (device->is_atim) {
        FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ATIM);
    } else if (config->channel < 6) {
        FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM0);
    } else {
        FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_GPTIM1);
    }
    
    /* 计算定时器参�?*/
    uint16_t prescaler;
    uint32_t period;
    if (!calculate_timer_params(config->frequency, &prescaler, &period)) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    device->period = period;
    device->pulse = (uint32_t)(period * config->duty_cycle);
    
    /* 根据通道类型选择初始化方�?*/
    if (device->is_atim) {
        /* 高级定时器初始化 */
        FL_ATIM_InitTypeDef ATIM_InitStruct;
        FL_ATIM_OC_InitTypeDef ATIM_OC_InitStruct;
        
        /* 定时器基本参数配�?*/
        ATIM_InitStruct.clockSource = FL_RCC_ATIM_CLK_SOURCE_APBCLK;
        ATIM_InitStruct.prescaler = prescaler - 1;  /* 预分频值从0开始计�?*/
        ATIM_InitStruct.counterMode = PWM_COUNTER_MODE_MAP[config->counter_mode];
        ATIM_InitStruct.autoReload = period - 1;    /* 自动重载�?*/
        ATIM_InitStruct.clockDivision = FL_ATIM_CLK_DIVISION_DIV1;
        ATIM_InitStruct.repetitionCounter = 0;
        ATIM_InitStruct.autoReloadState = FL_ENABLE;
        ATIM_InitStruct.bufferState = FL_ENABLE;
        
        FL_ATIM_Init(ATIM, &ATIM_InitStruct);
        
        /* 输出通道配置 */
        ATIM_OC_InitStruct.OCMode = FL_ATIM_OC_MODE_PWM1;
        ATIM_OC_InitStruct.OCState = FL_ENABLE;
        ATIM_OC_InitStruct.OCNState = config->complementary ? FL_ENABLE : FL_DISABLE;
        ATIM_OC_InitStruct.OCPolarity = PWM_POLARITY_MAP[config->polarity];
        ATIM_OC_InitStruct.OCNPolarity = PWM_POLARITY_MAP[config->polarity];
        ATIM_OC_InitStruct.OCIdleState = FL_ATIM_OC_IDLE_STATE_LOW;
        ATIM_OC_InitStruct.OCNIdleState = FL_ATIM_OCN_IDLE_STATE_LOW;
        
        /* 计算脉冲宽度 */
        ATIM_OC_InitStruct.compareValue = device->pulse;
        
        /* 根据通道配置输出比较 */
        switch (PWM_CHANNEL_MAP[config->channel].channel) {
            case FL_ATIM_CHANNEL_1:
                FL_ATIM_OC_Init(ATIM, FL_ATIM_CHANNEL_1, &ATIM_OC_InitStruct);
                break;
            case FL_ATIM_CHANNEL_2:
                FL_ATIM_OC_Init(ATIM, FL_ATIM_CHANNEL_2, &ATIM_OC_InitStruct);
                break;
            case FL_ATIM_CHANNEL_3:
                FL_ATIM_OC_Init(ATIM, FL_ATIM_CHANNEL_3, &ATIM_OC_InitStruct);
                break;
            case FL_ATIM_CHANNEL_4:
                FL_ATIM_OC_Init(ATIM, FL_ATIM_CHANNEL_4, &ATIM_OC_InitStruct);
                break;
            default:
                break;
        }
        
        /* 配置死区时间(如果启用互补输出) */
        if (config->complementary) {
            uint32_t dead_time_cycles = (config->dead_time * SystemCoreClock) / 1000000000;
            FL_ATIM_SetDeadTime(ATIM, dead_time_cycles);
        }
        
    } else {
        /* 通用定时器初始化 */
        FL_GPTIM_InitTypeDef GPTIM_InitStruct;
        FL_GPTIM_OC_InitTypeDef GPTIM_OC_InitStruct;
        
        /* 获取对应的定时器 */
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[config->channel].timer;
        
        /* 定时器基本参数配�?*/
        GPTIM_InitStruct.prescaler = prescaler - 1;  /* 预分频值从0开始计�?*/
        GPTIM_InitStruct.counterMode = FL_GPTIM_COUNTER_DIR_UP; /* 通用定时器只支持向上计数 */
        GPTIM_InitStruct.autoReload = period - 1;    /* 自动重载�?*/
        GPTIM_InitStruct.clockDivision = FL_GPTIM_CLK_DIVISION_DIV1;
        GPTIM_InitStruct.autoReloadState = FL_ENABLE;
        
        FL_GPTIM_Init(TIMx, &GPTIM_InitStruct);
        
        /* 输出通道配置 */
        GPTIM_OC_InitStruct.OCMode = FL_GPTIM_OC_MODE_PWM1;
        GPTIM_OC_InitStruct.OCState = FL_ENABLE;
        GPTIM_OC_InitStruct.OCPolarity = PWM_POLARITY_MAP[config->polarity];
        GPTIM_OC_InitStruct.OCFastMode = FL_DISABLE;
        
        /* 计算脉冲宽度 */
        GPTIM_OC_InitStruct.compareValue = device->pulse;
        
        /* 根据通道配置输出比较 */
        uint32_t channel = PWM_CHANNEL_MAP[config->channel].channel;
        FL_GPTIM_OC_Init(TIMx, channel, &GPTIM_OC_InitStruct);
    }
    
    /* 标记初始化完�?*/
    device->initialized = true;
    
    /* 返回设备句柄 */
    *handle = (pwm_handle_t)device;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化PWM
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_deinit(pwm_handle_t handle)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    uint8_t channel = device->config.channel;
    
    /* 禁用定时�?*/
    if (device->is_atim) {
        /* 禁用通道 */
        switch (PWM_CHANNEL_MAP[channel].channel) {
            case FL_ATIM_CHANNEL_1:
                FL_ATIM_OC_DisableChannel(ATIM, FL_ATIM_CHANNEL_1);
                break;
            case FL_ATIM_CHANNEL_2:
                FL_ATIM_OC_DisableChannel(ATIM, FL_ATIM_CHANNEL_2);
                break;
            case FL_ATIM_CHANNEL_3:
                FL_ATIM_OC_DisableChannel(ATIM, FL_ATIM_CHANNEL_3);
                break;
            case FL_ATIM_CHANNEL_4:
                FL_ATIM_OC_DisableChannel(ATIM, FL_ATIM_CHANNEL_4);
                break;
            default:
                break;
        }
        
        /* 禁用定时�?*/
        FL_ATIM_Disable(ATIM);
        
        /* 禁用更新中断 */
        FL_ATIM_DisableIT_Update(ATIM);
        
    } else {
        /* 获取对应的定时器 */
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[channel].timer;
        
        /* 禁用通道 */
        uint32_t tim_channel = PWM_CHANNEL_MAP[channel].channel;
        FL_GPTIM_OC_DisableChannel(TIMx, tim_channel);
        
        /* 禁用定时�?*/
        FL_GPTIM_Disable(TIMx);
        
        /* 禁用更新中断 */
        FL_GPTIM_DisableIT_Update(TIMx);
    }
    
    /* 重置设备状�?*/
    device->initialized = false;
    device->callback = NULL;
    device->user_data = NULL;
    
    return DRIVER_OK;
}

/**
 * @brief 启动PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_start(pwm_handle_t handle)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 启动定时�?*/
    if (device->is_atim) {
        FL_ATIM_Enable(ATIM);
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        FL_GPTIM_Enable(TIMx);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 停止PWM输出
 * 
 * @param handle PWM设备句柄
 * @return int 0表示成功，非0表示失败
 */
int pwm_stop(pwm_handle_t handle)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 停止定时�?*/
    if (device->is_atim) {
        FL_ATIM_Disable(ATIM);
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        FL_GPTIM_Disable(TIMx);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置PWM频率
 * 
 * @param handle PWM设备句柄
 * @param frequency 频率(Hz)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_frequency(pwm_handle_t handle, uint32_t frequency)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    if (frequency == 0) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 计算定时器参�?*/
    uint16_t prescaler;
    uint32_t period;
    if (!calculate_timer_params(frequency, &prescaler, &period)) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 保存周期�?*/
    device->period = period;
    
    /* 重新计算脉冲宽度 */
    device->pulse = (uint32_t)(period * device->config.duty_cycle);
    
    /* 更新定时器参�?*/
    if (device->is_atim) {
        /* 临时停止定时�?*/
        bool was_enabled = FL_ATIM_IsEnabled(ATIM);
        if (was_enabled) {
            FL_ATIM_Disable(ATIM);
        }
        
        /* 更新分频和自动重载�?*/
        FL_ATIM_SetPrescaler(ATIM, prescaler - 1);
        FL_ATIM_WriteAutoReload(ATIM, period - 1);
        
        /* 更新比较�?*/
        switch (PWM_CHANNEL_MAP[device->config.channel].channel) {
            case FL_ATIM_CHANNEL_1:
                FL_ATIM_WriteCompareCH1(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_2:
                FL_ATIM_WriteCompareCH2(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_3:
                FL_ATIM_WriteCompareCH3(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_4:
                FL_ATIM_WriteCompareCH4(ATIM, device->pulse);
                break;
            default:
                break;
        }
        
        /* 生成更新事件 */
        FL_ATIM_GenerateUpdateEvent(ATIM);
        
        /* 恢复定时器状�?*/
        if (was_enabled) {
            FL_ATIM_Enable(ATIM);
        }
        
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        
        /* 临时停止定时�?*/
        bool was_enabled = FL_GPTIM_IsEnabled(TIMx);
        if (was_enabled) {
            FL_GPTIM_Disable(TIMx);
        }
        
        /* 更新分频和自动重载�?*/
        FL_GPTIM_SetPrescaler(TIMx, prescaler - 1);
        FL_GPTIM_WriteAutoReload(TIMx, period - 1);
        
        /* 更新比较�?*/
        uint32_t channel = PWM_CHANNEL_MAP[device->config.channel].channel;
        if (channel == FL_GPTIM_CHANNEL_1) {
            FL_GPTIM_WriteCompareCH1(TIMx, device->pulse);
        } else {
            FL_GPTIM_WriteCompareCH2(TIMx, device->pulse);
        }
        
        /* 生成更新事件 */
        FL_GPTIM_GenerateUpdateEvent(TIMx);
        
        /* 恢复定时器状�?*/
        if (was_enabled) {
            FL_GPTIM_Enable(TIMx);
        }
    }
    
    /* 更新配置 */
    device->config.frequency = frequency;
    
    return DRIVER_OK;
}

/**
 * @brief 设置PWM占空�?
 * 
 * @param handle PWM设备句柄
 * @param duty_cycle 占空�?0.0-1.0)
 * @return int 0表示成功，非0表示失败
 */
int pwm_set_duty_cycle(pwm_handle_t handle, float duty_cycle)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    if (duty_cycle < 0.0f || duty_cycle > 1.0f) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 保存占空�?*/
    device->config.duty_cycle = duty_cycle;
    
    /* 计算新的脉冲宽度 */
    device->pulse = (uint32_t)(device->period * duty_cycle);
    
    /* 更新比较�?*/
    if (device->is_atim) {
        switch (PWM_CHANNEL_MAP[device->config.channel].channel) {
            case FL_ATIM_CHANNEL_1:
                FL_ATIM_WriteCompareCH1(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_2:
                FL_ATIM_WriteCompareCH2(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_3:
                FL_ATIM_WriteCompareCH3(ATIM, device->pulse);
                break;
            case FL_ATIM_CHANNEL_4:
                FL_ATIM_WriteCompareCH4(ATIM, device->pulse);
                break;
            default:
                break;
        }
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        uint32_t channel = PWM_CHANNEL_MAP[device->config.channel].channel;
        
        if (channel == FL_GPTIM_CHANNEL_1) {
            FL_GPTIM_WriteCompareCH1(TIMx, device->pulse);
        } else {
            FL_GPTIM_WriteCompareCH2(TIMx, device->pulse);
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 注册PWM事件回调函数
 * 
 * @param handle PWM设备句柄
 * @param event 事件类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int pwm_register_event_callback(pwm_handle_t handle, pwm_event_t event, pwm_callback_t callback, void *user_data)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 目前只支持周期结束事�?*/
    if (event != PWM_EVENT_PERIOD_ELAPSED) {
        return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 保存回调函数和用户数�?*/
    device->callback = callback;
    device->user_data = user_data;
    
    /* 使能更新中断 */
    if (device->is_atim) {
        FL_ATIM_EnableIT_Update(ATIM);
        NVIC_EnableIRQ(ATIM_IRQn);
        NVIC_SetPriority(ATIM_IRQn, 3);
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        FL_GPTIM_EnableIT_Update(TIMx);
        
        if (TIMx == GPTIM0) {
            NVIC_EnableIRQ(GPTIM0_IRQn);
            NVIC_SetPriority(GPTIM0_IRQn, 3);
        } else {
            NVIC_EnableIRQ(GPTIM1_IRQn);
            NVIC_SetPriority(GPTIM1_IRQn, 3);
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 取消注册PWM事件回调函数
 * 
 * @param handle PWM设备句柄
 * @param event 事件类型
 * @return int 0表示成功，非0表示失败
 */
int pwm_unregister_event_callback(pwm_handle_t handle, pwm_event_t event)
{
    fm33lc0xx_pwm_device_t *device = (fm33lc0xx_pwm_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device < &g_pwm_devices[0] || device > &g_pwm_devices[PWM_CHANNEL_MAX - 1] || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 目前只支持周期结束事�?*/
    if (event != PWM_EVENT_PERIOD_ELAPSED) {
        return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 清除回调函数 */
    device->callback = NULL;
    device->user_data = NULL;
    
    /* 禁用更新中断 */
    if (device->is_atim) {
        FL_ATIM_DisableIT_Update(ATIM);
    } else {
        GPTIM_Type *TIMx = (GPTIM_Type *)PWM_CHANNEL_MAP[device->config.channel].timer;
        FL_GPTIM_DisableIT_Update(TIMx);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取PWM驱动版本
 * 
 * @return const char* 版本字符�?
 */
const char* pwm_get_version(void)
{
    return FM33LC0XX_PWM_DRIVER_VERSION;
}

