/**
 * @file driver_common.c
 * @brief 驱动层通用接口实现
 *
 * 该文件实现了驱动层的通用功能和辅助函数
 */

#include "common/driver_api.h"
#include "common/error_api.h"
#include "common/log_api.h"
#include <string.h>
#include <stdlib.h>

/* 驱动注册表结构 */
typedef struct {
    const char* name;             /* 驱动名称 */
    driver_init_func_t init;      /* 初始化函数 */
    driver_deinit_func_t deinit;  /* 去初始化函数 */
    bool initialized;             /* 初始化状态 */
} driver_entry_t;

/* 驱动注册表 */
#define MAX_DRIVERS 32
static driver_entry_t drivers[MAX_DRIVERS];
static int driver_count = 0;

/**
 * @brief 注册驱动
 *
 * @param name 驱动名称
 * @param init_func 初始化函数
 * @param deinit_func 去初始化函数
 * @return 成功返回0，失败返回负值
 */
int driver_register(const char* name, driver_init_func_t init_func, driver_deinit_func_t deinit_func)
{
    if (!name || !init_func || !deinit_func) {
        set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_PARAM, "驱动注册参数无效");
        return DRIVER_INVALID_PARAM;
    }
    
    /* 检查是否已达到最大驱动数量 */
    if (driver_count >= MAX_DRIVERS) {
        set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_RESOURCE, "驱动注册表已满");
        return DRIVER_ERROR;
    }
    
    /* 检查驱动是否已经注册 */
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_STATE, "驱动已注册");
            return DRIVER_ERROR;
        }
    }
    
    /* 注册驱动 */
    drivers[driver_count].name = name;
    drivers[driver_count].init = init_func;
    drivers[driver_count].deinit = deinit_func;
    drivers[driver_count].initialized = false;
    driver_count++;
    
    log_info("驱动已注册: %s", name);
    
    return DRIVER_OK;
}

/**
 * @brief 初始化驱动
 *
 * @param name 驱动名称
 * @return 成功返回0，失败返回负值
 */
int driver_init(const char* name)
{
    if (!name) {
        set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_PARAM, "驱动名称无效");
        return DRIVER_INVALID_PARAM;
    }
    
    /* 查找驱动 */
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            /* 检查驱动是否已初始化 */
            if (drivers[i].initialized) {
                log_warn("驱动已初始化: %s", name);
                return DRIVER_OK;
            }
            
            /* 初始化驱动 */
            int result = drivers[i].init();
            if (result != DRIVER_OK) {
                log_error("驱动初始化失败: %s, 错误码: %d", name, result);
                return result;
            }
            
            drivers[i].initialized = true;
            log_info("驱动已初始化: %s", name);
            return DRIVER_OK;
        }
    }
    
    set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_STATE, "驱动未注册");
    return DRIVER_ERROR;
}

/**
 * @brief 去初始化驱动
 *
 * @param name 驱动名称
 * @return 成功返回0，失败返回负值
 */
int driver_deinit(const char* name)
{
    if (!name) {
        set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_PARAM, "驱动名称无效");
        return DRIVER_INVALID_PARAM;
    }
    
    /* 查找驱动 */
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            /* 检查驱动是否已初始化 */
            if (!drivers[i].initialized) {
                log_warn("驱动未初始化: %s", name);
                return DRIVER_OK;
            }
            
            /* 去初始化驱动 */
            int result = drivers[i].deinit();
            if (result != DRIVER_OK) {
                log_error("驱动去初始化失败: %s, 错误码: %d", name, result);
                return result;
            }
            
            drivers[i].initialized = false;
            log_info("驱动已去初始化: %s", name);
            return DRIVER_OK;
        }
    }
    
    set_error(ERROR_MODULE_DRIVER | ERROR_TYPE_STATE, "驱动未注册");
    return DRIVER_ERROR;
}

/**
 * @brief 初始化所有驱动
 *
 * @return 成功初始化的驱动数量
 */
int driver_init_all(void)
{
    int success_count = 0;
    
    for (int i = 0; i < driver_count; i++) {
        /* 跳过已初始化的驱动 */
        if (drivers[i].initialized) {
            success_count++;
            continue;
        }
        
        /* 初始化驱动 */
        int result = drivers[i].init();
        if (result == DRIVER_OK) {
            drivers[i].initialized = true;
            success_count++;
            log_info("驱动已初始化: %s", drivers[i].name);
        } else {
            log_error("驱动初始化失败: %s, 错误码: %d", drivers[i].name, result);
        }
    }
    
    log_info("已初始化 %d/%d 个驱动", success_count, driver_count);
    
    return success_count;
}

/**
 * @brief 去初始化所有驱动
 */
void driver_deinit_all(void)
{
    for (int i = driver_count - 1; i >= 0; i--) {
        /* 跳过未初始化的驱动 */
        if (!drivers[i].initialized) {
            continue;
        }
        
        /* 去初始化驱动 */
        int result = drivers[i].deinit();
        if (result == DRIVER_OK) {
            drivers[i].initialized = false;
            log_info("驱动已去初始化: %s", drivers[i].name);
        } else {
            log_error("驱动去初始化失败: %s, 错误码: %d", drivers[i].name, result);
        }
    }
    
    log_info("所有驱动已去初始化");
}

/**
 * @brief 判断驱动是否已初始化
 *
 * @param name 驱动名称
 * @return 已初始化返回true，否则返回false
 */
bool driver_is_initialized(const char* name)
{
    if (!name) {
        return false;
    }
    
    /* 查找驱动 */
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            return drivers[i].initialized;
        }
    }
    
    return false;
}
