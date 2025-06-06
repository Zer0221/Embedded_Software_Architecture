/**
 * @file unit_test.h
 * @brief 单元测试框架接口定义
 *
 * 该头文件定义了简单的单元测试框架接口
 */

#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 定义颜色代码 - 用于终端输出 */
#define UT_COLOR_RED     "\033[0;31m"
#define UT_COLOR_GREEN   "\033[0;32m"
#define UT_COLOR_YELLOW  "\033[0;33m"
#define UT_COLOR_BLUE    "\033[0;34m"
#define UT_COLOR_RESET   "\033[0m"

/* 测试断言宏 */
#define UT_ASSERT(condition) \
    do { \
        ut_assert_impl(condition, #condition, __FILE__, __LINE__); \
    } while (0)

#define UT_ASSERT_EQUAL_INT(expected, actual) \
    do { \
        ut_assert_equal_int_impl(expected, actual, #expected, #actual, __FILE__, __LINE__); \
    } while (0)

#define UT_ASSERT_EQUAL_FLOAT(expected, actual, epsilon) \
    do { \
        ut_assert_equal_float_impl(expected, actual, epsilon, #expected, #actual, __FILE__, __LINE__); \
    } while (0)

#define UT_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        ut_assert_equal_string_impl(expected, actual, #expected, #actual, __FILE__, __LINE__); \
    } while (0)

#define UT_ASSERT_NULL(pointer) \
    do { \
        ut_assert_null_impl(pointer, #pointer, __FILE__, __LINE__); \
    } while (0)

#define UT_ASSERT_NOT_NULL(pointer) \
    do { \
        ut_assert_not_null_impl(pointer, #pointer, __FILE__, __LINE__); \
    } while (0)

/* 测试案例函数类型 */
typedef void (*ut_test_func_t)(void);

/* 测试案例结构体 */
typedef struct {
    const char *name;      /* 测试案例名称 */
    ut_test_func_t func;   /* 测试案例函数 */
} ut_test_case_t;

/* 测试套件结构体 */
typedef struct {
    const char *name;               /* 测试套件名称 */
    ut_test_case_t *cases;          /* 测试案例数组 */
    unsigned int case_count;        /* 测试案例数量 */
    void (*setup)(void);            /* 测试套件初始化函数 */
    void (*teardown)(void);         /* 测试套件清理函数 */
    void (*case_setup)(void);       /* 测试案例初始化函数 */
    void (*case_teardown)(void);    /* 测试案例清理函数 */
} ut_test_suite_t;

/* 测试统计信息 */
typedef struct {
    unsigned int total_suites;      /* 总测试套件数 */
    unsigned int total_cases;       /* 总测试案例数 */
    unsigned int passed_cases;      /* 通过测试案例数 */
    unsigned int failed_cases;      /* 失败测试案例数 */
    unsigned int total_assertions;  /* 总断言数 */
    unsigned int passed_assertions; /* 通过断言数 */
    unsigned int failed_assertions; /* 失败断言数 */
} ut_statistics_t;

/**
 * @brief 运行测试套件
 * 
 * @param suite 测试套件
 * @param stats 测试统计信息
 */
void ut_run_suite(ut_test_suite_t *suite, ut_statistics_t *stats);

/**
 * @brief 运行多个测试套件
 * 
 * @param suites 测试套件数组
 * @param suite_count 测试套件数量
 * @return int 0表示全部通过，非0表示存在失败
 */
int ut_run_suites(ut_test_suite_t *suites, unsigned int suite_count);

/**
 * @brief 打印测试统计信息
 * 
 * @param stats 测试统计信息
 */
void ut_print_statistics(ut_statistics_t *stats);

/**
 * @brief 初始化测试统计信息
 * 
 * @param stats 测试统计信息
 */
void ut_init_statistics(ut_statistics_t *stats);

/**
 * @brief 断言实现函数
 * 
 * @param condition 条件
 * @param condition_str 条件字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_impl(bool condition, const char *condition_str, const char *file, int line);

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
                             const char *actual_str, const char *file, int line);

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
                               const char *file, int line);

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
                                const char *file, int line);

/**
 * @brief 指针为NULL断言实现函数
 * 
 * @param pointer 指针
 * @param pointer_str 指针字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_null_impl(const void *pointer, const char *pointer_str, 
                        const char *file, int line);

/**
 * @brief 指针不为NULL断言实现函数
 * 
 * @param pointer 指针
 * @param pointer_str 指针字符串
 * @param file 文件名
 * @param line 行号
 */
void ut_assert_not_null_impl(const void *pointer, const char *pointer_str, 
                            const char *file, int line);

/* 当前测试状态 */
extern ut_statistics_t *ut_current_stats;
extern bool ut_current_test_failed;

#endif /* UNIT_TEST_H */
