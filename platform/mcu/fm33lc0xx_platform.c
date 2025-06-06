/**
 * @file fm33lc0xx_platform.c
 * @brief FM33LC0xx平台适配层实现
 *
 * 该文件实现了FM33LC0xx平台的硬件抽象层接口
 */

#include "fm33lc0xx_platform.h"
#include "error_api.h"
#include <string.h>

/* FM33LC0xx芯片头文件 */
#include "fm33lc0xx_fl.h"

/* 平台名称 */
static const char PLATFORM_NAME[] = "FM33LC0xx";

/* 平台描述 */
static const char PLATFORM_DESCRIPTION[] = "FM33LC0xx MCU平台适配层";

/* 平台版本 */
static const char PLATFORM_VERSION[] = "1.0.0";

/* 平台运行时间(毫秒) */
static volatile uint32_t g_platform_uptime = 0;

/**
 * @brief 初始化FM33LC0xx平台
 * 
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_init(void)
{
    /* 初始化系统时钟 */
    FL_RCC_DeInit();
    
    /* 配置系统时钟为XTHF，使用16MHz晶振 */
    FL_RCC_XTHF_SetMode(FL_RCC_XTHF_MODE_OSC);
    FL_RCC_XTHF_Enable();
    while (FL_RCC_XTHF_IsReady() == 0) {
        /* 等待XTHF稳定 */
    }
    
    /* 配置PLL倍频，使用XTHF作为PLL输入，倍频至48MHz */
    FL_RCC_PLL_SetInputSource(FL_RCC_PLL_INPUT_SOURCE_XTHF);
    FL_RCC_PLL_SetPrescaler(FL_RCC_PLL_PSC_DIV16);  /* 16MHz / 16 = 1MHz */
    FL_RCC_PLL_SetMultiplier(FL_RCC_PLL_MUL_48);    /* 1MHz * 48 = 48MHz */
    FL_RCC_PLL_Enable();
    while (FL_RCC_PLL_IsEnabled() == 0) {
        /* 等待PLL使能 */
    }
    while (FL_RCC_PLL_IsReady() == 0) {
        /* 等待PLL稳定 */
    }
    
    /* 配置AHB分频为1，APB分频为1 */
    FL_RCC_SetAHBPrescaler(FL_RCC_AHBCLK_PSC_DIV1);
    FL_RCC_SetAPBPrescaler(FL_RCC_APBCLK_PSC_DIV1);
    
    /* 设置FLASH等待周期为2 */
    FL_FLASH_SetReadWait(FLASH, FL_FLASH_READ_WAIT_2CYCLE);
    
    /* 配置系统时钟源为PLL */
    FL_RCC_SetSystemClockSource(FL_RCC_SYSTEM_CLK_SOURCE_PLL);
    while (FL_RCC_GetSystemClockSource() != FL_RCC_SYSTEM_CLK_SOURCE_PLL) {
        /* 等待时钟源切换完成 */
    }
    
    /* 初始化SysTick */
    SysTick_Config(SystemCoreClock / 1000); /* 配置为1ms中断 */
    
    /* 初始化GPIO时钟 */
    FL_RCC_EnableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_GPIOA);
    FL_RCC_EnableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_GPIOB);
    FL_RCC_EnableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_GPIOC);
    FL_RCC_EnableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_GPIOD);
    
    /* 初始化其他必要的外设 */
    FL_RCC_EnableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_DMA);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_UART0);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_UART1);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_UART4);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_I2C);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_SPI0);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ATIM);
    FL_RCC_EnableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_BSTIM);
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化FM33LC0xx平台
 * 
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_deinit(void)
{
    /* 关闭所有外设时钟 */
    FL_RCC_DisableGroup1BusClock(FL_RCC_GROUP1_BUSCLK_ALL);
    FL_RCC_DisableGroup2BusClock(FL_RCC_GROUP2_BUSCLK_ALL);
    
    /* 关闭PLL */
    FL_RCC_PLL_Disable();
    
    /* 关闭XTHF */
    FL_RCC_XTHF_Disable();
    
    /* 复位RCC */
    FL_RCC_DeInit();
    
    return DRIVER_OK;
}

/**
 * @brief 获取FM33LC0xx平台信息
 * 
 * @param info 平台信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_info(platform_info_t *info)
{
    /* 参数检查 */
    if (info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 填充平台信息 */
    strcpy(info->name, PLATFORM_NAME);
    strcpy(info->description, PLATFORM_DESCRIPTION);
    strcpy(info->version, PLATFORM_VERSION);
    info->cpu_freq_hz = SystemCoreClock;
    
    return DRIVER_OK;
}

