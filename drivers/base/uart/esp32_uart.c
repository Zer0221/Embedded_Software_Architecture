/**
 * @file esp32_uart.c
 * @brief ESP32平台UART驱动实现
 *
 * 该文件实现了ESP32平台的UART驱动接口
 */

#include "base/uart_api.h"
#include "common/error_api.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

/* 日志标签 */
static const char *TAG = "ESP32_UART";

/* 定义UART缓冲区大�?*/
#define UART_RX_BUF_SIZE 1024
#define UART_TX_BUF_SIZE 1024
#define UART_QUEUE_SIZE  20

/* UART设备句柄结构�?*/
typedef struct {
    uart_port_t port;                  /* ESP32 UART端口�?*/
    QueueHandle_t uart_queue;          /* UART事件队列 */
    uart_config_t config;              /* UART配置参数 */
    bool initialized;                  /* 初始化标�?*/
    uart_rx_callback_t rx_callback;    /* 接收回调函数 */
    void *user_data;                   /* 用户数据 */
    TaskHandle_t rx_task;              /* 接收任务句柄 */
} esp32_uart_handle_t;

/* UART设备句柄数组 */
static esp32_uart_handle_t g_uart_handles[UART_CHANNEL_MAX];

/* UART接收任务函数声明 */
static void uart_rx_task(void *arg);

/**
 * @brief 将抽象数据位转换为ESP32数据�?
 * 
 * @param data_bits 抽象数据�?
 * @return uart_word_length_t ESP32数据�?
 */
static uart_word_length_t convert_data_bits(uart_data_bits_t data_bits)
{
    switch (data_bits) {
        case UART_DATA_BITS_5:
            return UART_DATA_5_BITS;
        case UART_DATA_BITS_6:
            return UART_DATA_6_BITS;
        case UART_DATA_BITS_7:
            return UART_DATA_7_BITS;
        case UART_DATA_BITS_8:
            return UART_DATA_8_BITS;
        default:
            return UART_DATA_8_BITS;
    }
}

/**
 * @brief 将抽象停止位转换为ESP32停止�?
 * 
 * @param stop_bits 抽象停止�?
 * @return uart_stop_bits_t ESP32停止�?
 */
static uart_stop_bits_t convert_stop_bits(uart_stop_bits_t stop_bits)
{
    switch (stop_bits) {
        case UART_STOP_BITS_1:
            return UART_STOP_BITS_1;
        case UART_STOP_BITS_1_5:
            return UART_STOP_BITS_1_5;
        case UART_STOP_BITS_2:
            return UART_STOP_BITS_2;
        default:
            return UART_STOP_BITS_1;
    }
}

/**
 * @brief 将抽象校验位转换为ESP32校验�?
 * 
 * @param parity 抽象校验�?
 * @return uart_parity_t ESP32校验�?
 */
static uart_parity_t convert_parity(uart_parity_t parity)
{
    switch (parity) {
        case UART_PARITY_NONE:
            return UART_PARITY_DISABLE;
        case UART_PARITY_ODD:
            return UART_PARITY_ODD;
        case UART_PARITY_EVEN:
            return UART_PARITY_EVEN;
        default:
            return UART_PARITY_DISABLE;
    }
}

/**
 * @brief 将抽象流控转换为ESP32流控
 * 
 * @param flow_control 抽象流控
 * @return uart_hw_flowcontrol_t ESP32流控
 */
static uart_hw_flowcontrol_t convert_flow_control(uart_flow_control_t flow_control)
{
    switch (flow_control) {
        case UART_FLOW_CONTROL_NONE:
            return UART_HW_FLOWCTRL_DISABLE;
        case UART_FLOW_CONTROL_RTS:
            return UART_HW_FLOWCTRL_RTS;
        case UART_FLOW_CONTROL_CTS:
            return UART_HW_FLOWCTRL_CTS;
        case UART_FLOW_CONTROL_RTS_CTS:
            return UART_HW_FLOWCTRL_CTS_RTS;
        default:
            return UART_HW_FLOWCTRL_DISABLE;
    }
}

/**
 * @brief 获取波特�?
 * 
 * @param config UART配置参数
 * @return uint32_t 波特�?
 */
static uint32_t get_baudrate(const uart_config_t *config)
{
    if (config->baudrate == UART_BAUDRATE_CUSTOM) {
        return config->custom_baudrate;
    } else {
        return (uint32_t)config->baudrate;
    }
}

