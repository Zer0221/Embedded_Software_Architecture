/**
 * @file test_pwm.c
 * @brief PWM驱动单元测试
 *
 * 该文件实现了PWM驱动的单元测试
 */

#include "unit_test.h"
#include "pwm_api.h"
#include "error_api.h"
#include <string.h>

/* 模拟PWM设备句柄 */
static pwm_handle_t pwm_handle = NULL;

/* 模拟PWM回调函数计数器 */
static int callback_count[3] = {0, 0, 0};

/**
 * @brief 模拟PWM周期事件回调函数
 * 
 * @param user_data 用户数据
 */
static void mock_pwm_period_callback(void *user_data)
{
    callback_count[PWM_EVENT_PERIOD_ELAPSED]++;
}

/**
 * @brief 模拟PWM脉冲完成事件回调函数
 * 
 * @param user_data 用户数据
 */
static void mock_pwm_pulse_callback(void *user_data)
{
    callback_count[PWM_EVENT_PULSE_FINISHED]++;
}

/**
 * @brief 模拟PWM中断事件回调函数
 * 
 * @param user_data 用户数据
 */
static void mock_pwm_break_callback(void *user_data)
{
    callback_count[PWM_EVENT_BREAK]++;
}

/**
 * @brief 测试PWM初始化
 */
static void test_pwm_init(void)
{
    pwm_config_t config;
    int ret;
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 测试正常初始化 */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    UT_ASSERT_NOT_NULL(pwm_handle);
    
    /* 测试无效参数 */
    ret = pwm_init(NULL, &pwm_handle);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = pwm_init(&config, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 测试无效通道 */
    config.channel = PWM_CHANNEL_MAX;
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 恢复有效配置 */
    config.channel = PWM_CHANNEL_0;
}

/**
 * @brief 测试PWM去初始化
 */
static void test_pwm_deinit(void)
{
    int ret;
    
    /* 测试正常去初始化 */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_deinit(NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
}

/**
 * @brief 测试PWM启动和停止
 */
static void test_pwm_start_stop(void)
{
    pwm_config_t config;
    int ret;
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 初始化PWM */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试PWM启动 */
    ret = pwm_start(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_start(NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 测试PWM停止 */
    ret = pwm_stop(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_stop(NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化PWM */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试PWM频率设置
 */
static void test_pwm_set_frequency(void)
{
    pwm_config_t config;
    int ret;
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 初始化PWM */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试设置频率 */
    ret = pwm_set_frequency(pwm_handle, 2000);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_set_frequency(pwm_handle, 500);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_set_frequency(NULL, 1000);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化PWM */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试PWM占空比设置
 */
static void test_pwm_set_duty_cycle(void)
{
    pwm_config_t config;
    int ret;
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 初始化PWM */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试设置占空比 */
    ret = pwm_set_duty_cycle(pwm_handle, 0.25f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_set_duty_cycle(pwm_handle, 0.75f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试边界情况 */
    ret = pwm_set_duty_cycle(pwm_handle, 0.0f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_set_duty_cycle(pwm_handle, 1.0f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试超出范围的情况 */
    ret = pwm_set_duty_cycle(pwm_handle, -0.1f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_set_duty_cycle(pwm_handle, 1.1f);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_set_duty_cycle(NULL, 0.5f);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化PWM */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试PWM回调函数注册
 */
static void test_pwm_callback(void)
{
    pwm_config_t config;
    int ret;
    
    /* 初始化计数器 */
    memset(callback_count, 0, sizeof(callback_count));
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 初始化PWM */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试注册回调函数 */
    ret = pwm_register_callback(pwm_handle, PWM_EVENT_PERIOD_ELAPSED, 
                              mock_pwm_period_callback, NULL);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_register_callback(pwm_handle, PWM_EVENT_PULSE_FINISHED, 
                              mock_pwm_pulse_callback, NULL);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_register_callback(pwm_handle, PWM_EVENT_BREAK, 
                              mock_pwm_break_callback, NULL);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试取消注册回调函数 */
    ret = pwm_unregister_callback(pwm_handle, PWM_EVENT_PERIOD_ELAPSED);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_unregister_callback(pwm_handle, PWM_EVENT_PULSE_FINISHED);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    ret = pwm_unregister_callback(pwm_handle, PWM_EVENT_BREAK);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_register_callback(NULL, PWM_EVENT_PERIOD_ELAPSED, 
                              mock_pwm_period_callback, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = pwm_register_callback(pwm_handle, PWM_EVENT_PERIOD_ELAPSED, 
                              NULL, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = pwm_unregister_callback(NULL, PWM_EVENT_PERIOD_ELAPSED);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化PWM */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试PWM脉冲生成
 */
static void test_pwm_generate_pulse(void)
{
    pwm_config_t config;
    int ret;
    
    /* 配置PWM */
    memset(&config, 0, sizeof(config));
    config.channel = PWM_CHANNEL_0;
    config.frequency = 1000;
    config.duty_cycle = 0.5f;
    config.align_mode = PWM_ALIGN_EDGE;
    config.polarity = PWM_POLARITY_NORMAL;
    config.counter_mode = PWM_COUNTER_UP;
    
    /* 初始化PWM */
    ret = pwm_init(&config, &pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试生成脉冲 */
    ret = pwm_generate_pulse(pwm_handle, 10);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = pwm_generate_pulse(NULL, 10);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化PWM */
    ret = pwm_deinit(pwm_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/* PWM测试套件初始化 */
static void pwm_test_setup(void)
{
    printf("PWM测试套件初始化\n");
}

/* PWM测试套件清理 */
static void pwm_test_teardown(void)
{
    printf("PWM测试套件清理\n");
}

/* PWM测试案例初始化 */
static void pwm_test_case_setup(void)
{
    /* 无需特殊操作 */
}

/* PWM测试案例清理 */
static void pwm_test_case_teardown(void)
{
    /* 确保每个测试后都去初始化 */
    if (pwm_handle != NULL) {
        pwm_stop(pwm_handle);
        pwm_deinit(pwm_handle);
        pwm_handle = NULL;
    }
}

/* PWM测试案例 */
static ut_test_case_t pwm_test_cases[] = {
    {"测试PWM初始化", test_pwm_init},
    {"测试PWM去初始化", test_pwm_deinit},
    {"测试PWM启动和停止", test_pwm_start_stop},
    {"测试PWM频率设置", test_pwm_set_frequency},
    {"测试PWM占空比设置", test_pwm_set_duty_cycle},
    {"测试PWM回调函数注册", test_pwm_callback},
    {"测试PWM脉冲生成", test_pwm_generate_pulse}
};

/* PWM测试套件 */
ut_test_suite_t pwm_test_suite = {
    "PWM驱动测试套件",
    pwm_test_cases,
    sizeof(pwm_test_cases) / sizeof(pwm_test_cases[0]),
    pwm_test_setup,
    pwm_test_teardown,
    pwm_test_case_setup,
    pwm_test_case_teardown
};
