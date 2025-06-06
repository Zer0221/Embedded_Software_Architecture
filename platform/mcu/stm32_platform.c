/**
 * @file stm32_platform.c
 * @brief STM32平台相关的实现
 *
 * 该文件包含STM32平台相关的实现，实现了平台抽象层定义的接口
 */

#include "platform_api.h"
#include "stm32_platform.h"

/**
 * @brief STM32平台初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_init(void)
{
    /* 初始化系统时钟 */
    SystemClock_Config();
    
    /* 初始化HAL库 */
    HAL_Init();
    
    /* 初始化系统滴答定时器 */
    SysTick_Config(SystemCoreClock / 1000);
    
    return 0;
}

/**
 * @brief STM32平台去初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
int platform_deinit(void)
{
    /* 去初始化HAL库 */
    HAL_DeInit();
    
    return 0;
}

/**
 * @brief 获取STM32平台信息
 * 
 * @param info 平台信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int platform_get_info(void *info)
{
    stm32_platform_info_t *platform_info = (stm32_platform_info_t *)info;
    
    if (platform_info == NULL) {
        return -1;
    }
    
    /* 填充平台信息 */
    platform_info->cpu_type = STM32_CPU_TYPE;
    platform_info->cpu_clock = SystemCoreClock;
    platform_info->flash_size = STM32_FLASH_SIZE;
    platform_info->ram_size = STM32_RAM_SIZE;
    
    return 0;
}

/**
 * @brief STM32平台延时函数（毫秒）
 * 
 * @param ms 延时毫秒数
 */
void platform_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief STM32平台延时函数（微秒）
 * 
 * @param us 延时微秒数
 */
void platform_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000) * us;
    
    while ((DWT->CYCCNT - start) < cycles);
}

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t platform_get_time_ms(void)
{
    return HAL_GetTick();
}

/**
 * @brief 系统时钟配置
 * 
 * @note 这是一个STM32平台特定的函数，不是平台抽象层的一部分
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* 配置主振荡器和PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* 配置系统时钟、AHB、APB总线时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
    
    /* 配置Systick中断 */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}
