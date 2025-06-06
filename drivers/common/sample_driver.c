/**
 * @file sample_driver.c
 * @brief 示例驱动实现
 */

#include <stdio.h>
#include "common/driver_manager.h"
#include "common/driver_api.h"
#include "common/error_handling.h"

// 驱动接口函数声明
static int sample_driver_init(void);
static int sample_driver_deinit(void);

// 驱动特定功能函数
static int sample_function1(void *args);
static int sample_function2(int param);

// 驱动接口定义
static const driver_interface_t sample_driver_interface = {
    .sample_function1 = sample_function1,
    .sample_function2 = sample_function2
};

// 驱动功能实现
static int sample_function1(void *args) {
    printf("Sample driver function1 called with args: %p\n", args);
    return 0;
}

static int sample_function2(int param) {
    printf("Sample driver function2 called with param: %d\n", param);
    return param * 2;
}

// 驱动初始化函数
static int sample_driver_init(void) {
    printf("Sample driver initializing\n");
    return 0;
}

// 驱动去初始化函数
static int sample_driver_deinit(void) {
    printf("Sample driver deinitializing\n");
    return 0;
}

// 注册驱动
DRIVER_REGISTER(sample_driver,
                "Sample Driver",
                "1.0.0",
                DRIVER_TYPE_MISC,
                (void *)&sample_driver_interface,
                0,
                sample_driver_init,
                sample_driver_deinit,
                NULL);
