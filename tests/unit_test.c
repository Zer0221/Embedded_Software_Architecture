/**
 * @file unit_test.c
 * @brief 单元测试框架实现
 *
 * 该文件实现了简单的单元测试框架功能
 */

#include "unit_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* 全局测试状态 */
ut_statistics_t *ut_current_stats = NULL;
bool ut_current_test_failed = false;

/**
 * @brief 初始化测试统计信息
 * 
 * @param stats 测试统计信息
 */
void ut_init_statistics(ut_statistics_t *stats)
{
    if (stats != NULL) {
        memset(stats, 0, sizeof(ut_statistics_t));
    }
}

/**
 * @brief 断言实现函数
 * 
 * @param condition 条件
 * @param condition_str 条件字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_impl(bool condition, const char *condition_str, const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (condition) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               condition_str, file, line);
    }
}

/**
 * @brief 整数相等断言实现函数
 * 
 * @param expected 期望值
 * @param actual 实际值
 * @param expected_str 期望值字符串
 * @param actual_str 实际值字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_equal_int_impl(int expected, int actual, const char *expected_str, 
                             const char *actual_str, const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (expected == actual) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s == %s, 期望: %d, 实际: %d, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               expected_str, actual_str, expected, actual, file, line);
    }
}

/**
 * @brief 浮点数相等断言实现函数
 * 
 * @param expected 期望值
 * @param actual 实际值
 * @param epsilon 误差范围
 * @param expected_str 期望值字符串
 * @param actual_str 实际值字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_equal_float_impl(float expected, float actual, float epsilon, 
                               const char *expected_str, const char *actual_str, 
                               const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (fabsf(expected - actual) <= epsilon) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s == %s, 期望: %f, 实际: %f, 误差: %f, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               expected_str, actual_str, expected, actual, epsilon, file, line);
    }
}

/**
 * @brief 字符串相等断言实现函数
 * 
 * @param expected 期望值
 * @param actual 实际值
 * @param expected_str 期望值字符串
 * @param actual_str 实际值字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_equal_string_impl(const char *expected, const char *actual, 
                                const char *expected_str, const char *actual_str, 
                                const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (expected == NULL && actual == NULL) {
        ut_current_stats->passed_assertions++;
        return;
    }
    
    if (expected == NULL || actual == NULL) {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s == %s, 期望: %s, 实际: %s, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               expected_str, actual_str, 
               expected ? expected : "NULL", 
               actual ? actual : "NULL", 
               file, line);
        return;
    }
    
    if (strcmp(expected, actual) == 0) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s == %s, 期望: %s, 实际: %s, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               expected_str, actual_str, expected, actual, file, line);
    }
}

/**
 * @brief 指针为NULL断言实现函数
 * 
 * @param pointer 指针
 * @param pointer_str 指针字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_null_impl(const void *pointer, const char *pointer_str, 
                        const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (pointer == NULL) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s == NULL, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               pointer_str, file, line);
    }
}

/**
 * @brief 指针不为NULL断言实现函数
 * 
 * @param pointer 指针
 * @param pointer_str 指针字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_not_null_impl(const void *pointer, const char *pointer_str, 
                            const char *file, int line)
{
    ut_current_stats->total_assertions++;
    
    if (pointer != NULL) {
        ut_current_stats->passed_assertions++;
    } else {
        ut_current_stats->failed_assertions++;
        ut_current_test_failed = true;
        printf(UT_COLOR_RED "断言失败: %s != NULL, 文件: %s, 行: %d\n" UT_COLOR_RESET, 
               pointer_str, file, line);
    }
}

/**
 * @brief 运行测试套件
 * 
 * @param suite 测试套件
 * @param stats 测试统计信息
 */
