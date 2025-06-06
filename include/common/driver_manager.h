/**
 * @file driver_manager.h
 * @brief 驱动管理器接口定义
 *
 * 该头文件定义了驱动管理器相关的接口，提供驱动的注册、查找和管理功能
 */

#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"
#include "driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化驱动管理器
 * 
 * @return int 0表示成功，非0表示失败
 */
int driver_manager_init(void);

/**
 * @brief 注册驱动
 * 
 * @param driver_info 驱动信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int driver_register(driver_info_t *driver_info);

/**
 * @brief 注销驱动
 * 
 * @param name 驱动名称
 * @return int 0表示成功，非0表示失败
 */
int driver_unregister(const char *name);

/**
 * @brief 查找驱动
 * 
 * @param name 驱动名称
 * @return driver_info_t* 找到的驱动信息，未找到返回NULL
 */
driver_info_t *driver_find(const char *name);

/**
 * @brief 按类型查找驱动
 * 
 * @param type 驱动类型
 * @param drivers 驱动信息数组
 * @param max_count 数组最大容量
 * @param count 实际找到的驱动数量
 * @return int 0表示成功，非0表示失败
 */
int driver_find_by_type(driver_type_t type, driver_info_t **drivers, uint8_t max_count, uint8_t *count);

/**
 * @brief 获取所有已注册的驱动
 * 
 * @param drivers 驱动信息数组
 * @param max_count 数组最大容量
 * @param count 实际驱动数量
 * @return int 0表示成功，非0表示失败
 */
int driver_get_all(driver_info_t **drivers, uint8_t max_count, uint8_t *count);

/**
 * @brief 初始化所有驱动
 * 
 * @return int 0表示成功，非0表示失败
 */
int driver_init_all(void);

/**
 * @brief 自动注册驱动宏
 * 
 * 此宏用于支持驱动的自动注册功能，会在系统启动时自动注册驱动
 */
#if CONFIG_AUTO_DRIVER_REGISTER

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    #define DRIVER_REGISTER(name, desc, ver, type, interface, cap, init_func, deinit_func, data) \
        static driver_info_t _driver_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .type = type, \
            .interface = interface, \
            .capabilities = cap, \
            .status = DRIVER_STATUS_UNINITIALIZED, \
            .init = init_func, \
            .deinit = deinit_func, \
            .private_data = data \
        }; \
        __attribute__((constructor)) \
        static void _register_driver_##name(void) { \
            driver_register(&_driver_##name); \
        }
#elif defined(COMPILER_KEIL) || defined(COMPILER_IAR)
    /* 对于不支持constructor属性的编译器，使用初始化数组 */
    #define DRIVER_REGISTER(name, desc, ver, type, interface, cap, init_func, deinit_func, data) \
        static driver_info_t _driver_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .type = type, \
            .interface = interface, \
            .capabilities = cap, \
            .status = DRIVER_STATUS_UNINITIALIZED, \
            .init = init_func, \
            .deinit = deinit_func, \
            .private_data = data \
        }; \
        static driver_info_t* const _registered_driver_##name __attribute__((used, section(".drivers_section"))) = &_driver_##name;
#else
    /* 不支持自动注册的编译器，需手动调用驱动注册函数 */
    #define DRIVER_REGISTER(name, desc, ver, type, interface, cap, init_func, deinit_func, data) \
        static driver_info_t _driver_##name = { \
            .name = #name, \
            .description = desc, \
            .version = ver, \
            .type = type, \
            .interface = interface, \
            .capabilities = cap, \
            .status = DRIVER_STATUS_UNINITIALIZED, \
            .init = init_func, \
            .deinit = deinit_func, \
            .private_data = data \
        };
#endif

#else /* CONFIG_AUTO_DRIVER_REGISTER */

/* 不使用自动注册时的空定义 */
#define DRIVER_REGISTER(name, desc, ver, type, interface, cap, init_func, deinit_func, data) \
    static driver_info_t _driver_##name = { \
        .name = #name, \
        .description = desc, \
        .version = ver, \
        .type = type, \
        .interface = interface, \
        .capabilities = cap, \
        .status = DRIVER_STATUS_UNINITIALIZED, \
        .init = init_func, \
        .deinit = deinit_func, \
        .private_data = data \
    };

#endif /* CONFIG_AUTO_DRIVER_REGISTER */

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_MANAGER_H */
