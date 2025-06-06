/**
 * @file rtos_api.h
 * @brief RTOS抽象层接口定义
 *
 * 该头文件定义了RTOS抽象层的统一接口，使上层应用能够与底层RTOS实现解耦
 */

#ifndef RTOS_API_H
#define RTOS_API_H

#include <stdint.h>
#include <stdbool.h>

/* 通用错误码定义 */
#define RTOS_OK            0    /**< 操作成功 */
#define RTOS_ERROR        -1    /**< 一般错误 */
#define RTOS_TIMEOUT      -2    /**< 操作超时 */
#define RTOS_NO_MEMORY    -3    /**< 内存不足 */
#define RTOS_INVALID_PARAM -4   /**< 无效参数 */

/* 线程优先级定义 */
typedef enum {
    RTOS_PRIORITY_IDLE = 0,     /**< 空闲优先级 */
    RTOS_PRIORITY_LOW,          /**< 低优先级 */
    RTOS_PRIORITY_NORMAL,       /**< 普通优先级 */
    RTOS_PRIORITY_HIGH,         /**< 高优先级 */
    RTOS_PRIORITY_REALTIME      /**< 实时优先级 */
} rtos_priority_t;

/* 事件标志等待模式 */
typedef enum {
    RTOS_EVENT_WAIT_ALL = 0,    /**< 等待所有指定的事件标志 */
    RTOS_EVENT_WAIT_ANY         /**< 等待任一指定的事件标志 */
} rtos_event_wait_mode_t;

/* 通用句柄定义 */
typedef void* rtos_handle_t;

/* 线程句柄 */
typedef rtos_handle_t rtos_thread_t;

/* 信号量句柄 */
typedef rtos_handle_t rtos_sem_t;

/* 互斥锁句柄 */
typedef rtos_handle_t rtos_mutex_t;

/* 事件组句柄 */
typedef rtos_handle_t rtos_event_group_t;

/* 消息队列句柄 */
typedef rtos_handle_t rtos_queue_t;

/* 定时器句柄 */
typedef rtos_handle_t rtos_timer_t;

/* 线程函数类型 */
typedef void (*rtos_thread_func_t)(void *arg);

/* 定时器回调函数类型 */
typedef void (*rtos_timer_func_t)(rtos_timer_t timer, void *arg);

/**
 * @brief 初始化RTOS
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_init(void);

/**
 * @brief 启动RTOS调度器
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_start_scheduler(void);

/**
 * @brief 创建线程
 * 
 * @param thread 线程句柄指针
 * @param name 线程名称
 * @param func 线程函数
 * @param arg 线程函数参数
 * @param stack_size 线程栈大小
 * @param priority 线程优先级
 * @return int 0表示成功，非0表示失败
 */
int rtos_thread_create(rtos_thread_t *thread, const char *name, rtos_thread_func_t func, 
                       void *arg, uint32_t stack_size, rtos_priority_t priority);

/**
 * @brief 删除线程
 * 
 * @param thread 线程句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_thread_delete(rtos_thread_t thread);

/**
 * @brief 线程睡眠（毫秒）
 * 
 * @param ms 睡眠毫秒数
 */
void rtos_thread_sleep_ms(uint32_t ms);

/**
 * @brief 获取当前线程句柄
 * 
 * @return rtos_thread_t 当前线程句柄
 */
rtos_thread_t rtos_thread_get_current(void);

/**
 * @brief 创建信号量
 * 
 * @param sem 信号量句柄指针
 * @param initial_count 初始计数值
 * @param max_count 最大计数值
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_create(rtos_sem_t *sem, uint32_t initial_count, uint32_t max_count);

/**
 * @brief 删除信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_delete(rtos_sem_t sem);

/**
 * @brief 获取信号量
 * 
 * @param sem 信号量句柄
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_take(rtos_sem_t sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_give(rtos_sem_t sem);

/**
 * @brief 创建互斥锁
 * 
 * @param mutex 互斥锁句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_create(rtos_mutex_t *mutex);

/**
 * @brief 删除互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_delete(rtos_mutex_t mutex);

/**
 * @brief 获取互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_lock(rtos_mutex_t mutex, uint32_t timeout_ms);

/**
 * @brief 释放互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_unlock(rtos_mutex_t mutex);

/**
 * @brief 创建事件标志组
 * 
 * @param event_group 事件标志组句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_create(rtos_event_group_t *event_group);

/**
 * @brief 删除事件标志组
 * 
 * @param event_group 事件标志组句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_delete(rtos_event_group_t event_group);

/**
 * @brief 设置事件标志
 * 
 * @param event_group 事件标志组句柄
 * @param bits_to_set 要设置的事件标志位
 * @return uint32_t 设置后的事件标志值
 */
