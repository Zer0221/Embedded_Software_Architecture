/**
 * @file main.c
 * @brief 应用程序入口点
 *
 * 该文件包含应用程序的入口点，演示如何使用平台化架构
 */

#include <stdio.h>
#include <string.h>
#include "base/platform_api.h"
#include "common/rtos_api.h"
#include "base/i2c_api.h"
#include "base/uart_api.h"
#include "base/spi_api.h"

/* 定义任务栈大小 */
#define TASK_STACK_SIZE         (1024)

/* 定义任务优先级 */
#define TASK_PRIORITY_LOW       RTOS_PRIORITY_LOW
#define TASK_PRIORITY_NORMAL    RTOS_PRIORITY_NORMAL
#define TASK_PRIORITY_HIGH      RTOS_PRIORITY_HIGH

/* 定义消息队列长度 */
#define QUEUE_LENGTH            (10)

/* 定义外设句柄 */
static uart_handle_t g_uart_handle;
static i2c_handle_t g_i2c_handle;
static spi_handle_t g_spi_handle;

/* 定义RTOS对象 */
static rtos_thread_t g_uart_thread;
static rtos_thread_t g_i2c_thread;
static rtos_thread_t g_spi_thread;
static rtos_queue_t g_message_queue;
static rtos_mutex_t g_uart_mutex;

/* 消息结构体定义 */
typedef struct {
    uint8_t source;          /**< 消息来源 */
    uint8_t type;            /**< 消息类型 */
    uint8_t data[32];        /**< 消息数据 */
    uint8_t length;          /**< 数据长度 */
} message_t;

/* 消息来源定义 */
#define MSG_SOURCE_UART     0
#define MSG_SOURCE_I2C      1
#define MSG_SOURCE_SPI      2

/* 消息类型定义 */
#define MSG_TYPE_DATA       0
#define MSG_TYPE_EVENT      1
#define MSG_TYPE_ERROR      2

/* UART接收回调函数 */
static void uart_rx_callback(uint8_t *data, uint32_t size, void *user_data)
{
    message_t msg;
    
    /* 填充消息结构体 */
    msg.source = MSG_SOURCE_UART;
    msg.type = MSG_TYPE_DATA;
    msg.length = (size > sizeof(msg.data)) ? sizeof(msg.data) : size;
    memcpy(msg.data, data, msg.length);
    
    /* 发送消息到队列 */
    rtos_queue_send(g_message_queue, &msg, 0);
}

/* UART处理任务 */
static void uart_task(void *arg)
{
    uart_config_t uart_config;
    int ret;
    uint8_t tx_buffer[64];
    uint8_t count = 0;
    
    /* 配置UART参数 */
    uart_config.channel = UART_CHANNEL_0;
    uart_config.baudrate = UART_BAUDRATE_115200;
    uart_config.data_bits = UART_DATA_BITS_8;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.parity = UART_PARITY_NONE;
    uart_config.flow_control = UART_FLOW_CONTROL_NONE;
    
    /* 初始化UART */
    ret = uart_init(&uart_config, &g_uart_handle);
    if (ret != 0) {
        printf("UART init failed: %d\n", ret);
        return;
    }
    
    /* 注册UART接收回调 */
    uart_register_rx_callback(g_uart_handle, uart_rx_callback, NULL);
    
    /* 主循环 */
    while (1) {
        /* 准备发送数据 */
        snprintf((char *)tx_buffer, sizeof(tx_buffer), "UART Task Count: %d\r\n", count++);
        
        /* 获取互斥锁 */
        rtos_mutex_lock(g_uart_mutex, UINT32_MAX);
        
        /* 发送数据 */
        uart_transmit(g_uart_handle, tx_buffer, strlen((char *)tx_buffer), 100);
        
        /* 释放互斥锁 */
        rtos_mutex_unlock(g_uart_mutex);
        
        /* 延时1秒 */
        rtos_thread_sleep_ms(1000);
    }
}

/* I2C处理任务 */
static void i2c_task(void *arg)
{
    i2c_config_t i2c_config;
    int ret;
    uint8_t reg_addr;
    uint8_t data[2];
    message_t msg;
    
    /* 配置I2C参数 */
    i2c_config.channel = I2C_CHANNEL_0;
    i2c_config.speed = I2C_SPEED_STANDARD;
    i2c_config.addr_10bit = false;
    
    /* 初始化I2C */
    ret = i2c_init(&i2c_config, &g_i2c_handle);
    if (ret != 0) {
        printf("I2C init failed: %d\n", ret);
        return;
    }
    
    /* 主循环 */
    while (1) {
        /* 尝试读取I2C设备（这里以温度传感器为例，假设地址为0x48） */
        reg_addr = 0x00;  /* 温度寄存器地址 */
        
        /* 检查设备是否存在 */
        ret = i2c_is_device_ready(g_i2c_handle, 0x48 << 1, 3, 100);
        if (ret == 0) {
            /* 读取温度数据 */
            ret = i2c_mem_read(g_i2c_handle, 0x48 << 1, reg_addr, 1, data, 2, 100);
            if (ret == 2) {
                /* 填充消息结构体 */
                msg.source = MSG_SOURCE_I2C;
                msg.type = MSG_TYPE_DATA;
                msg.length = 2;
                msg.data[0] = data[0];  /* 温度高字节 */
                msg.data[1] = data[1];  /* 温度低字节 */
                
                /* 发送消息到队列 */
                rtos_queue_send(g_message_queue, &msg, 0);
            }
        }
        
        /* 延时2秒 */
        rtos_thread_sleep_ms(2000);
    }
}

