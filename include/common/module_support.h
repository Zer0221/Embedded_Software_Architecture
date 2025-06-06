/**
 * @file module_support.h
 * @brief 模块化构建支持接口定义
 *
 * 该头文件定义了模块化支持相关的接口，提供模块的注册、初始化和管理功能
 */

#ifndef MODULE_SUPPORT_H
#define MODULE_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 模块优先级 */
typedef enum {
    MODULE_PRIORITY_HIGHEST = 0,   /**< 最高优先级 */
    MODULE_PRIORITY_HIGH,          /**< 高优先级 */
    MODULE_PRIORITY_NORMAL,        /**< 普通优先级 */
    MODULE_PRIORITY_LOW,           /**< 低优先级 */
    MODULE_PRIORITY_LOWEST         /**< 最低优先级 */
} module_priority_t;

/* 模块状态 */
typedef enum {
    MODULE_STATUS_UNINITIALIZED,   /**< 未初始化 */
    MODULE_STATUS_INITIALIZED,     /**< 已初始化 */
    MODULE_STATUS_RUNNING,         /**< 运行中 */
    MODULE_STATUS_SUSPENDED,       /**< 已挂起 */
    MODULE_STATUS_ERROR            /**< 错误状态 */
} module_status_t;

/* 模块依赖项 */
typedef struct {
    const char *name;              /**< 依赖模块名称 */
    bool optional;                 /**< 是否为可选依赖 */
} module_dependency_t;

/* 模块接口 */
typedef struct {
    int (*init)(void);             /**< 初始化函数 */
    int (*deinit)(void);           /**< 去初始化函数 */
    int (*start)(void);            /**< 启动函数 */
    int (*stop)(void);             /**< 停止函数 */
    int (*suspend)(void);          /**< 挂起函数 */
    int (*resume)(void);           /**< 恢复函数 */
} module_interface_t;

/* 模块信息结构体 */
typedef struct {
    const char *name;                    /**< 模块名称 */
    const char *description;             /**< 模块描述 */
    const char *version;                 /**< 模块版本 */
    module_priority_t priority;          /**< 模块优先级 */
    module_dependency_t *dependencies;   /**< 依赖项数组 */
    uint8_t dependency_count;            /**< 依赖项数量 */
    module_interface_t interface;        /**< 模块接口 */
    module_status_t status;              /**< 模块状态 */
    void *private_data;                  /**< 私有数据 */
} module_info_t;

/**
 * @brief 初始化模块系统
 * 
 * @return int 0表示成功，非0表示失败
 */
int module_system_init(void);

/**
 * @brief 注册模块
 * 
 * @param module 模块信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int module_register(module_info_t *module);

/**
 * @brief 注销模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_unregister(const char *name);

/**
 * @brief 查找模块
 * 
 * @param name 模块名称
 * @return module_info_t* 找到的模块信息，未找到返回NULL
 */
module_info_t *module_find(const char *name);

/**
 * @brief 初始化所有模块
 * 
 * 按照优先级和依赖关系初始化所有已注册模块
 * 
 * @return int 0表示成功，非0表示失败
 */
int module_init_all(void);

/**
 * @brief 启动所有模块
 * 
 * 按照优先级和依赖关系启动所有已初始化的模块
 * 
 * @return int 0表示成功，非0表示失败
 */
int module_start_all(void);

/**
 * @brief 停止所有模块
 * 
 * 按照依赖关系的反序停止所有运行中的模块
 * 
 * @return int 0表示成功，非0表示失败
 */
int module_stop_all(void);

/**
 * @brief 初始化指定模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_init(const char *name);

/**
 * @brief 启动指定模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_start(const char *name);

/**
 * @brief 停止指定模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_stop(const char *name);

/**
 * @brief 挂起指定模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_suspend(const char *name);

/**
 * @brief 恢复指定模块
 * 
 * @param name 模块名称
 * @return int 0表示成功，非0表示失败
 */
int module_resume(const char *name);

/**
 * @brief 获取模块状态
 * 
 * @param name 模块名称
 * @param status 返回的模块状态
 * @return int 0表示成功，非0表示失败
 */
int module_get_status(const char *name, module_status_t *status);

/**
 * @brief 获取所有模块信息
 * 
 * @param modules 模块信息数组
 * @param max_count 数组最大容量
 * @param count 实际模块数量
 * @return int 0表示成功，非0表示失败
 */
int module_get_all(module_info_t **modules, uint8_t max_count, uint8_t *count);

/**
 * @brief 检查模块依赖关系
 * 
 * @param name 模块名称
 * @return int 0表示依赖满足，负值表示存在未满足的依赖
 */
int module_check_dependencies(const char *name);

/* 模块注册宏 */
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    #define MODULE_REGISTER(name, desc, ver, prio, deps, dep_count, init_func, deinit_func, start_func, stop_func, suspend_func, resume_func, data) \
        static module_info_t _module_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .priority = prio, \
            .dependencies = deps, \
            .dependency_count = dep_count, \
            .interface = { \
                .init = init_func, \
                .deinit = deinit_func, \
                .start = start_func, \
                .stop = stop_func, \
                .suspend = suspend_func, \
                .resume = resume_func, \
            }, \
            .status = MODULE_STATUS_UNINITIALIZED, \
            .private_data = data \
        }; \
        __attribute__((constructor)) \
        static void _register_module_##name(void) { \
            module_register(&_module_##name); \
        }
#elif defined(COMPILER_KEIL) || defined(COMPILER_IAR)
    /* 对于不支持constructor属性的编译器，使用初始化数组 */
    #define MODULE_REGISTER(name, desc, ver, prio, deps, dep_count, init_func, deinit_func, start_func, stop_func, suspend_func, resume_func, data) \
        static module_info_t _module_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .priority = prio, \
            .dependencies = deps, \
            .dependency_count = dep_count, \
            .interface = { \
                .init = init_func, \
                .deinit = deinit_func, \
                .start = start_func, \
                .stop = stop_func, \
                .suspend = suspend_func, \
                .resume = resume_func, \
            }, \
            .status = MODULE_STATUS_UNINITIALIZED, \
            .private_data = data \
        }; \
        static module_info_t* const _registered_module_##name __attribute__((used, section(".modules_section"))) = &_module_##name;
#else
    /* 不支持自动注册的编译器，需手动调用模块注册函数 */
    #define MODULE_REGISTER(name, desc, ver, prio, deps, dep_count, init_func, deinit_func, start_func, stop_func, suspend_func, resume_func, data) \
        static module_info_t _module_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .priority = prio, \
            .dependencies = deps, \
            .dependency_count = dep_count, \
            .interface = { \
                .init = init_func, \
                .deinit = deinit_func, \
                .start = start_func, \
                .stop = stop_func, \
                .suspend = suspend_func, \
                .resume = resume_func, \
            }, \
            .status = MODULE_STATUS_UNINITIALIZED, \
            .private_data = data \
        };
#endif

#ifdef __cplusplus
}
#endif

#endif /* MODULE_SUPPORT_H */