/**
 * @brief UART接收任务
 * 
 * @param arg 任务参数
 */
static void uart_rx_task(void *arg)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)arg;
    uart_event_t event;
    uint8_t *data = NULL;
    
    /* 分配接收缓冲�?*/
    data = (uint8_t *)malloc(UART_RX_BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate UART RX buffer");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        /* 等待UART事件 */
        if (xQueueReceive(esp32_handle->uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    /* 读取数据 */
                    if (esp32_handle->rx_callback != NULL) {
                        size_t buffered_size;
                        int len = uart_read_bytes(esp32_handle->port, data, 
                                                event.size, pdMS_TO_TICKS(100));
                        
                        if (len > 0) {
                            /* 调用用户回调函数 */
                            esp32_handle->rx_callback(data, len, esp32_handle->user_data);
                        }
                        
                        /* 检查是否还有数�?*/
                        uart_get_buffered_data_len(esp32_handle->port, &buffered_size);
                        if (buffered_size > 0) {
                            /* 手动触发UART_DATA事件 */
                            xQueueSend(esp32_handle->uart_queue, (void *)&event, 0);
                        }
                    }
                    break;
                    
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(esp32_handle->port);
                    xQueueReset(esp32_handle->uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART buffer full");
                    uart_flush_input(esp32_handle->port);
                    xQueueReset(esp32_handle->uart_queue);
                    break;
                    
                case UART_BREAK:
                    ESP_LOGW(TAG, "UART break");
                    break;
                    
                case UART_PARITY_ERR:
                    ESP_LOGW(TAG, "UART parity error");
                    break;
                    
                case UART_FRAME_ERR:
                    ESP_LOGW(TAG, "UART frame error");
                    break;
                    
                default:
                    ESP_LOGW(TAG, "UART event: %d", event.type);
                    break;
            }
        }
    }
    
    /* 释放缓冲�?*/
    free(data);
    
    /* 删除任务 */
    vTaskDelete(NULL);
}

