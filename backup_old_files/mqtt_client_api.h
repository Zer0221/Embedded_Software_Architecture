/**
 * @file mqtt_client_api.h
 * @brief MQTT客户端接口定义
 *
 * 该头文件定义了MQTT客户端的统一接口，支持MQTT 3.1和5.0版本，提供消息发布、订阅等功能
 */

#ifndef MQTT_CLIENT_API_H
#define MQTT_CLIENT_API_H

#include <stdint.h>
#include <stdbool.h>
#include "network_api.h"
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MQTT版本 */
typedef enum {
    MQTT_PROTOCOL_V3_1,      /**< MQTT 3.1 */
    MQTT_PROTOCOL_V3_1_1,    /**< MQTT 3.1.1 */
    MQTT_PROTOCOL_V5_0       /**< MQTT 5.0 */
} mqtt_protocol_ver_t;

/* MQTT QoS级别 */
typedef enum {
    MQTT_QOS_0,              /**< 最多发送一次 */
    MQTT_QOS_1,              /**< 至少发送一次 */
    MQTT_QOS_2               /**< 只发送一次 */
} mqtt_qos_t;

/* MQTT客户端句柄 */
typedef void* mqtt_client_handle_t;

/* MQTT连接信息 */
typedef struct {
    char host[128];            /**< 服务器地址 */
    uint16_t port;             /**< 服务器端口 */
    bool use_ssl;              /**< 是否使用SSL/TLS */
    const char* client_id;     /**< 客户端ID */
    const char* username;      /**< 用户名 */
    const char* password;      /**< 密码 */
    uint16_t keepalive;        /**< 保活时间(秒) */
    bool clean_session;        /**< 是否清除会话 */
    const char* lwt_topic;     /**< 遗嘱主题 */
    const char* lwt_msg;       /**< 遗嘱消息 */
    uint32_t lwt_msg_len;      /**< 遗嘱消息长度 */
    mqtt_qos_t lwt_qos;        /**< 遗嘱QoS */
    bool lwt_retain;           /**< 遗嘱是否保留 */
    bool disable_auto_reconnect;/**< 是否禁用自动重连 */
    uint8_t reconnect_max_times;/**< 最大重连次数 */
    mqtt_protocol_ver_t protocol_ver;/**< MQTT协议版本 */
} mqtt_client_config_t;

/* MQTT TLS配置 */
typedef struct {
    const char* ca_cert;           /**< CA证书 */
    const char* client_cert;       /**< 客户端证书 */
    const char* client_key;        /**< 客户端密钥 */
    const char* server_cert;       /**< 服务器证书 */
    bool skip_cert_verify;         /**< 是否跳过证书验证 */
    bool use_global_ca_store;      /**< 是否使用全局CA存储 */
    const char* alpn_protos[5];    /**< ALPN协议 */
    uint8_t alpn_count;            /**< ALPN协议数量 */
    char server_name[128];         /**< SNI服务器名称 */
} mqtt_client_tls_config_t;

/* MQTT事件类型 */
typedef enum {
    MQTT_EVENT_CONNECTED,           /**< 已连接 */
    MQTT_EVENT_DISCONNECTED,        /**< 已断开连接 */
    MQTT_EVENT_SUBSCRIBED,          /**< 已订阅 */
    MQTT_EVENT_UNSUBSCRIBED,        /**< 已取消订阅 */
    MQTT_EVENT_PUBLISHED,           /**< 已发布 */
    MQTT_EVENT_DATA,                /**< 收到数据 */
    MQTT_EVENT_ERROR,               /**< 错误 */
    MQTT_EVENT_BEFORE_CONNECT       /**< 连接前 */
} mqtt_event_id_t;

/* MQTT错误类型 */
typedef enum {
    MQTT_ERR_NONE = 0,              /**< 无错误 */
    MQTT_ERR_NETWORK_FAILURE,       /**< 网络错误 */
    MQTT_ERR_PROTOCOL_ERROR,        /**< 协议错误 */
    MQTT_ERR_NOT_SUPPORTED,         /**< 不支持的功能 */
    MQTT_ERR_NOT_AUTHORIZED,        /**< 未授权 */
    MQTT_ERR_TIMEOUT,               /**< 超时 */
    MQTT_ERR_MEMORY,                /**< 内存错误 */
    MQTT_ERR_UNKNOWN                /**< 未知错误 */
} mqtt_error_t;

