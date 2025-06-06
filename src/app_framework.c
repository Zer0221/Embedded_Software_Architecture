/**
 * @file app_framework.c
 * @brief 应用程序框架接口实现
 *
 * 该文件实现了应用程序框架接口，用于管理应用程序的初始化、启动和停止
 */

#include "common/app_framework.h"
#include "common/error_api.h"
#include <string.h>

#if (CURRENT_RTOS != RTOS_NONE)
#include "common/rtos_api.h"
#endif

/* 应用程序列表 */
static application_t *g_applications[MAX_APPLICATIONS] = {0};
static uint32_t g_app_count = 0;

#if (CURRENT_RTOS != RTOS_NONE)
static rtos_mutex_t g_app_mutex = NULL;
#endif

/**
 * @brief 初始化应用框架
 * 
 * @return int 0表示成功，非0表示失败
 */
static int app_framework_init(void)
{
    static bool initialized = false;
    
    if (initialized) {
        return 0;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 创建互斥锁 */
    if (rtos_mutex_create(&g_app_mutex) != 0) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
        return -1;
    }
#endif
    
    initialized = true;
    return 0;
}

/**
 * @brief 注册应用程序
 * 
 * @param app 应用程序结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_register(application_t *app)
{
    int i;
    
    if (app == NULL || app->name == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 初始化应用框架 */
    if (app_framework_init() != 0) {
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 检查应用是否已经注册 */
    for (i = 0; i < g_app_count; i++) {
        if (g_applications[i] != NULL && 
            strcmp(g_applications[i]->name, app->name) == 0) {
#if (CURRENT_RTOS != RTOS_NONE)
            rtos_mutex_unlock(g_app_mutex);
#endif
            REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
            return -1;
        }
    }
    
    /* 检查是否达到最大应用数 */
    if (g_app_count >= MAX_APPLICATIONS) {
#if (CURRENT_RTOS != RTOS_NONE)
        rtos_mutex_unlock(g_app_mutex);
#endif
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 注册应用 */
    g_applications[g_app_count] = app;
    g_app_count++;
    
    /* 设置初始状态 */
    app->state = APP_STATE_UNINITIALIZED;
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return 0;
}

/**
 * @brief 取消注册应用程序
 * 
 * @param name 应用程序名称
 * @return int 0表示成功，非0表示失败
 */
int app_unregister(const char *name)
{
    int i;
    bool found = false;
    
    if (name == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 查找应用 */
    for (i = 0; i < g_app_count; i++) {
        if (g_applications[i] != NULL && 
            strcmp(g_applications[i]->name, name) == 0) {
            /* 找到应用，停止并去初始化 */
            if (g_applications[i]->state == APP_STATE_RUNNING && 
                g_applications[i]->stop != NULL) {
                g_applications[i]->stop();
            }
            
            if (g_applications[i]->state != APP_STATE_UNINITIALIZED && 
                g_applications[i]->deinit != NULL) {
                g_applications[i]->deinit();
            }
            
            /* 移除应用 */
            g_applications[i] = NULL;
            found = true;
            
            /* 整理数组 */
            for (int j = i; j < g_app_count - 1; j++) {
                g_applications[j] = g_applications[j + 1];
            }
            g_applications[g_app_count - 1] = NULL;
            g_app_count--;
            
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    if (!found) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 比较应用优先级
 * 
 * @param a 应用A
 * @param b 应用B
 * @return int 比较结果
 */
static int compare_app_priority(const void *a, const void *b)
{
    application_t *app_a = *(application_t **)a;
    application_t *app_b = *(application_t **)b;
    
    if (app_a == NULL && app_b == NULL) {
        return 0;
    } else if (app_a == NULL) {
        return 1;
    } else if (app_b == NULL) {
        return -1;
    }
    
    return (int)app_a->priority - (int)app_b->priority;
}

/**
 * @brief 初始化所有应用程序
 * 
 * @param params 初始化参数
 * @return int 0表示成功，非0表示失败
 */
int app_init_all(void *params)
{
    int i;
    int result = 0;
    application_t *sorted_apps[MAX_APPLICATIONS];
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 复制应用列表并按优先级排序 */
    memcpy(sorted_apps, g_applications, sizeof(g_applications));
    qsort(sorted_apps, g_app_count, sizeof(application_t *), compare_app_priority);
    
    /* 按优先级初始化应用 */
    for (i = 0; i < g_app_count; i++) {
        if (sorted_apps[i] != NULL && 
            sorted_apps[i]->state == APP_STATE_UNINITIALIZED && 
            sorted_apps[i]->init != NULL) {
            if (sorted_apps[i]->init(params) != 0) {
                REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_INIT | ERROR_SEVERITY_ERROR);
                sorted_apps[i]->state = APP_STATE_ERROR;
                result = -1;
            } else {
                sorted_apps[i]->state = APP_STATE_INITIALIZED;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return result;
}

/**
 * @brief 启动所有应用程序
 * 
 * @return int 0表示成功，非0表示失败
 */
int app_start_all(void)
{
    int i;
    int result = 0;
    application_t *sorted_apps[MAX_APPLICATIONS];
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 复制应用列表并按优先级排序 */
    memcpy(sorted_apps, g_applications, sizeof(g_applications));
    qsort(sorted_apps, g_app_count, sizeof(application_t *), compare_app_priority);
    
    /* 按优先级启动应用 */
    for (i = 0; i < g_app_count; i++) {
        if (sorted_apps[i] != NULL && 
            sorted_apps[i]->state == APP_STATE_INITIALIZED && 
            sorted_apps[i]->start != NULL) {
            if (sorted_apps[i]->start() != 0) {
                REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_OPERATION | ERROR_SEVERITY_ERROR);
                sorted_apps[i]->state = APP_STATE_ERROR;
                result = -1;
            } else {
                sorted_apps[i]->state = APP_STATE_RUNNING;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return result;
}

/**
 * @brief 停止所有应用程序
 * 
 * @return int 0表示成功，非0表示失败
 */
int app_stop_all(void)
{
    int i;
    int result = 0;
    application_t *sorted_apps[MAX_APPLICATIONS];
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 复制应用列表并按优先级排序（反向顺序） */
    memcpy(sorted_apps, g_applications, sizeof(g_applications));
    qsort(sorted_apps, g_app_count, sizeof(application_t *), compare_app_priority);
    
    /* 按优先级停止应用（从低优先级到高优先级） */
    for (i = g_app_count - 1; i >= 0; i--) {
        if (sorted_apps[i] != NULL && 
            sorted_apps[i]->state == APP_STATE_RUNNING && 
            sorted_apps[i]->stop != NULL) {
            if (sorted_apps[i]->stop() != 0) {
                REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_OPERATION | ERROR_SEVERITY_ERROR);
                sorted_apps[i]->state = APP_STATE_ERROR;
                result = -1;
            } else {
                sorted_apps[i]->state = APP_STATE_STOPPED;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return result;
}

/**
 * @brief 发送消息给指定应用程序
 * 
 * @param name 目标应用程序名称
 * @param msg 消息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_send_message(const char *name, app_message_t *msg)
{
    application_t *app;
    
    if (name == NULL || msg == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 查找应用 */
    app = app_find(name);
    if (app == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 检查消息处理函数 */
    if (app->msg_handler == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_OPERATION | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 发送消息 */
    return app->msg_handler(msg, app->user_data);
}

/**
 * @brief 广播消息给所有应用程序
 * 
 * @param msg 消息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_broadcast_message(app_message_t *msg)
{
    int i;
    int result = 0;
    
    if (msg == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 向所有应用发送消息 */
    for (i = 0; i < g_app_count; i++) {
        if (g_applications[i] != NULL && 
            g_applications[i]->msg_handler != NULL) {
            if (g_applications[i]->msg_handler(msg, g_applications[i]->user_data) != 0) {
                result = -1;
            }
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return result;
}

/**
 * @brief 获取应用程序状态
 * 
 * @param name 应用程序名称
 * @param state 状态指针，用于返回状态
 * @return int 0表示成功，非0表示失败
 */
int app_get_state(const char *name, app_state_t *state)
{
    application_t *app;
    
    if (name == NULL || state == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_PARAM | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    /* 查找应用 */
    app = app_find(name);
    if (app == NULL) {
        REPORT_ERROR(ERROR_MODULE_APP | ERROR_TYPE_RESOURCE | ERROR_SEVERITY_ERROR);
        return -1;
    }
    
    *state = app->state;
    return 0;
}

/**
 * @brief 查找应用程序
 * 
 * @param name 应用程序名称
 * @return application_t* 应用程序结构体指针，如果未找到则返回NULL
 */
application_t *app_find(const char *name)
{
    int i;
    application_t *result = NULL;
    
    if (name == NULL) {
        return NULL;
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 获取互斥锁 */
    rtos_mutex_lock(g_app_mutex, UINT32_MAX);
#endif
    
    /* 查找应用 */
    for (i = 0; i < g_app_count; i++) {
        if (g_applications[i] != NULL && 
            strcmp(g_applications[i]->name, name) == 0) {
            result = g_applications[i];
            break;
        }
    }
    
#if (CURRENT_RTOS != RTOS_NONE)
    /* 释放互斥锁 */
    rtos_mutex_unlock(g_app_mutex);
#endif
    
    return result;
}