/**
 * @brief 初始化UART
 * 
 * @param config UART配置参数
 * @param handle UART设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int uart_init(const uart_config_t *config, uart_handle_t *handle)
{
    esp32_uart_handle_t *esp32_handle;
    uart_config_t esp32_uart_config;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否有效 */
    if (config->channel >= UART_CHANNEL_MAX) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查通道是否已初始化 */
    esp32_handle = &g_uart_handles[config->channel];
    if (esp32_handle->initialized) {
        return ERROR_BUSY;
    }
    
    /* 配置ESP32 UART参数 */
    memset(&esp32_uart_config, 0, sizeof(esp32_uart_config));
    
    esp32_uart_config.baud_rate = get_baudrate(config);
    esp32_uart_config.data_bits = convert_data_bits(config->data_bits);
    esp32_uart_config.parity = convert_parity(config->parity);
    esp32_uart_config.stop_bits = convert_stop_bits(config->stop_bits);
    esp32_uart_config.flow_ctrl = convert_flow_control(config->flow_control);
    esp32_uart_config.rx_flow_ctrl_thresh = 122;  /* 默认流控阈�?*/
    esp32_uart_config.source_clk = UART_SCLK_APB;
    
    /* 安装UART驱动 */
    ret = uart_driver_install(config->channel, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 
                            UART_QUEUE_SIZE, &esp32_handle->uart_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 配置UART参数 */
    ret = uart_param_config(config->channel, &esp32_uart_config);
    if (ret != ESP_OK) {
        uart_driver_delete(config->channel);
        ESP_LOGE(TAG, "UART param config failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 根据不同UART通道设置默认引脚 */
    switch (config->channel) {
        case UART_CHANNEL_0:
            ret = uart_set_pin(config->channel, 1, 3, -1, -1);  /* GPIO1: TX, GPIO3: RX */
            break;
        case UART_CHANNEL_1:
            ret = uart_set_pin(config->channel, 10, 9, -1, -1); /* GPIO10: TX, GPIO9: RX */
            break;
        case UART_CHANNEL_2:
            ret = uart_set_pin(config->channel, 17, 16, -1, -1); /* GPIO17: TX, GPIO16: RX */
            break;
        default:
            ret = ESP_FAIL;
            break;
    }
    
    if (ret != ESP_OK) {
        uart_driver_delete(config->channel);
        ESP_LOGE(TAG, "UART set pin failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 创建接收任务 */
    char task_name[32];
    snprintf(task_name, sizeof(task_name), "uart_rx_task_%d", config->channel);
    
    /* 更新句柄信息 */
    esp32_handle->port = config->channel;
    esp32_handle->initialized = true;
    esp32_handle->rx_callback = NULL;
    esp32_handle->user_data = NULL;
    
    /* 创建接收任务 */
    BaseType_t task_created = xTaskCreate(uart_rx_task, task_name, 2048, 
                                         esp32_handle, 10, &esp32_handle->rx_task);
    if (task_created != pdPASS) {
        uart_driver_delete(config->channel);
        ESP_LOGE(TAG, "Failed to create UART RX task");
        esp32_handle->initialized = false;
        return ERROR_NO_MEMORY;
    }
    
    /* 返回句柄 */
    *handle = (uart_handle_t)esp32_handle;
    
    return ERROR_NONE;
}

/**
 * @brief 去初始化UART
 * 
 * @param handle UART设备句柄
 * @return int 0表示成功，非0表示失败
 */
int uart_deinit(uart_handle_t handle)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 删除接收任务 */
    if (esp32_handle->rx_task != NULL) {
        vTaskDelete(esp32_handle->rx_task);
        esp32_handle->rx_task = NULL;
    }
    
    /* 卸载UART驱动 */
    ret = uart_driver_delete(esp32_handle->port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver delete failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    /* 清除句柄信息 */
    esp32_handle->initialized = false;
    esp32_handle->rx_callback = NULL;
    esp32_handle->user_data = NULL;
    
    return ERROR_NONE;
}

/**
 * @brief UART发送数�?
 * 
 * @param handle UART设备句柄
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功发送的字节数，负值表示错�?
 */
int uart_transmit(uart_handle_t handle, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    int sent_len;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 发送数�?*/
    sent_len = uart_write_bytes(esp32_handle->port, (const char *)data, len);
    if (sent_len < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return ERROR_HARDWARE;
    }
    
    /* 等待发送完�?*/
    if (uart_wait_tx_done(esp32_handle->port, pdMS_TO_TICKS(timeout_ms)) != ESP_OK) {
        ESP_LOGE(TAG, "UART TX timeout");
        return ERROR_TIMEOUT;
    }
    
    return sent_len;
}

/**
 * @brief UART接收数据
 * 
 * @param handle UART设备句柄
 * @param data 数据缓冲�?
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功接收的字节数，负值表示错�?
 */
int uart_receive(uart_handle_t handle, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    int recv_len;
    
    /* 参数检�?*/
    if (handle == NULL || data == NULL || len == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 接收数据 */
    recv_len = uart_read_bytes(esp32_handle->port, data, len, pdMS_TO_TICKS(timeout_ms));
    if (recv_len < 0) {
        ESP_LOGE(TAG, "UART read failed");
        return ERROR_HARDWARE;
    }
    
    return recv_len;
}

/**
 * @brief 注册UART接收回调函数（中断模式）
 * 
 * @param handle UART设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return int 0表示成功，非0表示失败
 */
int uart_register_rx_callback(uart_handle_t handle, uart_rx_callback_t callback, void *user_data)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新回调函数 */
    esp32_handle->rx_callback = callback;
    esp32_handle->user_data = user_data;
    
    return ERROR_NONE;
}

/**
 * @brief 获取UART接收缓冲区中可读取的数据长度
 * 
 * @param handle UART设备句柄
 * @return int 可读取的数据长度，负值表示错�?
 */
int uart_get_rx_data_size(uart_handle_t handle)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    size_t buffered_size = 0;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取缓冲区大�?*/
    ret = uart_get_buffered_data_len(esp32_handle->port, &buffered_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Get buffered data length failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return (int)buffered_size;
}

/**
 * @brief 清空UART接收缓冲�?
 * 
 * @param handle UART设备句柄
 * @return int 0表示成功，非0表示失败
 */
int uart_flush_rx_buffer(uart_handle_t handle)
{
    esp32_uart_handle_t *esp32_handle = (esp32_uart_handle_t *)handle;
    esp_err_t ret;
    
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查句柄是否有�?*/
    if (!esp32_handle->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 清空接收缓冲�?*/
    ret = uart_flush_input(esp32_handle->port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Flush input failed: %d", ret);
        return ERROR_HARDWARE;
    }
    
    return ERROR_NONE;
}