/**
 * @brief FM33LC0xx平台睡眠
 * 
 * @param ms 睡眠时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_sleep(uint32_t ms)
{
    uint32_t target_time = g_platform_uptime + ms;
    
    /* 等待时间到达 */
    while (g_platform_uptime < target_time) {
        /* 简单延时，实际应用中可以进入低功耗模式 */
        __NOP();
    }
    
    return DRIVER_OK;
}

/**
 * @brief FM33LC0xx平台复位
 * 
 * @param type 复位类型
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_reset(platform_reset_type_t type)
{
    switch (type) {
        case PLATFORM_RESET_MCU:
            /* 软件复位 */
            NVIC_SystemReset();
            break;
            
        case PLATFORM_RESET_PERIPHERAL:
            /* 复位所有外设 */
            FL_RCC_DeInit();
            fm33lc0xx_platform_init();
            break;
            
        default:
            return ERROR_INVALID_PARAM;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取FM33LC0xx平台运行时间
 * 
 * @param uptime 运行时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_uptime(uint32_t *uptime)
{
    /* 参数检查 */
    if (uptime == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取运行时间 */
    *uptime = g_platform_uptime;
    
    return DRIVER_OK;
}

/**
 * @brief 获取FM33LC0xx平台唯一ID
 * 
 * @param id ID缓冲区
 * @param len ID缓冲区长度
 * @return int 实际ID长度，负值表示错误
 */
int fm33lc0xx_platform_get_unique_id(uint8_t *id, uint32_t len)
{
    uint32_t uid[3];
    uint32_t copy_len;
    
    /* 参数检查 */
    if (id == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 读取UID */
    uid[0] = FL_UID->ID0;
    uid[1] = FL_UID->ID1;
    uid[2] = FL_UID->ID2;
    
    /* 复制UID到缓冲区 */
    copy_len = (len < sizeof(uid)) ? len : sizeof(uid);
    memcpy(id, uid, copy_len);
    
    return copy_len;
}

/**
 * @brief 获取FM33LC0xx平台随机数
 * 
 * @param data 随机数缓冲区
 * @param len 随机数长度
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_random(uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t random;
    
    /* 参数检查 */
    if (data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* FM33LC0xx没有硬件随机数生成器，使用ADC采样噪声作为随机源 */
    for (i = 0; i < len; i++) {
        /* 使用ADC采样不连接的通道获取噪声 */
        FL_ADC_Start(ADC);
        while (FL_ADC_IsActiveFlag_ConversionComplete(ADC) == 0) {
            /* 等待转换完成 */
        }
        random = FL_ADC_ReadConversionData(ADC);
        
        /* 使用ADC值的低8位作为随机数 */
        data[i] = (uint8_t)(random & 0xFF);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取FM33LC0xx平台闪存信息
 * 
 * @param info 闪存信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_flash_info(platform_flash_info_t *info)
{
    /* 参数检查 */
    if (info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 填充闪存信息 */
    info->start_address = 0x08000000;           /* Flash起始地址 */
    info->size = 64 * 1024;                     /* 64KB Flash */
    info->page_size = 1024;                     /* 1KB页大小 */
    info->sector_size = 1024;                   /* 1KB扇区大小 */
    info->erase_value = 0xFF;                   /* 擦除值为0xFF */
    info->program_unit = 4;                     /* 4字节编程单元 */
    
    return DRIVER_OK;
}

/**
 * @brief 获取FM33LC0xx平台RAM信息
 * 
 * @param info RAM信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int fm33lc0xx_platform_get_ram_info(platform_ram_info_t *info)
{
    /* 参数检查 */
    if (info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 填充RAM信息 */
    info->start_address = 0x20000000;           /* RAM起始地址 */
    info->size = 8 * 1024;                      /* 8KB RAM */
    
    return DRIVER_OK;
}

/**
 * @brief SysTick中断处理函数
 */
void SysTick_Handler(void)
{
    /* 更新运行时间 */
    g_platform_uptime++;
}

/**
 * @brief 毫秒级延时
 * 
 * @param ms 延时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int timer_delay_ms(uint32_t ms)
{
    uint32_t target_time = g_platform_uptime + ms;
    
    /* 等待时间到达 */
    while (g_platform_uptime < target_time) {
        /* 简单延时，实际应用中可以进入低功耗模式 */
        __NOP();
    }
    
    return DRIVER_OK;
}
