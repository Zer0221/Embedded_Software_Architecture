/**
 * @file sample_module.c
 * @brief 示例模块实现
 */

#include <stdio.h>
#include "common/module_support.h"
#include "common/error_handling.h"
#include "common/project_config.h"

// 模块依赖定义
static module_dependency_t sample_dependencies[] = {
    {
        .name = "memory_manager",
        .optional = false
    },
    {
        .name = "device_tree",
        .optional = true
    }
};

// 模块接口实现
static int sample_module_init(void) {
    printf("Sample module initializing\n");
    return 0;
}

static int sample_module_deinit(void) {
    printf("Sample module deinitializing\n");
    return 0;
}

static int sample_module_start(void) {
    printf("Sample module starting\n");
    return 0;
}

static int sample_module_stop(void) {
    printf("Sample module stopping\n");
    return 0;
}

static int sample_module_suspend(void) {
    printf("Sample module suspending\n");
    return 0;
}

static int sample_module_resume(void) {
    printf("Sample module resuming\n");
    return 0;
}

// 注册模块
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    MODULE_REGISTER(sample_module,
                    "Sample Module",
                    "1.0.0",
                    MODULE_PRIORITY_NORMAL,
                    sample_dependencies,
                    sizeof(sample_dependencies) / sizeof(sample_dependencies[0]),
                    sample_module_init,
                    sample_module_deinit,
                    sample_module_start,
                    sample_module_stop,
                    sample_module_suspend,
                    sample_module_resume,
                    NULL);
#else
    // 手动注册模块
    static module_info_t sample_module_info = {
        .name = "sample_module",
        .description = "Sample Module",
        .version = "1.0.0",
        .priority = MODULE_PRIORITY_NORMAL,
        .dependencies = sample_dependencies,
        .dependency_count = sizeof(sample_dependencies) / sizeof(sample_dependencies[0]),
        .interface = {
            .init = sample_module_init,
            .deinit = sample_module_deinit,
            .start = sample_module_start,
            .stop = sample_module_stop,
            .suspend = sample_module_suspend,
            .resume = sample_module_resume
        },
        .status = MODULE_STATUS_UNINITIALIZED,
        .private_data = NULL
    };
    
    // 在应用程序启动时需要手动调用：module_register(&sample_module_info);
#endif
