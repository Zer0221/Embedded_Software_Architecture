/**
 * @file freertos_adapter.c
 * @brief FreeRTOS适配层实现
 *
 * 该文件实现了FreeRTOS的适配层，将FreeRTOS的API映射到统一的RTOS抽象接口
 */

#include "rtos_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"

/**
 * @brief 初始化RTOS
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_init(void)
{
    /* FreeRTOS不需要特殊的初始化，直接返回成功 */
    return RTOS_OK;
}

/**
 * @brief 启动RTOS调度器
 * 
 * @return int 0表示成功，非0表示失败
 */
int rtos_start_scheduler(void)
{
    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();
    
    /* 如果执行到这里，说明调度器启动失败 */
    return RTOS_ERROR;
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
    UBaseType_t freertos_priority;
    BaseType_t result;
    
    if (thread == NULL || func == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 将抽象优先级映射到FreeRTOS优先级 */
    switch (priority) {
        case RTOS_PRIORITY_IDLE:
            freertos_priority = tskIDLE_PRIORITY;
            break;
        case RTOS_PRIORITY_LOW:
            freertos_priority = tskIDLE_PRIORITY + 1;
            break;
        case RTOS_PRIORITY_NORMAL:
            freertos_priority = tskIDLE_PRIORITY + 2;
            break;
        case RTOS_PRIORITY_HIGH:
            freertos_priority = tskIDLE_PRIORITY + 3;
            break;
        case RTOS_PRIORITY_REALTIME:
            freertos_priority = configMAX_PRIORITIES - 1;
            break;
        default:
            return RTOS_INVALID_PARAM;
    }
    
    /* 创建FreeRTOS任务 */
    result = xTaskCreate((TaskFunction_t)func, name, (stack_size / sizeof(StackType_t)), 
                         arg, freertos_priority, (TaskHandle_t *)thread);
    
    return (result == pdPASS) ? RTOS_OK : RTOS_NO_MEMORY;
}

/**
 * @brief 删除线程
 * 
 * @param thread 线程句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_thread_delete(rtos_thread_t thread)
{
    if (thread == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    vTaskDelete((TaskHandle_t)thread);
    return RTOS_OK;
}

/**
 * @brief 线程睡眠（毫秒）
 * 
 * @param ms 睡眠毫秒数
 */
void rtos_thread_sleep_ms(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    vTaskDelay(ticks);
}

/**
 * @brief 获取当前线程句柄
 * 
 * @return rtos_thread_t 当前线程句柄
 */
rtos_thread_t rtos_thread_get_current(void)
{
    return (rtos_thread_t)xTaskGetCurrentTaskHandle();
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
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    if (max_count == 1) {
        /* 二值信号量 */
        *sem = xSemaphoreCreateBinary();
        if (*sem != NULL && initial_count > 0) {
            xSemaphoreGive((SemaphoreHandle_t)*sem);
        }
    } else {
        /* 计数信号量 */
        *sem = xSemaphoreCreateCounting(max_count, initial_count);
    }
    
    return (*sem != NULL) ? RTOS_OK : RTOS_NO_MEMORY;
}

/**
 * @brief 删除信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_delete(rtos_sem_t sem)
{
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    vSemaphoreDelete((SemaphoreHandle_t)sem);
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
    TickType_t ticks;
    BaseType_t result;
    
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = 0;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    result = xSemaphoreTake((SemaphoreHandle_t)sem, ticks);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_TIMEOUT;
}

/**
 * @brief 释放信号量
 * 
 * @param sem 信号量句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_sem_give(rtos_sem_t sem)
{
    BaseType_t result;
    
    if (sem == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    result = xSemaphoreGive((SemaphoreHandle_t)sem);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 创建互斥锁
 * 
 * @param mutex 互斥锁句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_create(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    *mutex = xSemaphoreCreateMutex();
    
    return (*mutex != NULL) ? RTOS_OK : RTOS_NO_MEMORY;
}

/**
 * @brief 删除互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_delete(rtos_mutex_t mutex)
{
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
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
    TickType_t ticks;
    BaseType_t result;
    
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = 0;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    result = xSemaphoreTake((SemaphoreHandle_t)mutex, ticks);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_TIMEOUT;
}

/**
 * @brief 释放互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_mutex_unlock(rtos_mutex_t mutex)
{
    BaseType_t result;
    
    if (mutex == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    result = xSemaphoreGive((SemaphoreHandle_t)mutex);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 创建事件标志组
 * 
 * @param event_group 事件标志组句柄指针
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_create(rtos_event_group_t *event_group)
{
    if (event_group == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    *event_group = xEventGroupCreate();
    
    return (*event_group != NULL) ? RTOS_OK : RTOS_NO_MEMORY;
}

/**
 * @brief 删除事件标志组
 * 
 * @param event_group 事件标志组句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_event_group_delete(rtos_event_group_t event_group)
{
    if (event_group == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    vEventGroupDelete((EventGroupHandle_t)event_group);
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
    EventBits_t bits;
    
    if (event_group == NULL) {
        return 0;
    }
    
    bits = xEventGroupSetBits((EventGroupHandle_t)event_group, (EventBits_t)bits_to_set);
    
    return (uint32_t)bits;
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
    EventBits_t bits;
    
    if (event_group == NULL) {
        return 0;
    }
    
    bits = xEventGroupClearBits((EventGroupHandle_t)event_group, (EventBits_t)bits_to_clear);
    
    return (uint32_t)bits;
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
    TickType_t ticks;
    EventBits_t bits;
    BaseType_t clear_bits = clear_on_exit ? pdTRUE : pdFALSE;
    BaseType_t wait_all = (wait_mode == RTOS_EVENT_WAIT_ALL) ? pdTRUE : pdFALSE;
    
    if (event_group == NULL) {
        return 0;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = 0;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    bits = xEventGroupWaitBits((EventGroupHandle_t)event_group, 
                              (EventBits_t)bits_to_wait, 
                              clear_bits, 
                              wait_all, 
                              ticks);
    
    /* 检查是否满足等待条件 */
    if (wait_all) {
        if ((bits & bits_to_wait) != bits_to_wait) {
            return 0; /* 超时 */
        }
    } else {
        if ((bits & bits_to_wait) == 0) {
            return 0; /* 超时 */
        }
    }
    
    return (uint32_t)bits;
}