uint32_t rtos_event_group_set_bits(rtos_event_group_t event_group, uint32_t bits_to_set);

/**
 * @brief 清除事件标志
 * 
 * @param event_group 事件标志组句柄
 * @param bits_to_clear 要清除的事件标志位
 * @return uint32_t 清除后的事件标志值
 */
uint32_t rtos_event_group_clear_bits(rtos_event_group_t event_group, uint32_t bits_to_clear);

/**
 * @brief 等待事件标志
 * 
 * @param event_group 事件标志组句柄
 * @param bits_to_wait 要等待的事件标志位
 * @param wait_mode 等待模式，ALL表示等待所有指定的位，ANY表示等待任一指定的位
 * @param clear_on_exit 退出时是否清除标志，true表示清除，false表示不清除
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return uint32_t 满足条件时的事件标志值，如果超时则返回0
 */
uint32_t rtos_event_group_wait_bits(rtos_event_group_t event_group, uint32_t bits_to_wait,
                                   rtos_event_wait_mode_t wait_mode, bool clear_on_exit,
                                   uint32_t timeout_ms);

/**
 * @brief 获取事件标志组当前值
 * 
 * @param event_group 事件标志组句柄
 * @return uint32_t 事件标志组当前值
 */
uint32_t rtos_event_group_get_bits(rtos_event_group_t event_group);

/**
 * @brief 创建消息队列
 * 
 * @param queue 消息队列句柄指针
 * @param item_size 队列项大小
 * @param item_count 队列项数量
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_create(rtos_queue_t *queue, uint32_t item_size, uint32_t item_count);

/**
 * @brief 删除消息队列
 * 
 * @param queue 消息队列句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_delete(rtos_queue_t queue);

/**
 * @brief 发送消息到队列
 * 
 * @param queue 消息队列句柄
 * @param item 消息项指针
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_send(rtos_queue_t queue, const void *item, uint32_t timeout_ms);

/**
 * @brief 从队列接收消息
 * 
 * @param queue 消息队列句柄
 * @param item 消息项指针
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_receive(rtos_queue_t queue, void *item, uint32_t timeout_ms);

/**
 * @brief 创建定时器
 * 
 * @param timer 定时器句柄指针
 * @param name 定时器名称
 * @param period_ms 定时器周期（毫秒）
 * @param auto_reload 自动重载标志，true表示周期性触发，false表示一次性触发
 * @param timer_id 定时器ID
 * @param callback 定时器回调函数
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_create(rtos_timer_t *timer, const char *name, uint32_t period_ms, 
                      bool auto_reload, uint32_t timer_id, rtos_timer_func_t callback);

/**
 * @brief 删除定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_delete(rtos_timer_t timer);

/**
 * @brief 启动定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_start(rtos_timer_t timer);

/**
 * @brief 停止定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_stop(rtos_timer_t timer);

/**
 * @brief 重置定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_reset(rtos_timer_t timer);

/**
 * @brief 获取系统当前tick数
 * 
 * @return uint32_t 系统当前tick数
 */
uint32_t rtos_get_tick_count(void);

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t rtos_get_time_ms(void);

/**
 * @brief 分配内存
 * 
 * @param size 需要分配的内存大小（字节）
 * @return void* 分配的内存指针，失败返回NULL
 */
void* rtos_malloc(uint32_t size);

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void rtos_free(void *ptr);

#endif /* RTOS_API_H */
