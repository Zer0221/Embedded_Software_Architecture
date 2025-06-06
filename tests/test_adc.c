/**
 * @file test_adc.c
 * @brief ADC驱动单元测试
 *
 * 该文件实现了ADC驱动的单元测试
 */

#include "unit_test.h"
#include "adc_api.h"
#include "error_api.h"
#include <string.h>

/* 模拟ADC设备句柄 */
static adc_handle_t adc_handle = NULL;

/* 模拟ADC回调函数计数器 */
static int callback_count = 0;

/* 模拟ADC值 */
static uint32_t mock_adc_value = 0;

/**
 * @brief 模拟ADC回调函数
 * 
 * @param value ADC值
 * @param user_data 用户数据
 */
static void mock_adc_callback(uint32_t value, void *user_data)
{
    callback_count++;
    mock_adc_value = value;
}

/**
 * @brief 测试ADC初始化
 */
static void test_adc_init(void)
{
    adc_config_t config;
    int ret;
    
    /* 配置ADC */
    memset(&config, 0, sizeof(config));
    config.channel = ADC_CHANNEL_0;
    config.resolution = ADC_RESOLUTION_12BIT;
    config.reference = ADC_REFERENCE_VDDA;
    config.sample_rate = ADC_SAMPLE_RATE_MEDIUM;
    config.reference_voltage = 3.3f;
    
    /* 测试正常初始化 */
    ret = adc_init(&config, &adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    UT_ASSERT_NOT_NULL(adc_handle);
    
    /* 测试无效参数 */
    ret = adc_init(NULL, &adc_handle);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = adc_init(&config, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 测试无效通道 */
    config.channel = ADC_CHANNEL_MAX;
    ret = adc_init(&config, &adc_handle);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 恢复有效配置 */
    config.channel = ADC_CHANNEL_0;
}

/**
 * @brief 测试ADC去初始化
 */
static void test_adc_deinit(void)
{
    int ret;
    
    /* 测试正常去初始化 */
    ret = adc_deinit(adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = adc_deinit(NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
}

/**
 * @brief 测试ADC读取
 */
static void test_adc_read(void)
{
    uint32_t value;
    int ret;
    adc_config_t config;
    
    /* 配置ADC */
    memset(&config, 0, sizeof(config));
    config.channel = ADC_CHANNEL_0;
    config.resolution = ADC_RESOLUTION_12BIT;
    config.reference = ADC_REFERENCE_VDDA;
    config.sample_rate = ADC_SAMPLE_RATE_MEDIUM;
    config.reference_voltage = 3.3f;
    
    /* 初始化ADC */
    ret = adc_init(&config, &adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试正常读取 */
    ret = adc_read(adc_handle, &value);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = adc_read(NULL, &value);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = adc_read(adc_handle, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化ADC */
    ret = adc_deinit(adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试ADC连续转换
 */
static void test_adc_continuous(void)
{
    int ret;
    adc_config_t config;
    
    /* 初始化计数器 */
    callback_count = 0;
    
    /* 配置ADC */
    memset(&config, 0, sizeof(config));
    config.channel = ADC_CHANNEL_0;
    config.resolution = ADC_RESOLUTION_12BIT;
    config.reference = ADC_REFERENCE_VDDA;
    config.sample_rate = ADC_SAMPLE_RATE_MEDIUM;
    config.reference_voltage = 3.3f;
    
    /* 初始化ADC */
    ret = adc_init(&config, &adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试启动连续转换 */
    ret = adc_start_continuous(adc_handle, mock_adc_callback, NULL);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = adc_start_continuous(NULL, mock_adc_callback, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = adc_start_continuous(adc_handle, NULL, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 测试停止连续转换 */
    ret = adc_stop_continuous(adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试无效参数 */
    ret = adc_stop_continuous(NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化ADC */
    ret = adc_deinit(adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试ADC电压转换
 */
static void test_adc_voltage_conversion(void)
{
    int ret;
    uint32_t raw_value = 2048; /* 12位ADC中间值 */
    float voltage = 0.0f;
    adc_config_t config;
    
    /* 配置ADC */
    memset(&config, 0, sizeof(config));
    config.channel = ADC_CHANNEL_0;
    config.resolution = ADC_RESOLUTION_12BIT;
    config.reference = ADC_REFERENCE_VDDA;
    config.sample_rate = ADC_SAMPLE_RATE_MEDIUM;
    config.reference_voltage = 3.3f;
    
    /* 初始化ADC */
    ret = adc_init(&config, &adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 测试电压转换 */
    ret = adc_convert_to_voltage(adc_handle, raw_value, &voltage);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
    
    /* 对于12位ADC，中间值应该接近参考电压的一半 */
    UT_ASSERT_EQUAL_FLOAT(config.reference_voltage / 2.0f, voltage, 0.2f);
    
    /* 测试无效参数 */
    ret = adc_convert_to_voltage(NULL, raw_value, &voltage);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    ret = adc_convert_to_voltage(adc_handle, raw_value, NULL);
    UT_ASSERT_EQUAL_INT(ERROR_INVALID_PARAM, ret);
    
    /* 去初始化ADC */
    ret = adc_deinit(adc_handle);
    UT_ASSERT_EQUAL_INT(DRIVER_OK, ret);
}

/**
 * @brief 测试ADC最大值
 */
static void test_adc_max_value(void)
{
    uint32_t max_value;
    
    /* 测试不同分辨率的最大值 */
    max_value = adc_get_max_value(ADC_RESOLUTION_6BIT);
    UT_ASSERT_EQUAL_INT(63, max_value); /* 2^6 - 1 */
    
    max_value = adc_get_max_value(ADC_RESOLUTION_8BIT);
    UT_ASSERT_EQUAL_INT(255, max_value); /* 2^8 - 1 */
    
    max_value = adc_get_max_value(ADC_RESOLUTION_10BIT);
    UT_ASSERT_EQUAL_INT(1023, max_value); /* 2^10 - 1 */
    
    max_value = adc_get_max_value(ADC_RESOLUTION_12BIT);
    UT_ASSERT_EQUAL_INT(4095, max_value); /* 2^12 - 1 */
}

/* ADC测试套件初始化 */
static void adc_test_setup(void)
{
    printf("ADC测试套件初始化\n");
}

/* ADC测试套件清理 */
static void adc_test_teardown(void)
{
    printf("ADC测试套件清理\n");
}

/* ADC测试案例初始化 */
static void adc_test_case_setup(void)
{
    /* 无需特殊操作 */
}

/* ADC测试案例清理 */
static void adc_test_case_teardown(void)
{
    /* 确保每个测试后都去初始化 */
    if (adc_handle != NULL) {
        adc_deinit(adc_handle);
        adc_handle = NULL;
    }
}

/* ADC测试案例 */
static ut_test_case_t adc_test_cases[] = {
    {"测试ADC初始化", test_adc_init},
    {"测试ADC去初始化", test_adc_deinit},
    {"测试ADC读取", test_adc_read},
    {"测试ADC连续转换", test_adc_continuous},
    {"测试ADC电压转换", test_adc_voltage_conversion},
    {"测试ADC最大值", test_adc_max_value}
};

/* ADC测试套件 */
ut_test_suite_t adc_test_suite = {
    "ADC驱动测试套件",
    adc_test_cases,
    sizeof(adc_test_cases) / sizeof(adc_test_cases[0]),
    adc_test_setup,
    adc_test_teardown,
    adc_test_case_setup,
    adc_test_case_teardown
};