/**
 * @brief 获取事件标志组当前值
 * 
 * @param event_group 事件标志组句柄
 * @return uint32_t 事件标志组当前值
 */
uint32_t rtos_event_group_get_bits(rtos_event_group_t event_group)
{
    if (event_group == NULL) {
        return 0;
    }
    
    return (uint32_t)xEventGroupGetBits((EventGroupHandle_t)event_group);
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
    if (queue == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    *queue = xQueueCreate(item_count, item_size);
    
    return (*queue != NULL) ? RTOS_OK : RTOS_NO_MEMORY;
}

/**
 * @brief 删除消息队列
 * 
 * @param queue 消息队列句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_queue_delete(rtos_queue_t queue)
{
    if (queue == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    vQueueDelete((QueueHandle_t)queue);
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
    TickType_t ticks;
    BaseType_t result;
    
    if (queue == NULL || item == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = 0;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    result = xQueueSendToBack((QueueHandle_t)queue, item, ticks);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_TIMEOUT;
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
    TickType_t ticks;
    BaseType_t result;
    
    if (queue == NULL || item == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    /* 转换超时时间 */
    if (timeout_ms == 0) {
        ticks = 0;
    } else if (timeout_ms == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    result = xQueueReceive((QueueHandle_t)queue, item, ticks);
    
    return (result == pdTRUE) ? RTOS_OK : RTOS_TIMEOUT;
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
    TimerHandle_t timer_handle;
    UBaseType_t reload = auto_reload ? pdTRUE : pdFALSE;
    TickType_t period = pdMS_TO_TICKS(period_ms);
    
    if (timer == NULL || callback == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    timer_handle = xTimerCreate(name, period, reload, (void *)(uintptr_t)timer_id, 
                                (TimerCallbackFunction_t)callback);
    
    if (timer_handle == NULL) {
        return RTOS_NO_MEMORY;
    }
    
    *timer = (rtos_timer_t)timer_handle;
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
    BaseType_t result;
    
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    result = xTimerDelete((TimerHandle_t)timer, portMAX_DELAY);
    
    return (result == pdPASS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 启动定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_start(rtos_timer_t timer)
{
    BaseType_t result;
    
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStartFromISR((TimerHandle_t)timer, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStart((TimerHandle_t)timer, portMAX_DELAY);
    }
    
    return (result == pdPASS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 停止定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_stop(rtos_timer_t timer)
{
    BaseType_t result;
    
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStopFromISR((TimerHandle_t)timer, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStop((TimerHandle_t)timer, portMAX_DELAY);
    }
    
    return (result == pdPASS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 重置定时器
 * 
 * @param timer 定时器句柄
 * @return int 0表示成功，非0表示失败
 */
int rtos_timer_reset(rtos_timer_t timer)
{
    BaseType_t result;
    
    if (timer == NULL) {
        return RTOS_INVALID_PARAM;
    }
    
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerResetFromISR((TimerHandle_t)timer, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerReset((TimerHandle_t)timer, portMAX_DELAY);
    }
    
    return (result == pdPASS) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 获取系统当前tick数
 * 
 * @return uint32_t 系统当前tick数
 */
uint32_t rtos_get_tick_count(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/**
 * @brief 获取系统运行时间（毫秒）
 * 
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t rtos_get_time_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
 * @brief 分配内存
 * 
 * @param size 需要分配的内存大小（字节）
 * @return void* 分配的内存指针，失败返回NULL
 */
void* rtos_malloc(uint32_t size)
{
    return pvPortMalloc(size);
}

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void rtos_free(void *ptr)
{
    vPortFree(ptr);
}
