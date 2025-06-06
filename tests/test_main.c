/**
 * @file test_main.c
 * @brief 单元测试主程序
 *
 * 该文件是单元测试的入口点，用于运行所有测试套件
 */

#include "unit_test.h"
#include <stdio.h>

/* 导入测试套件 */
extern ut_test_suite_t adc_test_suite;
extern ut_test_suite_t pwm_test_suite;
extern int test_power(void);

/* 所有测试套件 */
static ut_test_suite_t *all_test_suites[] = {
    &adc_test_suite,
    &pwm_test_suite
};

/**
 * @brief 单元测试主函数
 * 
 * @return int 0表示全部通过，非0表示存在失败
 */
int main(void)
{
    int ret;
    
    /* 打印测试标题 */
    printf("\n");
    printf("==================================================\n");
    printf("          嵌入式设备软件架构单元测试              \n");
    printf("==================================================\n");
      /* 运行所有测试套件 */
    ret = ut_run_suites(all_test_suites, sizeof(all_test_suites) / sizeof(all_test_suites[0]));
    
    /* 运行电源管理测试 */
    test_power();
    
    return ret;
}