/* MQTT事件数据 */
typedef struct {
    mqtt_event_id_t event_id;       /**< 事件类型 */
    mqtt_client_handle_t client;    /**< 客户端句柄 */
    void* user_context;             /**< 用户上下文 */
    char* data;                     /**< 数据 */
    int data_len;                   /**< 数据长度 */
    int total_data_len;             /**< 总数据长度 */
    char* topic;                    /**< 主题 */
    int topic_len;                  /**< 主题长度 */
    int msg_id;                     /**< 消息ID */
    mqtt_error_t error_code;        /**< 错误码 */
} mqtt_event_data_t;

/* MQTT事件回调函数类型 */
typedef void (*mqtt_event_callback_t)(mqtt_event_data_t* event);

/**
 * @brief 初始化MQTT客户端
 * 
 * @param config 配置参数
 * @param handle 返回的客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_init(const mqtt_client_config_t* config, mqtt_client_handle_t* handle);

/**
 * @brief 设置MQTT TLS配置
 * 
 * @param handle 客户端句柄
 * @param tls_config TLS配置
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_set_tls(mqtt_client_handle_t handle, const mqtt_client_tls_config_t* tls_config);

/**
 * @brief 设置MQTT事件回调
 * 
 * @param handle 客户端句柄
 * @param event_callback 事件回调函数
 * @param user_context 用户上下文
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_set_event_callback(mqtt_client_handle_t handle, mqtt_event_callback_t event_callback, void* user_context);

/**
 * @brief 连接MQTT服务器
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_connect(mqtt_client_handle_t handle);

/**
 * @brief 断开MQTT连接
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_disconnect(mqtt_client_handle_t handle);

/**
 * @brief 订阅MQTT主题
 * 
 * @param handle 客户端句柄
 * @param topic 主题
 * @param qos QoS级别
 * @param msg_id 返回的消息ID
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_subscribe(mqtt_client_handle_t handle, const char* topic, mqtt_qos_t qos, int* msg_id);

/**
 * @brief 取消订阅MQTT主题
 * 
 * @param handle 客户端句柄
 * @param topic 主题
 * @param msg_id 返回的消息ID
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_unsubscribe(mqtt_client_handle_t handle, const char* topic, int* msg_id);

/**
 * @brief 发布MQTT消息
 * 
 * @param handle 客户端句柄
 * @param topic 主题
 * @param data 数据
 * @param len 数据长度
 * @param qos QoS级别
 * @param retain 是否保留
 * @param msg_id 返回的消息ID
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_publish(mqtt_client_handle_t handle, const char* topic, const void* data, int len, mqtt_qos_t qos, bool retain, int* msg_id);

/**
 * @brief 释放MQTT客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_destroy(mqtt_client_handle_t handle);

/**
 * @brief 停止MQTT客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_stop(mqtt_client_handle_t handle);

/**
 * @brief 恢复MQTT客户端
 * 
 * @param handle 客户端句柄
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_resume(mqtt_client_handle_t handle);

/**
 * @brief 获取MQTT客户端状态
 * 
 * @param handle 客户端句柄
 * @param connected 返回的连接状态
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_get_state(mqtt_client_handle_t handle, bool* connected);

/**
 * @brief 获取MQTT客户端配置
 * 
 * @param handle 客户端句柄
 * @param config 返回的配置
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_get_config(mqtt_client_handle_t handle, mqtt_client_config_t* config);

/**
 * @brief 设置MQTT客户端遗嘱
 * 
 * @param handle 客户端句柄
 * @param topic 遗嘱主题
 * @param msg 遗嘱消息
 * @param msg_len 消息长度
 * @param qos QoS级别
 * @param retain 是否保留
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_set_lwt(mqtt_client_handle_t handle, const char* topic, const char* msg, int msg_len, mqtt_qos_t qos, bool retain);

#ifdef MQTT_ENABLE_V5
/**
 * @brief 设置MQTT 5.0属性
 * 
 * @param handle 客户端句柄
 * @param property_id 属性ID
 * @param value 属性值
 * @param value_len 属性值长度
 * @return int 成功返回0，失败返回错误码
 */
int mqtt_client_set_property(mqtt_client_handle_t handle, uint8_t property_id, const void* value, int value_len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MQTT_CLIENT_API_H */