void ut_run_suite(ut_test_suite_t *suite, ut_statistics_t *stats)
{
    unsigned int i;
    
    if (suite == NULL || stats == NULL) {
        return;
    }
    
    printf(UT_COLOR_BLUE "\n运行测试套件: %s\n" UT_COLOR_RESET, suite->name);
    
    /* 更新统计信息 */
    stats->total_suites++;
    stats->total_cases += suite->case_count;
    
    /* 设置当前统计信息指针 */
    ut_current_stats = stats;
    
    /* 调用套件初始化函数 */
    if (suite->setup != NULL) {
        suite->setup();
    }
    
    /* 运行测试案例 */
    for (i = 0; i < suite->case_count; i++) {
        ut_test_case_t *test_case = &suite->cases[i];
        
        printf(UT_COLOR_YELLOW "  测试案例: %s\n" UT_COLOR_RESET, test_case->name);
        
        /* 重置当前测试失败标志 */
        ut_current_test_failed = false;
        
        /* 调用测试案例初始化函数 */
        if (suite->case_setup != NULL) {
            suite->case_setup();
        }
        
        /* 执行测试案例 */
        test_case->func();
        
        /* 调用测试案例清理函数 */
        if (suite->case_teardown != NULL) {
            suite->case_teardown();
        }
        
        /* 更新统计信息 */
        if (ut_current_test_failed) {
            stats->failed_cases++;
            printf(UT_COLOR_RED "  测试案例失败: %s\n" UT_COLOR_RESET, test_case->name);
        } else {
            stats->passed_cases++;
            printf(UT_COLOR_GREEN "  测试案例通过: %s\n" UT_COLOR_RESET, test_case->name);
        }
    }
    
    /* 调用套件清理函数 */
    if (suite->teardown != NULL) {
        suite->teardown();
    }
}

/**
 * @brief 运行多个测试套件
 * 
 * @param suites 测试套件数组
 * @param suite_count 测试套件数量
 * @return int 0表示全部通过，非0表示存在失败
 */
int ut_run_suites(ut_test_suite_t *suites, unsigned int suite_count)
{
    unsigned int i;
    ut_statistics_t stats;
    
    /* 初始化统计信息 */
    ut_init_statistics(&stats);
    
    printf(UT_COLOR_BLUE "\n===== 开始运行单元测试 =====\n" UT_COLOR_RESET);
    
    /* 运行测试套件 */
    for (i = 0; i < suite_count; i++) {
        ut_run_suite(&suites[i], &stats);
    }
    
    /* 打印统计信息 */
    ut_print_statistics(&stats);
    
    /* 返回测试结果 */
    return (stats.failed_cases > 0) ? 1 : 0;
}

/**
 * @brief 打印测试统计信息
 * 
 * @param stats 测试统计信息
 */
void ut_print_statistics(ut_statistics_t *stats)
{
    if (stats == NULL) {
        return;
    }
    
    printf(UT_COLOR_BLUE "\n===== 单元测试统计信息 =====\n" UT_COLOR_RESET);
    printf("测试套件总数: %u\n", stats->total_suites);
    printf("测试案例总数: %u\n", stats->total_cases);
    printf("通过案例数: " UT_COLOR_GREEN "%u" UT_COLOR_RESET "\n", stats->passed_cases);
    printf("失败案例数: " UT_COLOR_RED "%u" UT_COLOR_RESET "\n", stats->failed_cases);
    printf("断言总数: %u\n", stats->total_assertions);
    printf("通过断言数: " UT_COLOR_GREEN "%u" UT_COLOR_RESET "\n", stats->passed_assertions);
    printf("失败断言数: " UT_COLOR_RED "%u" UT_COLOR_RESET "\n", stats->failed_assertions);
    
    printf(UT_COLOR_BLUE "\n===== 单元测试结束 =====\n" UT_COLOR_RESET);
    
    if (stats->failed_cases == 0) {
        printf(UT_COLOR_GREEN "\n全部测试通过！\n" UT_COLOR_RESET);
    } else {
        printf(UT_COLOR_RED "\n存在测试失败！\n" UT_COLOR_RESET);
    }
}
