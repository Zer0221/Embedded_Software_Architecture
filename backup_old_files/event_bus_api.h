/**
 * @file event_bus_api.h
 * @brief 事件总线接口抽象层定义
 *
 * 该头文件定义了事件总线的统一抽象接口，用于系统内部组件间的消息传递和事件通知
 */

#ifndef EVENT_BUS_API_H
#define EVENT_BUS_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 事件总线句柄 */
typedef driver_handle_t event_bus_handle_t;

/* 事件ID类型 */
typedef uint32_t event_id_t;

/* 事件优先级枚举 */
typedef enum {
    EVENT_PRIORITY_LOW,       /**< 低优先级 */
    EVENT_PRIORITY_NORMAL,    /**< 普通优先级 */
    EVENT_PRIORITY_HIGH,      /**< 高优先级 */
    EVENT_PRIORITY_CRITICAL   /**< 关键优先级 */
} event_priority_t;

/* 事件结构体 */
typedef struct {
    event_id_t id;            /**< 事件ID */
    uint32_t timestamp;       /**< 时间戳 */
    event_priority_t priority;/**< 优先级 */
    void *data;               /**< 事件数据 */
    size_t data_len;          /**< 数据长度 */
    uint32_t source;          /**< 事件源标识 */
} event_t;

/* 事件回调函数类型 */
typedef void (*event_callback_t)(const event_t *event, void *user_data);

/* 事件过滤器函数类型 */
typedef bool (*event_filter_t)(const event_t *event, void *filter_data);

/* 事件总线配置结构体 */
typedef struct {
    uint16_t max_subscribers;     /**< 最大订阅者数量 */
    uint16_t queue_size;          /**< 事件队列大小 */
    bool thread_safe;             /**< 是否线程安全 */
    uint32_t dispatch_timeout_ms; /**< 派发超时时间 */
} event_bus_config_t;

/**
 * @brief 初始化事件总线
 * 
 * @param config 配置参数
 * @param handle 返回的事件总线句柄
 * @return int 0表示成功，非0表示失败
 */
int event_bus_init(const event_bus_config_t *config, event_bus_handle_t *handle);

/**
 * @brief 订阅事件
 * 
 * @param handle 事件总线句柄
 * @param event_id 事件ID，0表示订阅所有事件
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @param subscriber_id 返回的订阅者ID
 * @return int 0表示成功，非0表示失败
 */
int event_bus_subscribe(event_bus_handle_t handle, 
                       event_id_t event_id, 
                       event_callback_t callback, 
                       void *user_data, 
                       uint32_t *subscriber_id);

/**
 * @brief 带过滤器的订阅事件
 * 
 * @param handle 事件总线句柄
 * @param event_id 事件ID，0表示订阅所有事件
 * @param callback 回调函数
 * @param filter 过滤器函数
 * @param filter_data 过滤器数据
 * @param user_data 用户数据指针
 * @param subscriber_id 返回的订阅者ID
 * @return int 0表示成功，非0表示失败
 */
int event_bus_subscribe_filtered(event_bus_handle_t handle, 
                                event_id_t event_id, 
                                event_callback_t callback, 
                                event_filter_t filter, 
                                void *filter_data, 
                                void *user_data, 
                                uint32_t *subscriber_id);

/**
 * @brief 取消订阅
 * 
 * @param handle 事件总线句柄
 * @param subscriber_id 订阅者ID
 * @return int 0表示成功，非0表示失败
 */
int event_bus_unsubscribe(event_bus_handle_t handle, uint32_t subscriber_id);

/**
 * @brief 发布事件
 * 
 * @param handle 事件总线句柄
 * @param event 事件结构体指针
 * @return int 0表示成功，非0表示失败
 */
int event_bus_publish(event_bus_handle_t handle, const event_t *event);

/**
 * @brief 创建事件
 * 
 * @param id 事件ID
 * @param data 事件数据
 * @param data_len 数据长度
 * @param priority 优先级
 * @param source 事件源标识
 * @param event 返回的事件结构体指针
 * @return int 0表示成功，非0表示失败
 */
int event_bus_create_event(event_id_t id, 
                          const void *data, 
                          size_t data_len, 
                          event_priority_t priority, 
                          uint32_t source, 
                          event_t *event);

/**
 * @brief 销毁事件
 * 
 * @param event 事件结构体指针
 * @return int 0表示成功，非0表示失败
 */
int event_bus_destroy_event(event_t *event);

/**
 * @brief 同步派发事件（阻塞直到所有订阅者处理完）
 * 
 * @param handle 事件总线句柄
 * @param event 事件结构体指针
 * @return int 0表示成功，非0表示失败
 */
int event_bus_publish_sync(event_bus_handle_t handle, const event_t *event);

/**
 * @brief 开始事件派发（启动事件处理线程）
 * 
 * @param handle 事件总线句柄
 * @return int 0表示成功，非0表示失败
 */
int event_bus_start(event_bus_handle_t handle);

/**
 * @brief 停止事件派发（停止事件处理线程）
 * 
 * @param handle 事件总线句柄
 * @return int 0表示成功，非0表示失败
 */
int event_bus_stop(event_bus_handle_t handle);

/**
 * @brief 获取队列中的事件数量
 * 
 * @param handle 事件总线句柄
 * @param count 返回的事件数量
 * @return int 0表示成功，非0表示失败
 */
int event_bus_get_queue_count(event_bus_handle_t handle, uint32_t *count);

/**
 * @brief 清空事件队列
 * 
 * @param handle 事件总线句柄
 * @return int 0表示成功，非0表示失败
 */
int event_bus_clear_queue(event_bus_handle_t handle);

/**
 * @brief 获取订阅者数量
 * 
 * @param handle 事件总线句柄
 * @param event_id 事件ID，0表示所有事件
 * @param count 返回的订阅者数量
 * @return int 0表示成功，非0表示失败
 */
int event_bus_get_subscriber_count(event_bus_handle_t handle, 
                                  event_id_t event_id, 
                                  uint32_t *count);

#endif /* EVENT_BUS_API_H */
