/**
 * @file test_power.c
 * @brief 电源管理单元测试
 *
 * 该文件实现了电源管理模块的单元测试
 */

#include "common/unit_test.h"
#include "base/power_api.h"
#include <stdio.h>
#include <string.h>

/* 测试用例计数 */
static int g_test_count = 0;
static int g_pass_count = 0;

/* 电源设备句柄 */
static power_handle_t g_power_handle;

/**
 * @brief 电源回调函数
 */
static void power_callback(power_mode_t mode, wakeup_source_t source, void *user_data)
{
    printf("Power callback: mode=%d, source=0x%x\n", mode, source);
}

/**
 * @brief 测试电源初始化
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_power_init(void)
{
    power_config_t config;
    int ret;
    
    g_test_count++;
    
    /* 初始化配置 */
    memset(&config, 0, sizeof(config));
    config.enable_auto_sleep = true;
    config.auto_sleep_timeout_ms = 5000;
    config.wakeup_sources = WAKEUP_SOURCE_PIN | WAKEUP_SOURCE_RTC_ALARM;
    config.battery_type = BATTERY_TYPE_LIPO;
    config.battery_low_threshold = 3.3f;
    config.battery_critical_threshold = 3.1f;
    config.enable_battery_monitor = true;
    config.battery_monitor_interval_ms = 5000;
    config.enable_power_saving = true;
    config.power_saving_level = 50;
    config.enable_thermal_protection = true;
    config.thermal_shutdown_temp = 60.0f;
    
    /* 初始化电源管理 */
    ret = power_init(&config, &g_power_handle);
    
    if (ret == 0) {
        printf("Test power_init: PASS\n");
        g_pass_count++;
        return 0;
    } else {
        printf("Test power_init: FAIL (ret=%d)\n", ret);
        return -1;
    }
}

/**
 * @brief 测试电源模式设置和获取
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_power_mode(void)
{
    power_mode_t mode;
    int ret;
    
    g_test_count++;
    
    /* 获取当前模式 */
    ret = power_get_mode(g_power_handle, &mode);
    
    if (ret != 0) {
        printf("Test power_mode: FAIL (power_get_mode ret=%d)\n", ret);
        return -1;
    }
    
    /* 设置睡眠模式，1秒后自动唤醒 */
    printf("Setting POWER_MODE_SLEEP for 1000ms...\n");
    ret = power_set_mode(g_power_handle, POWER_MODE_SLEEP, 1000);
    
    if (ret != 0) {
        printf("Test power_mode: FAIL (power_set_mode ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取当前模式，应该已经恢复为活动模式 */
    ret = power_get_mode(g_power_handle, &mode);
    
    if (ret != 0 || mode != POWER_MODE_ACTIVE) {
        printf("Test power_mode: FAIL (power_get_mode ret=%d, mode=%d)\n", ret, mode);
        return -1;
    }
    
    printf("Test power_mode: PASS\n");
    g_pass_count++;
    return 0;
}

/**
 * @brief 测试唤醒源配置
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_wakeup_source(void)
{
    uint32_t sources;
    int ret;
    
    g_test_count++;
    
    /* 配置唤醒源 */
    ret = power_config_wakeup_source(g_power_handle, WAKEUP_SOURCE_PIN, true);
    
    if (ret != 0) {
        printf("Test wakeup_source: FAIL (power_config_wakeup_source ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取唤醒源 */
    ret = power_get_wakeup_source(g_power_handle, &sources);
    
    if (ret != 0) {
        printf("Test wakeup_source: FAIL (power_get_wakeup_source ret=%d)\n", ret);
        return -1;
    }
    
    /* 重置唤醒源 */
    ret = power_reset_wakeup_source(g_power_handle, WAKEUP_SOURCE_PIN);
    
    if (ret != 0) {
        printf("Test wakeup_source: FAIL (power_reset_wakeup_source ret=%d)\n", ret);
        return -1;
    }
    
    printf("Test wakeup_source: PASS (sources=0x%x)\n", sources);
    g_pass_count++;
    return 0;
}

/**
 * @brief 测试电源回调函数
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_power_callback(void)
{
    int ret;
    
    g_test_count++;
    
    /* 注册回调函数 */
    ret = power_register_callback(g_power_handle, power_callback, NULL);
    
    if (ret != 0) {
        printf("Test power_callback: FAIL (power_register_callback ret=%d)\n", ret);
        return -1;
    }
    
    /* 设置睡眠模式，触发回调 */
    printf("Setting POWER_MODE_SLEEP for 500ms to trigger callback...\n");
    ret = power_set_mode(g_power_handle, POWER_MODE_SLEEP, 500);
    
    if (ret != 0) {
        printf("Test power_callback: FAIL (power_set_mode ret=%d)\n", ret);
        return -1;
    }
    
    /* 取消注册回调函数 */
    ret = power_unregister_callback(g_power_handle, power_callback);
    
    if (ret != 0) {
        printf("Test power_callback: FAIL (power_unregister_callback ret=%d)\n", ret);
        return -1;
    }
    
    printf("Test power_callback: PASS\n");
    g_pass_count++;
    return 0;
}

/**
 * @brief 测试电池信息获取
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_battery_info(void)
{
    float voltage;
    uint8_t percentage;
    battery_status_t status;
    battery_health_t health;
    charge_status_t charge_status;
    power_state_t power_state;
    float temperature;
    int ret;
    
    g_test_count++;
    
    /* 获取电池电压 */
    ret = power_get_battery_voltage(g_power_handle, &voltage);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_battery_voltage ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取电池电量 */
    ret = power_get_battery_percentage(g_power_handle, &percentage);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_battery_percentage ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取电池状态 */
    ret = power_get_battery_status(g_power_handle, &status);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_battery_status ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取电池健康状态 */
    ret = power_get_battery_health(g_power_handle, &health);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_battery_health ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取充电状态 */
    ret = power_get_charge_status(g_power_handle, &charge_status);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_charge_status ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取电源状态 */
    ret = power_get_state(g_power_handle, &power_state);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_state ret=%d)\n", ret);
        return -1;
    }
    
    /* 获取温度 */
    ret = power_get_temperature(g_power_handle, &temperature);
    
    if (ret != 0) {
        printf("Test battery_info: FAIL (power_get_temperature ret=%d)\n", ret);
        return -1;
    }
    
    printf("Battery info: voltage=%.2fV, percentage=%d%%, status=%d, health=%d, charge_status=%d, power_state=%d, temperature=%.1f°C\n",
           voltage, percentage, status, health, charge_status, power_state, temperature);
    
    printf("Test battery_info: PASS\n");
    g_pass_count++;
    return 0;
}

