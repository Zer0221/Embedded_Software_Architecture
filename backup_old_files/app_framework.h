/**
 * @file app_framework.h
 * @brief 应用程序框架接口定义
 *
 * 该头文件定义了应用程序框架接口，用于管理应用程序的初始化、启动和停止
 */

#ifndef APP_FRAMEWORK_H
#define APP_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>

/* 最大支持的应用数量 */
#define MAX_APPLICATIONS    10

/* 应用优先级定义 */
typedef enum {
    APP_PRIORITY_CRITICAL = 0,    /**< 关键优先级，最先初始化和启动 */
    APP_PRIORITY_HIGH,            /**< 高优先级 */
    APP_PRIORITY_NORMAL,          /**< 普通优先级 */
    APP_PRIORITY_LOW,             /**< 低优先级 */
    APP_PRIORITY_IDLE             /**< 空闲优先级，最后初始化和启动 */
} app_priority_t;

/* 应用状态定义 */
typedef enum {
    APP_STATE_UNINITIALIZED = 0,  /**< 未初始化状态 */
    APP_STATE_INITIALIZED,         /**< 已初始化状态 */
    APP_STATE_RUNNING,            /**< 运行状态 */
    APP_STATE_PAUSED,             /**< 暂停状态 */
    APP_STATE_STOPPED,            /**< 停止状态 */
    APP_STATE_ERROR               /**< 错误状态 */
} app_state_t;

/* 应用消息定义 */
typedef struct {
    uint32_t msg_id;              /**< 消息ID */
    uint32_t param1;              /**< 参数1 */
    uint32_t param2;              /**< 参数2 */
    void *data;                   /**< 数据指针 */
    uint32_t data_len;            /**< 数据长度 */
} app_message_t;

/* 应用消息处理函数类型 */
typedef int (*app_message_handler_t)(app_message_t *msg, void *user_data);

/* 应用程序结构体定义 */
typedef struct {
    const char *name;             /**< 应用名称 */
    app_priority_t priority;      /**< 应用优先级 */
    app_state_t state;            /**< 应用状态 */
    
    /* 应用生命周期函数 */
    int (*init)(void *params);    /**< 初始化函数 */
    int (*start)(void);           /**< 启动函数 */
    int (*pause)(void);           /**< 暂停函数 */
    int (*resume)(void);          /**< 恢复函数 */
    int (*stop)(void);            /**< 停止函数 */
    int (*deinit)(void);          /**< 去初始化函数 */
    
    /* 消息处理 */
    app_message_handler_t msg_handler;  /**< 消息处理函数 */
    void *user_data;              /**< 用户数据 */
} application_t;

/**
 * @brief 注册应用程序
 * 
 * @param app 应用程序结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_register(application_t *app);

/**
 * @brief 取消注册应用程序
 * 
 * @param name 应用程序名称
 * @return int 0表示成功，非0表示失败
 */
int app_unregister(const char *name);

/**
 * @brief 初始化所有应用程序
 * 
 * @param params 初始化参数
 * @return int 0表示成功，非0表示失败
 */
int app_init_all(void *params);

/**
 * @brief 启动所有应用程序
 * 
 * @return int 0表示成功，非0表示失败
 */
int app_start_all(void);

/**
 * @brief 停止所有应用程序
 * 
 * @return int 0表示成功，非0表示失败
 */
int app_stop_all(void);

/**
 * @brief 发送消息给指定应用程序
 * 
 * @param name 目标应用程序名称
 * @param msg 消息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_send_message(const char *name, app_message_t *msg);

/**
 * @brief 广播消息给所有应用程序
 * 
 * @param msg 消息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int app_broadcast_message(app_message_t *msg);

/**
 * @brief 获取应用程序状态
 * 
 * @param name 应用程序名称
 * @param state 状态指针，用于返回状态
 * @return int 0表示成功，非0表示失败
 */
int app_get_state(const char *name, app_state_t *state);

/**
 * @brief 查找应用程序
 * 
 * @param name 应用程序名称
 * @return application_t* 应用程序结构体指针，如果未找到则返回NULL
 */
application_t *app_find(const char *name);

#endif /* APP_FRAMEWORK_H */
