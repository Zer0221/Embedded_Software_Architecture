/**
 * @file threadx_adapter.c
 * @brief ThreadX适配层实现
 *
 * 该文件实现了ThreadX的适配层，将ThreadX的API映射到统一的RTOS抽象接口
 */

#include "common/rtos_api.h"
#include "common/error_api.h"
#include "tx_api.h"
#include <stdlib.h>
#include <string.h>

/* 定义内存池大小，用于动态内存分配 */
#define TX_BYTE_POOL_SIZE (1024 * 10)

/* 内存池控制块 */
static TX_BYTE_POOL byte_pool;

/* 内存池缓冲区 */
static UCHAR byte_pool_buffer[TX_BYTE_POOL_SIZE];

/* 定时器回调函数参数结构体 */
typedef struct {
    rtos_timer_func_t callback;
    void* arg;
} timer_callback_arg_t;

/**
 * @brief 初始化RTOS
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_init(void)
{
    UINT status;
    
    /* 创建内存池 */
    status = tx_byte_pool_create(&byte_pool, "memory_pool", byte_pool_buffer, TX_BYTE_POOL_SIZE);
    
    /* ThreadX在使用前需要先调用tx_kernel_enter() */
    /* 但这个函数会立即启动调度器，因此在这里不调用它 */
    return (status == TX_SUCCESS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 启动RTOS调度器
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_start_scheduler(void)
{
    /* 启动ThreadX调度器 */
    tx_kernel_enter();
    
    /* 如果执行到这里，说明调度器启动失败 */
    return RTOS_ERROR;
}

/**
 * @brief 将抽象优先级映射到ThreadX优先级
 * 
 * @param priority 抽象优先级
 * @return UINT ThreadX优先级
 */
static UINT map_priority(rtos_priority_t priority)
{
    /* ThreadX优先级与FreeRTOS相反，数值越小优先级越高 */
    switch (priority) {
        case RTOS_PRIORITY_IDLE:
            return TX_MAX_PRIORITIES - 1;
        case RTOS_PRIORITY_LOW:
            return TX_MAX_PRIORITIES - 2;
        case RTOS_PRIORITY_NORMAL:
            return TX_MAX_PRIORITIES / 2;
        case RTOS_PRIORITY_HIGH:
            return 2;
        case RTOS_PRIORITY_REALTIME:
            return 1;
        default:
            return TX_MAX_PRIORITIES / 2;
    }
}

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
                       void *arg, uint32_t stack_size, rtos_priority_t priority)
{
    TX_THREAD *thread_ptr;
    void *stack_ptr;
    UINT tx_priority;
    UINT status;
    
    /* 参数检查 */
    if (thread == NULL || func == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配线程控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&thread_ptr, sizeof(TX_THREAD), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 从内存池分配线程栈内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, stack_size, TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        tx_byte_release(thread_ptr);
        return RTOS_NO_MEMORY;
    }
    
    /* 将抽象优先级映射到ThreadX优先级 */
    tx_priority = map_priority(priority);
    
    /* 创建ThreadX线程 */
    status = tx_thread_create(thread_ptr, (CHAR *)name, (void (*)(ULONG))func, 
                              (ULONG)arg, stack_ptr, stack_size, 
                              tx_priority, tx_priority, TX_NO_TIME_SLICE, TX_AUTO_START);
    
    if (status != TX_SUCCESS) {
        tx_byte_release(stack_ptr);
        tx_byte_release(thread_ptr);
        return RTOS_ERROR;
    }
    
    *thread = thread_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除线程
 * 
 * @param thread 线程句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_thread_delete(rtos_thread_t thread)
{
    UINT status;
    TX_THREAD *thread_ptr = (TX_THREAD *)thread;
    void *stack_ptr;
    
    /* 参数检查 */
    if (thread == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 保存栈指针以便后续释放 */
    stack_ptr = thread_ptr->tx_thread_stack_start;
    
    /* 终止并删除线程 */
    status = tx_thread_terminate(thread_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    status = tx_thread_delete(thread_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放栈内存和线程控制块内存 */
    tx_byte_release(stack_ptr);
    tx_byte_release(thread_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 线程睡眠（毫秒）
 * 
 * @param ms 睡眠毫秒数
 */
void rtos_thread_sleep_ms(uint32_t ms)
{
    ULONG ticks = ms * TX_TIMER_TICKS_PER_SECOND / 1000;
    
    /* 确保至少延迟一个tick */
    if (ticks == 0 && ms > 0) {
        ticks = 1;
    }
    
    tx_thread_sleep(ticks);
}

/**
 * @brief 获取当前线程句柄
 * 
 * @return rtos_thread_t 当前线程句柄
 */
rtos_thread_t rtos_thread_get_current(void)
{
    return (rtos_thread_t)tx_thread_identify();
}

/**
 * @brief 创建信号量
 * 
 * @param sem 信号量句柄指针
 * @param initial_count 初始计数值
 * @param max_count 最大计数值
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_create(rtos_sem_t *sem, uint32_t initial_count, uint32_t max_count)
{
    TX_SEMAPHORE *sem_ptr;
    UINT status;
    
    /* 参数检查 */
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配信号量控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&sem_ptr, sizeof(TX_SEMAPHORE), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 创建ThreadX信号量 */
    /* ThreadX信号量没有最大计数值限制，仅使用初始计数值 */
    status = tx_semaphore_create(sem_ptr, (CHAR *)"Semaphore", initial_count);
    
    if (status != TX_SUCCESS) {
        tx_byte_release(sem_ptr);
        return RTOS_ERROR;
    }
    
    *sem = sem_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_delete(rtos_sem_t sem)
{
    UINT status;
    TX_SEMAPHORE *sem_ptr = (TX_SEMAPHORE *)sem;
    
    /* 参数检查 */
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 删除ThreadX信号量 */
    status = tx_semaphore_delete(sem_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放信号量控制块内存 */
    tx_byte_release(sem_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 获取信号量
 * 
 * @param sem 信号量句柄
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_take(rtos_sem_t sem, uint32_t timeout_ms)
{
    UINT status;
    ULONG ticks;
    TX_SEMAPHORE *sem_ptr = (TX_SEMAPHORE *)sem;
    
    /* 参数检查 */
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = TX_NO_WAIT;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = TX_WAIT_FOREVER;
    } else {
        ticks = timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    
    /* 获取ThreadX信号量 */
    status = tx_semaphore_get(sem_ptr, ticks);
    
    return (status == TX_SUCCESS) ? RTOS_OK : 
           (status == TX_WAIT_ABORTED) ? RTOS_ERROR : RTOS_TIMEOUT;
}

/**
 * @brief 释放信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_give(rtos_sem_t sem)
{
    UINT status;
    TX_SEMAPHORE *sem_ptr = (TX_SEMAPHORE *)sem;
    
    /* 参数检查 */
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 释放ThreadX信号量 */
    status = tx_semaphore_put(sem_ptr);
    
    return (status == TX_SUCCESS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 创建互斥锁
 * 
 * @param mutex 互斥锁句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_create(rtos_mutex_t *mutex)
{
    TX_MUTEX *mutex_ptr;
    UINT status;
    
    /* 参数检查 */
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配互斥锁控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&mutex_ptr, sizeof(TX_MUTEX), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 创建ThreadX互斥锁 */
    status = tx_mutex_create(mutex_ptr, (CHAR *)"Mutex", TX_INHERIT);
    
    if (status != TX_SUCCESS) {
        tx_byte_release(mutex_ptr);
        return RTOS_ERROR;
    }
    
    *mutex = mutex_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_delete(rtos_mutex_t mutex)
{
    UINT status;
    TX_MUTEX *mutex_ptr = (TX_MUTEX *)mutex;
    
    /* 参数检查 */
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 删除ThreadX互斥锁 */
    status = tx_mutex_delete(mutex_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放互斥锁控制块内存 */
    tx_byte_release(mutex_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 获取互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_lock(rtos_mutex_t mutex, uint32_t timeout_ms)
{
    UINT status;
    ULONG ticks;
    TX_MUTEX *mutex_ptr = (TX_MUTEX *)mutex;
    
    /* 参数检查 */
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = TX_NO_WAIT;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = TX_WAIT_FOREVER;
    } else {
        ticks = timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    
    /* 获取ThreadX互斥锁 */
    status = tx_mutex_get(mutex_ptr, ticks);
    
    return (status == TX_SUCCESS) ? RTOS_OK : 
           (status == TX_WAIT_ABORTED) ? RTOS_ERROR : RTOS_TIMEOUT;
}

/**
 * @brief 释放互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_unlock(rtos_mutex_t mutex)
{
    UINT status;
    TX_MUTEX *mutex_ptr = (TX_MUTEX *)mutex;
    
    /* 参数检查 */
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 释放ThreadX互斥锁 */
    status = tx_mutex_put(mutex_ptr);
    
    return (status == TX_SUCCESS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 创建消息队列
 * 
 * @param queue 消息队列句柄指针
 * @param item_size 队列项大小
 * @param item_count 队列项数量
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_create(rtos_queue_t *queue, uint32_t item_size, uint32_t item_count)
{
    TX_QUEUE *queue_ptr;
    void *queue_mem;
    UINT status;
    ULONG queue_size;
    
    /* 参数检查 */
    if (queue == NULL || item_size == 0 || item_count == 0) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配队列控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&queue_ptr, sizeof(TX_QUEUE), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 计算队列所需内存大小 */
    queue_size = item_count * ((item_size + sizeof(ULONG) - 1) / sizeof(ULONG));
    
    /* 从内存池分配队列内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&queue_mem, queue_size * sizeof(ULONG), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        tx_byte_release(queue_ptr);
        return RTOS_NO_MEMORY;
    }
    
    /* 创建ThreadX队列 */
    status = tx_queue_create(queue_ptr, (CHAR *)"Queue", 
                             (item_size + sizeof(ULONG) - 1) / sizeof(ULONG), 
                             queue_mem, queue_size * sizeof(ULONG));
    
    if (status != TX_SUCCESS) {
        tx_byte_release(queue_mem);
        tx_byte_release(queue_ptr);
        return RTOS_ERROR;
    }
    
    *queue = queue_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除消息队列
 * 
 * @param queue 消息队列句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_delete(rtos_queue_t queue)
{
    UINT status;
    TX_QUEUE *queue_ptr = (TX_QUEUE *)queue;
    void *queue_mem;
    
    /* 参数检查 */
    if (queue == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 保存队列内存指针以便后续释放 */
    queue_mem = queue_ptr->tx_queue_start;
    
    /* 删除ThreadX队列 */
    status = tx_queue_delete(queue_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放队列内存和队列控制块内存 */
    tx_byte_release(queue_mem);
    tx_byte_release(queue_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 发送消息到队列
 * 
 * @param queue 消息队列句柄
 * @param item 消息项指针
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_send(rtos_queue_t queue, const void *item, uint32_t timeout_ms)
{
    UINT status;
    ULONG ticks;
    TX_QUEUE *queue_ptr = (TX_QUEUE *)queue;
    
    /* 参数检查 */
    if (queue == NULL || item == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = TX_NO_WAIT;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = TX_WAIT_FOREVER;
    } else {
        ticks = timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    
    /* 发送消息到ThreadX队列 */
    status = tx_queue_send(queue_ptr, (VOID *)item, ticks);
    
    return (status == TX_SUCCESS) ? RTOS_OK : 
           (status == TX_QUEUE_FULL) ? RTOS_TIMEOUT : RTOS_ERROR;
}

/**
 * @brief 从队列接收消息
 * 
 * @param queue 消息队列句柄
 * @param item 消息项指针
 * @param timeout_ms 超时时间（毫秒），0表示不等待，UINT32_MAX表示永久等待
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_receive(rtos_queue_t queue, void *item, uint32_t timeout_ms)
{
    UINT status;
    ULONG ticks;
    TX_QUEUE *queue_ptr = (TX_QUEUE *)queue;
    
    /* 参数检查 */
    if (queue == NULL || item == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = TX_NO_WAIT;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = TX_WAIT_FOREVER;
    } else {
        ticks = timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    
    /* 从ThreadX队列接收消息 */
    status = tx_queue_receive(queue_ptr, item, ticks);
    
    return (status == TX_SUCCESS) ? RTOS_OK : 
           (status == TX_QUEUE_EMPTY) ? RTOS_TIMEOUT : RTOS_ERROR;
}

/**
 * @brief 定时器回调函数包装器
 * 
 * @param timer_ptr ThreadX定时器指针
 */
static void timer_callback_wrapper(ULONG timer_ptr)
{
    TX_TIMER *tx_timer = (TX_TIMER *)timer_ptr;
    timer_callback_arg_t *cb_arg = (timer_callback_arg_t *)tx_timer->tx_timer_internal.tx_timer_internal_application_timer_input;
    
    if (cb_arg != NULL && cb_arg->callback != NULL) {
        cb_arg->callback((rtos_timer_t)tx_timer, cb_arg->arg);
    }
}

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
                      bool auto_reload, uint32_t timer_id, rtos_timer_func_t callback)
{
    TX_TIMER *timer_ptr;
    timer_callback_arg_t *cb_arg;
    UINT status;
    ULONG ticks;
    
    /* 参数检查 */
    if (timer == NULL || callback == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配定时器控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&timer_ptr, sizeof(TX_TIMER), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 从内存池分配回调参数内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&cb_arg, sizeof(timer_callback_arg_t), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        tx_byte_release(timer_ptr);
        return RTOS_NO_MEMORY;
    }
    
    /* 设置回调参数 */
    cb_arg->callback = callback;
    cb_arg->arg = (void *)timer_id;
    
    /* 转换周期为ticks */
    ticks = period_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
    if (ticks == 0) {
        ticks = 1;
    }
    
    /* 创建ThreadX定时器 */
    status = tx_timer_create(timer_ptr, (CHAR *)name, 
                             timer_callback_wrapper, (ULONG)timer_ptr, 
                             ticks, auto_reload ? ticks : 0, 
                             TX_NO_ACTIVATE);
    
    if (status != TX_SUCCESS) {
        tx_byte_release(cb_arg);
        tx_byte_release(timer_ptr);
        return RTOS_ERROR;
    }
    
    /* 存储回调参数 */
    timer_ptr->tx_timer_internal.tx_timer_internal_application_timer_input = (ULONG)cb_arg;
    
    *timer = timer_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_delete(rtos_timer_t timer)
{
    UINT status;
    TX_TIMER *timer_ptr = (TX_TIMER *)timer;
    timer_callback_arg_t *cb_arg;
    
    /* 参数检查 */
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 保存回调参数指针以便后续释放 */
    cb_arg = (timer_callback_arg_t *)timer_ptr->tx_timer_internal.tx_timer_internal_application_timer_input;
    
    /* 停止并删除ThreadX定时器 */
    status = tx_timer_deactivate(timer_ptr);
    if (status != TX_SUCCESS && status != TX_TIMER_NOT_ACTIVE) {
        return RTOS_ERROR;
    }
    
    status = tx_timer_delete(timer_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放回调参数内存和定时器控制块内存 */
    if (cb_arg != NULL) {
        tx_byte_release(cb_arg);
    }
    tx_byte_release(timer_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 启动定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_start(rtos_timer_t timer)
{
    UINT status;
    TX_TIMER *timer_ptr = (TX_TIMER *)timer;
    
    /* 参数检查 */
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 激活ThreadX定时器 */
    status = tx_timer_activate(timer_ptr);
    
    return (status == TX_SUCCESS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 停止定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_stop(rtos_timer_t timer)
{
    UINT status;
    TX_TIMER *timer_ptr = (TX_TIMER *)timer;
    
    /* 参数检查 */
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 停止ThreadX定时器 */
    status = tx_timer_deactivate(timer_ptr);
    
    return (status == TX_SUCCESS || status == TX_TIMER_NOT_ACTIVE) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 重置定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_reset(rtos_timer_t timer)
{
    UINT status;
    TX_TIMER *timer_ptr = (TX_TIMER *)timer;
    
    /* 参数检查 */
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 重置ThreadX定时器 */
    status = tx_timer_deactivate(timer_ptr);
    if (status != TX_SUCCESS && status != TX_TIMER_NOT_ACTIVE) {
        return RTOS_ERROR;
    }
    
    status = tx_timer_activate(timer_ptr);
    
    return (status == TX_SUCCESS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 获取系统当前tick数
 * 
 * @return uint32_t 系统当前tick数
 */
uint32_t rtos_get_tick_count(void)
{
    return (uint32_t)tx_time_get();
}

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t rtos_get_time_ms(void)
{
    return (uint32_t)(tx_time_get() * 1000 / TX_TIMER_TICKS_PER_SECOND);
}

/**
 * @brief 创建事件标志组
 * 
 * @param event_group 事件标志组句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_create(rtos_event_group_t *event_group)
{
    TX_EVENT_FLAGS_GROUP *event_group_ptr;
    UINT status;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 从内存池分配事件标志组控制块内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&event_group_ptr, sizeof(TX_EVENT_FLAGS_GROUP), TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return RTOS_NO_MEMORY;
    }
    
    /* 创建ThreadX事件标志组 */
    status = tx_event_flags_create(event_group_ptr, (CHAR *)"EventGroup");
    
    if (status != TX_SUCCESS) {
        tx_byte_release(event_group_ptr);
        return RTOS_ERROR;
    }
    
    *event_group = event_group_ptr;
    return RTOS_OK;
}

/**
 * @brief 删除事件标志组
 * 
 * @param event_group 事件标志组句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_delete(rtos_event_group_t event_group)
{
    UINT status;
    TX_EVENT_FLAGS_GROUP *event_group_ptr = (TX_EVENT_FLAGS_GROUP *)event_group;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 删除ThreadX事件标志组 */
    status = tx_event_flags_delete(event_group_ptr);
    if (status != TX_SUCCESS) {
        return RTOS_ERROR;
    }
    
    /* 释放事件标志组控制块内存 */
    tx_byte_release(event_group_ptr);
    
    return RTOS_OK;
}

/**
 * @brief 设置事件标志
 * 
 * @param event_group 事件标志组句柄
 * @param bits_to_set 要设置的事件标志位
 * @return uint32_t 设置后的事件标志值
 */
uint32_t rtos_event_group_set_bits(rtos_event_group_t event_group, uint32_t bits_to_set)
{
    UINT status;
    TX_EVENT_FLAGS_GROUP *event_group_ptr = (TX_EVENT_FLAGS_GROUP *)event_group;
    ULONG current_flags;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return 0;
    }
    
    /* 设置ThreadX事件标志 */
    status = tx_event_flags_set(event_group_ptr, bits_to_set, TX_OR);
    if (status != TX_SUCCESS) {
        return 0;
    }
    
    /* 获取当前事件标志值 */
    status = tx_event_flags_info_get(event_group_ptr, NULL, &current_flags, NULL, NULL, NULL);
    if (status != TX_SUCCESS) {
        return 0;
    }
    
    return (uint32_t)current_flags;
}

/**
 * @brief 清除事件标志
 * 
 * @param event_group 事件标志组句柄
 * @param bits_to_clear 要清除的事件标志位
 * @return uint32_t 清除后的事件标志值
 */
uint32_t rtos_event_group_clear_bits(rtos_event_group_t event_group, uint32_t bits_to_clear)
{
    UINT status;
    TX_EVENT_FLAGS_GROUP *event_group_ptr = (TX_EVENT_FLAGS_GROUP *)event_group;
    ULONG current_flags;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return 0;
    }
    
    /* 清除ThreadX事件标志 */
    status = tx_event_flags_set(event_group_ptr, ~bits_to_clear, TX_AND);
    if (status != TX_SUCCESS) {
        return 0;
    }
    
    /* 获取当前事件标志值 */
    status = tx_event_flags_info_get(event_group_ptr, NULL, &current_flags, NULL, NULL, NULL);
    if (status != TX_SUCCESS) {
        return 0;
    }
    
    return (uint32_t)current_flags;
}

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
                                   uint32_t timeout_ms)
{
    UINT status;
    TX_EVENT_FLAGS_GROUP *event_group_ptr = (TX_EVENT_FLAGS_GROUP *)event_group;
    ULONG actual_flags = 0;
    ULONG ticks;
    UINT get_option;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return 0;
    }
    
    /* 设置等待选项 */
    if (wait_mode == RTOS_EVENT_WAIT_ALL) {
        get_option = TX_AND;
    } else {
        get_option = TX_OR;
    }
    
    /* 设置清除选项 */
    if (clear_on_exit) {
        get_option |= TX_EVENT_FLAGS_CLEAR;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = TX_NO_WAIT;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = TX_WAIT_FOREVER;
    } else {
        ticks = timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
    }
    
    /* 等待ThreadX事件标志 */
    status = tx_event_flags_get(event_group_ptr, bits_to_wait, get_option, &actual_flags, ticks);
    
    return (status == TX_SUCCESS) ? (uint32_t)actual_flags : 0;
}

/**
 * @brief 获取事件标志组当前值
 * 
 * @param event_group 事件标志组句柄
 * @return uint32_t 事件标志组当前值
 */
uint32_t rtos_event_group_get_bits(rtos_event_group_t event_group)
{
    UINT status;
    TX_EVENT_FLAGS_GROUP *event_group_ptr = (TX_EVENT_FLAGS_GROUP *)event_group;
    ULONG current_flags;
    
    /* 参数检查 */
    if (event_group == NULL) {
        return 0;
    }
    
    /* 获取当前事件标志值 */
    status = tx_event_flags_info_get(event_group_ptr, NULL, &current_flags, NULL, NULL, NULL);
    if (status != TX_SUCCESS) {
        return 0;
    }
    
    return (uint32_t)current_flags;
}

/**
 * @brief 分配内存
 * 
 * @param size 需要分配的内存大小（字节）
 * @return void* 分配的内存指针，失败返回NULL
 */
void* rtos_malloc(uint32_t size)
{
    UINT status;
    void *ptr = NULL;
    
    /* 从内存池分配内存 */
    status = tx_byte_allocate(&byte_pool, (VOID **)&ptr, size, TX_NO_WAIT);
    if (status != TX_SUCCESS) {
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void rtos_free(void *ptr)
{
    if (ptr != NULL) {
        tx_byte_release(ptr);
    }
}