/**
 * @brief 测试自动睡眠功能
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_auto_sleep(void)
{
    int ret;
    
    g_test_count++;
    
    /* 禁用自动睡眠 */
    ret = power_set_auto_sleep(g_power_handle, false, 0);
    
    if (ret != 0) {
        printf("Test auto_sleep: FAIL (power_set_auto_sleep disable ret=%d)\n", ret);
        return -1;
    }
    
    /* 启用自动睡眠，10秒超时 */
    ret = power_set_auto_sleep(g_power_handle, true, 10000);
    
    if (ret != 0) {
        printf("Test auto_sleep: FAIL (power_set_auto_sleep enable ret=%d)\n", ret);
        return -1;
    }
    
    /* 重置自动睡眠计时器 */
    ret = power_reset_auto_sleep_timer(g_power_handle);
    
    if (ret != 0) {
        printf("Test auto_sleep: FAIL (power_reset_auto_sleep_timer ret=%d)\n", ret);
        return -1;
    }
    
    /* 再次禁用自动睡眠 */
    ret = power_set_auto_sleep(g_power_handle, false, 0);
    
    if (ret != 0) {
        printf("Test auto_sleep: FAIL (power_set_auto_sleep disable again ret=%d)\n", ret);
        return -1;
    }
    
    printf("Test auto_sleep: PASS\n");
    g_pass_count++;
    return 0;
}

/**
 * @brief 测试电池监控功能
 * 
 * @return int 0表示成功，非0表示失败
 */
static int test_battery_monitor(void)
{
    int ret;
    
    g_test_count++;
    
    /* 禁用电池监控 */
    ret = power_set_battery_monitor(g_power_handle, false, 0);
    
    if (ret != 0) {
        printf("Test battery_monitor: FAIL (power_set_battery_monitor disable ret=%d)\n", ret);
        return -1;
    }
    
    /* 启用电池监控，5秒间隔 */
    ret = power_set_battery_monitor(g_power_handle, true, 5000);
    
    if (ret != 0) {
        printf("Test battery_monitor: FAIL (power_set_battery_monitor enable ret=%d)\n", ret);
        return -1;
    }
    
    /* 再次禁用电池监控 */
    ret = power_set_battery_monitor(g_power_handle, false, 0);
    
    if (ret != 0) {
        printf("Test battery_monitor: FAIL (power_set_battery_monitor disable again ret=%d)\n", ret);
        return -1;
    }
    
    printf("Test battery_monitor: PASS\n");
    g_pass_count++;
    return 0;
}

/**
 * @brief 运行所有电源管理测试
 * 
 * @return int 0表示成功，非0表示失败
 */
int test_power(void)
{
    int result = 0;
    
    printf("\n=== Running Power Management Tests ===\n");
    
    /* 初始化测试 */
    if (test_power_init() != 0) {
        return -1;
    }
    
    /* 模式测试 */
    result |= test_power_mode();
    
    /* 唤醒源测试 */
    result |= test_wakeup_source();
    
    /* 回调函数测试 */
    result |= test_power_callback();
    
    /* 电池信息测试 */
    result |= test_battery_info();
    
    /* 自动睡眠测试 */
    result |= test_auto_sleep();
    
    /* 电池监控测试 */
    result |= test_battery_monitor();
    
    /* 去初始化电源管理 */
    power_deinit(g_power_handle);
    
    /* 打印测试结果 */
    printf("\n=== Power Management Test Results ===\n");
    printf("Total: %d, Passed: %d, Failed: %d\n\n", 
           g_test_count, g_pass_count, g_test_count - g_pass_count);
    
    return result;
}