/* SPI处理任务 */
static void spi_task(void *arg)
{
    spi_config_t spi_config;
    int ret;
    uint8_t tx_data[4] = {0xAA, 0x55, 0xAA, 0x55};
    uint8_t rx_data[4] = {0};
    message_t msg;
    
    /* 配置SPI参数 */
    spi_config.channel = SPI_CHANNEL_0;
    spi_config.mode = SPI_MODE_0;
    spi_config.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    spi_config.data_width = SPI_DATA_WIDTH_8BIT;
    spi_config.cs_mode = SPI_CS_MODE_SOFTWARE;
    spi_config.clock_hz = 1000000;  /* 1MHz */
    spi_config.cs_pin = 0;          /* 假设使用GPIO 0作为片选 */
    
    /* 初始化SPI */
    ret = spi_init(&spi_config, &g_spi_handle);
    if (ret != 0) {
        printf("SPI init failed: %d\n", ret);
        return;
    }
    
    /* 主循环 */
    while (1) {
        /* 激活片选信号 */
        spi_cs_control(g_spi_handle, 0);
        
        /* 传输数据 */
        ret = spi_transfer(g_spi_handle, tx_data, rx_data, 4, 100);
        if (ret == 4) {
            /* 填充消息结构体 */
            msg.source = MSG_SOURCE_SPI;
            msg.type = MSG_TYPE_DATA;
            msg.length = 4;
            memcpy(msg.data, rx_data, 4);
            
            /* 发送消息到队列 */
            rtos_queue_send(g_message_queue, &msg, 0);
        }
        
        /* 取消片选信号 */
        spi_cs_control(g_spi_handle, 1);
        
        /* 延时3秒 */
        rtos_thread_sleep_ms(3000);
    }
}

/* 消息处理任务 */
static void message_task(void *arg)
{
    message_t msg;
    int ret;
    
    /* 主循环 */
    while (1) {
        /* 从队列接收消息 */
        ret = rtos_queue_receive(g_message_queue, &msg, UINT32_MAX);
        if (ret == 0) {
            /* 获取互斥锁 */
            rtos_mutex_lock(g_uart_mutex, UINT32_MAX);
            
            /* 根据消息来源和类型处理消息 */
            switch (msg.source) {
                case MSG_SOURCE_UART:
                    printf("UART message: type=%d, length=%d\n", msg.type, msg.length);
                    /* 打印接收到的数据 */
                    printf("Data: ");
                    for (int i = 0; i < msg.length; i++) {
                        printf("%02X ", msg.data[i]);
                    }
                    printf("\n");
                    break;
                    
                case MSG_SOURCE_I2C:
                    printf("I2C message: type=%d, length=%d\n", msg.type, msg.length);
                    if (msg.type == MSG_TYPE_DATA && msg.length == 2) {
                        /* 处理温度数据（假设是12位分辨率） */
                        int16_t temp_raw = (msg.data[0] << 8) | msg.data[1];
                        float temp = temp_raw * 0.0625f;  /* 转换为摄氏度 */
                        printf("Temperature: %.2f°C\n", temp);
                    }
                    break;
                    
                case MSG_SOURCE_SPI:
                    printf("SPI message: type=%d, length=%d\n", msg.type, msg.length);
                    /* 打印接收到的数据 */
                    printf("Data: ");
                    for (int i = 0; i < msg.length; i++) {
                        printf("%02X ", msg.data[i]);
                    }
                    printf("\n");
                    break;
                    
                default:
                    printf("Unknown message source: %d\n", msg.source);
                    break;
            }
            
            /* 释放互斥锁 */
            rtos_mutex_unlock(g_uart_mutex);
        }
    }
}

/**
 * @brief 应用程序入口点
 * 
 * @return int 返回值（未使用）
 */
int main(void)
{
    /* 初始化平台 */
    platform_init();
    
    printf("Platform initialized\n");
    
    /* 初始化RTOS */
    rtos_init();
    
    printf("RTOS initialized\n");
    
    /* 创建消息队列 */
    rtos_queue_create(&g_message_queue, sizeof(message_t), QUEUE_LENGTH);
    
    /* 创建互斥锁 */
    rtos_mutex_create(&g_uart_mutex);
    
    /* 创建任务 */
    rtos_thread_create(&g_uart_thread, "UART_Task", uart_task, NULL, TASK_STACK_SIZE, TASK_PRIORITY_NORMAL);
    rtos_thread_create(&g_i2c_thread, "I2C_Task", i2c_task, NULL, TASK_STACK_SIZE, TASK_PRIORITY_NORMAL);
    rtos_thread_create(&g_spi_thread, "SPI_Task", spi_task, NULL, TASK_STACK_SIZE, TASK_PRIORITY_NORMAL);
    rtos_thread_create(&g_spi_thread, "MSG_Task", message_task, NULL, TASK_STACK_SIZE, TASK_PRIORITY_HIGH);
    
    printf("Tasks created\n");
    
    /* 启动RTOS调度器 */
    printf("Starting scheduler...\n");
    rtos_start_scheduler();
    
    /* 如果执行到这里，说明RTOS启动失败 */
    printf("Scheduler start failed\n");
    
    /* 无限循环 */
    while (1) {
        platform_delay_ms(1000);
    }
    
    return 0;
}
