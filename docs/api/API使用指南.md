# 嵌入式软件架构API使用指南

## 目录

1. [介绍](#1-介绍)
2. [平台接口](#2-平台接口)
3. [RTOS接口](#3-rtos接口)
4. [GPIO接口](#4-gpio接口)
5. [UART接口](#5-uart接口)
6. [SPI接口](#6-spi接口)
7. [I2C接口](#7-i2c接口)
8. [电源管理接口](#8-电源管理接口)
9. [错误处理](#9-错误处理)
10. [示例应用](#10-示例应用)

## 1. 介绍

本文档提供了嵌入式软件架构API的详细使用指南。该架构采用平台化、分层设计，通过统一的接口规范，实现底层硬件与上层应用的解耦，使代码易于复用和跨平台移植。

### 1.1 如何使用本指南

本指南按照API类别进行组织，每个章节介绍一类API的使用方法，包括：
- 接口定义
- 使用示例
- 注意事项
- 常见问题

建议结合源代码一起阅读本指南，以便更好地理解API的使用方法。

### 1.2 约定和标记

- 所有API函数返回整数类型的错误码，0表示成功，负值表示失败
- 句柄类型（例如`gpio_handle_t`）是不透明的指针，用于标识特定的资源
- 配置结构体用于初始化各种资源

## 2. 平台接口

平台接口提供了与硬件平台相关的基本功能，如初始化、延时和获取系统信息等。

### 2.1 接口定义

```c
// 平台初始化
int platform_init(void);

// 平台去初始化
int platform_deinit(void);

// 获取平台信息
int platform_get_info(void *info);

// 毫秒级延时
void platform_delay_ms(uint32_t ms);

// 微秒级延时
void platform_delay_us(uint32_t us);

// 获取系统运行时间（毫秒）
uint32_t platform_get_time_ms(void);
```

### 2.2 使用示例

```c
#include "platform_api.h"

// 示例：平台初始化和延时
void platform_example(void)
{
    // 初始化平台
    platform_init();
    
    // 延时100毫秒
    platform_delay_ms(100);
    
    // 获取系统运行时间
    uint32_t time = platform_get_time_ms();
    
    // 获取平台信息
    platform_info_t info;
    platform_get_info(&info);
    
    // 去初始化平台
    platform_deinit();
}
```

### 2.3 注意事项

- `platform_init()`必须在使用任何其他功能前调用
- 平台信息结构体的具体类型依赖于当前平台，使用前需要进行类型转换
- 微秒级延时在某些平台上可能不精确，特别是在低频时钟下

## 3. RTOS接口

RTOS接口提供了操作系统相关的功能，如任务管理、同步机制和定时器等。

### 3.1 接口定义

```c
// RTOS初始化
int rtos_init(void);

// 启动RTOS调度器
int rtos_start_scheduler(void);

// 创建线程
int rtos_thread_create(rtos_thread_t *thread, const char *name, 
                      rtos_thread_func_t func, void *arg, 
                      uint32_t stack_size, rtos_priority_t priority);

// 线程睡眠
void rtos_thread_sleep_ms(uint32_t ms);

// 创建信号量
int rtos_sem_create(rtos_sem_t *sem, uint32_t initial_count, uint32_t max_count);

// 获取信号量
int rtos_sem_take(rtos_sem_t sem, uint32_t timeout_ms);

// 释放信号量
int rtos_sem_give(rtos_sem_t sem);

// 创建互斥锁
int rtos_mutex_create(rtos_mutex_t *mutex);

// 获取互斥锁
int rtos_mutex_lock(rtos_mutex_t mutex, uint32_t timeout_ms);

// 释放互斥锁
int rtos_mutex_unlock(rtos_mutex_t mutex);

// 创建消息队列
int rtos_queue_create(rtos_queue_t *queue, uint32_t item_size, uint32_t item_count);

// 发送消息
int rtos_queue_send(rtos_queue_t queue, const void *item, uint32_t timeout_ms);

// 接收消息
int rtos_queue_receive(rtos_queue_t queue, void *item, uint32_t timeout_ms);
```

### 3.2 使用示例

```c
#include "rtos_api.h"

// 线程函数
void test_thread(void *arg)
{
    while (1) {
        // 线程睡眠100毫秒
        rtos_thread_sleep_ms(100);
    }
}

// 示例：创建线程和使用队列
void rtos_example(void)
{
    rtos_thread_t thread;
    rtos_queue_t queue;
    int data = 123;
    int received;
    
    // 初始化RTOS
    rtos_init();
    
    // 创建线程
    rtos_thread_create(&thread, "TestThread", test_thread, NULL, 
                      1024, RTOS_PRIORITY_NORMAL);
    
    // 创建消息队列
    rtos_queue_create(&queue, sizeof(int), 10);
    
    // 发送消息
    rtos_queue_send(queue, &data, 0);
    
    // 接收消息
    rtos_queue_receive(queue, &received, 100);
    
    // 启动调度器（此函数不会返回）
    rtos_start_scheduler();
}
```

### 3.3 注意事项

- `rtos_start_scheduler()`会启动调度器并永远不会返回，因此应该在所有初始化完成后调用
- 线程栈大小单位为字节，应根据线程需求合理设置
- 队列项大小必须为固定值，队列创建后不能更改

## 4. GPIO接口

GPIO接口提供了通用输入/输出引脚的操作功能，如配置、读写和中断处理等。

### 4.1 接口定义

```c
// GPIO初始化
int gpio_init(const gpio_config_t *config, gpio_handle_t *handle);

// GPIO去初始化
int gpio_deinit(gpio_handle_t handle);

// 写GPIO状态
int gpio_write(gpio_handle_t handle, gpio_state_t state);

// 读GPIO状态
gpio_state_t gpio_read(gpio_handle_t handle);

// 翻转GPIO状态
int gpio_toggle(gpio_handle_t handle);

// 注册中断回调函数
int gpio_register_irq_callback(gpio_handle_t handle, 
                              gpio_irq_callback_t callback, 
                              void *user_data);

// 使能中断
int gpio_enable_irq(gpio_handle_t handle);

// 禁用中断
int gpio_disable_irq(gpio_handle_t handle);
```

### 4.2 使用示例

```c
#include "gpio_api.h"

// GPIO中断回调函数
void gpio_irq_handler(gpio_port_t port, gpio_pin_t pin, void *user_data)
{
    // 处理GPIO中断
}

// 示例：GPIO配置和使用
void gpio_example(void)
{
    gpio_handle_t led_handle, button_handle;
    
    // 配置LED引脚为输出
    gpio_config_t led_config = {
        .port = GPIO_PORT_A,
        .pin = GPIO_PIN_5,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_LOW
    };
    
    // 初始化LED GPIO
    gpio_init(&led_config, &led_handle);
    
    // 设置LED状态为高电平
    gpio_write(led_handle, GPIO_PIN_SET);
    
    // 配置按钮引脚为中断输入
    gpio_config_t button_config = {
        .port = GPIO_PORT_C,
        .pin = GPIO_PIN_13,
        .mode = GPIO_MODE_IT_FALLING,
        .pull = GPIO_PULL_UP
    };
    
    // 初始化按钮GPIO
    gpio_init(&button_config, &button_handle);
    
    // 注册中断回调函数
    gpio_register_irq_callback(button_handle, gpio_irq_handler, NULL);
    
    // 使能中断
    gpio_enable_irq(button_handle);
    
    // 读取按钮状态
    gpio_state_t button_state = gpio_read(button_handle);
}
```

### 4.3 注意事项

- GPIO配置参数因平台而异，但接口保持一致
- 中断回调函数在中断上下文中执行，应尽量简短，避免长时间阻塞
- 使用`gpio_toggle()`比读取-修改-写入序列更高效

## 5. UART接口

UART接口提供了串行通信功能，用于设备间的数据传输。

### 5.1 接口定义

```c
// UART初始化
int uart_init(const uart_config_t *config, uart_handle_t *handle);

// UART去初始化
int uart_deinit(uart_handle_t handle);

// 发送数据
int uart_transmit(uart_handle_t handle, const uint8_t *data, 
                 uint32_t size, uint32_t timeout_ms);

// 接收数据
int uart_receive(uart_handle_t handle, uint8_t *data, 
                uint32_t size, uint32_t timeout_ms);

// 注册回调函数
int uart_register_callback(uart_handle_t handle, 
                          uart_callback_t callback, 
                          void *user_data);

// 启动异步接收
int uart_receive_it(uart_handle_t handle, uint8_t *data, uint32_t size);
```

### 5.2 使用示例

```c
#include "uart_api.h"

// UART回调函数
void uart_callback(uart_handle_t handle, uart_event_t event, void *user_data)
{
    if (event == UART_EVENT_RX_COMPLETE) {
        // 接收完成处理
    }
}

// 示例：UART配置和使用
void uart_example(void)
{
    uart_handle_t uart_handle;
    uint8_t tx_buffer[64] = "Hello, World!\r\n";
    uint8_t rx_buffer[64];
    
    // 配置UART
    uart_config_t uart_config = {
        .channel = UART_CHANNEL_1,
        .baudrate = 115200,
        .data_bits = UART_DATA_BITS_8,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .mode = UART_MODE_TX_RX,
        .flow_ctrl = UART_FLOW_CTRL_NONE
    };
    
    // 初始化UART
    uart_init(&uart_config, &uart_handle);
    
    // 注册回调函数
    uart_register_callback(uart_handle, uart_callback, NULL);
    
    // 发送数据（阻塞方式）
    uart_transmit(uart_handle, tx_buffer, strlen((char *)tx_buffer), 1000);
    
    // 接收数据（阻塞方式）
    uart_receive(uart_handle, rx_buffer, sizeof(rx_buffer), 1000);
    
    // 启动异步接收
    uart_receive_it(uart_handle, rx_buffer, sizeof(rx_buffer));
}
```

### 5.3 注意事项

- 异步接收需要注册回调函数
- 超时设置为0表示非阻塞操作，立即返回
- 接收缓冲区在异步接收完成前必须保持有效

## 6. SPI接口

SPI接口提供了串行外设接口通信功能，用于与传感器、存储器等设备通信。

### 6.1 接口定义

```c
// SPI初始化
int spi_init(const spi_config_t *config, spi_handle_t *handle);

// SPI去初始化
int spi_deinit(spi_handle_t handle);

// SPI传输数据
int spi_transfer(spi_handle_t handle, const void *tx_data, 
                void *rx_data, uint32_t len, uint32_t timeout_ms);

// 注册事件回调函数
int spi_register_event_callback(spi_handle_t handle, 
                               spi_event_callback_t callback, 
                               void *user_data);

// 异步传输
int spi_transfer_async(spi_handle_t handle, const void *tx_data, 
                      void *rx_data, uint32_t len);

// 设置片选状态
int spi_set_cs(spi_handle_t handle, int state);
```

### 6.2 使用示例

```c
#include "spi_api.h"

// SPI事件回调函数
void spi_event_handler(spi_handle_t handle, spi_event_t event, void *user_data)
{
    if (event == SPI_EVENT_TRANSFER_COMPLETE) {
        // 传输完成处理
    }
}

// 示例：SPI配置和使用
void spi_example(void)
{
    spi_handle_t spi_handle;
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4];
    
    // 配置SPI
    spi_config_t spi_config = {
        .channel = SPI_CHANNEL_1,
        .mode = SPI_MODE_MASTER,
        .format = SPI_FORMAT_CPOL0_CPHA0,
        .data_bits = SPI_DATA_8BIT,
        .bit_order = SPI_MSB_FIRST,
        .speed = SPI_SPEED_HIGH,
        .nss = SPI_NSS_SOFT
    };
    
    // 初始化SPI
    spi_init(&spi_config, &spi_handle);
    
    // 注册回调函数
    spi_register_event_callback(spi_handle, spi_event_handler, NULL);
    
    // 设置片选为低电平（有效）
    spi_set_cs(spi_handle, 0);
    
    // 传输数据（阻塞方式）
    spi_transfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer), 1000);
    
    // 设置片选为高电平（无效）
    spi_set_cs(spi_handle, 1);
    
    // 异步传输
    spi_transfer_async(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
}
```

### 6.3 注意事项

- 对于软件片选模式，需要手动控制片选信号
- 异步传输需要注册回调函数
- 传输缓冲区在异步传输完成前必须保持有效

## 7. I2C接口

I2C接口提供了双线串行通信功能，用于与各种I2C设备通信。

### 7.1 接口定义

```c
// I2C初始化
int i2c_init(const i2c_config_t *config, i2c_handle_t *handle);

// I2C去初始化
int i2c_deinit(i2c_handle_t handle);

// 主机发送数据
int i2c_master_transmit(i2c_handle_t handle, uint16_t dev_addr, 
                       const uint8_t *data, uint32_t len, 
                       uint32_t flags, uint32_t timeout_ms);

// 主机接收数据
int i2c_master_receive(i2c_handle_t handle, uint16_t dev_addr, 
                      uint8_t *data, uint32_t len, 
                      uint32_t flags, uint32_t timeout_ms);

// 内存写入
int i2c_mem_write(i2c_handle_t handle, uint16_t dev_addr, 
                 uint16_t mem_addr, uint8_t mem_addr_size, 
                 const uint8_t *data, uint32_t len, 
                 uint32_t timeout_ms);

// 内存读取
int i2c_mem_read(i2c_handle_t handle, uint16_t dev_addr, 
                uint16_t mem_addr, uint8_t mem_addr_size, 
                uint8_t *data, uint32_t len, 
                uint32_t timeout_ms);

// 检测设备
int i2c_is_device_ready(i2c_handle_t handle, uint16_t dev_addr, 
                       uint32_t retries, uint32_t timeout_ms);
```

### 7.2 使用示例

```c
#include "i2c_api.h"

// 示例：I2C配置和使用
void i2c_example(void)
{
    i2c_handle_t i2c_handle;
    uint8_t tx_buffer[2] = {0x01, 0x02};
    uint8_t rx_buffer[2];
    uint8_t device_addr = 0x3C; // 设备地址
    
    // 配置I2C
    i2c_config_t i2c_config = {
        .channel = I2C_CHANNEL_1,
        .speed = I2C_SPEED_STANDARD,
        .addr_10bit = false
    };
    
    // 初始化I2C
    i2c_init(&i2c_config, &i2c_handle);
    
    // 检测设备是否就绪
    if (i2c_is_device_ready(i2c_handle, device_addr, 3, 1000) == 0) {
        // 设备就绪
        
        // 发送数据
        i2c_master_transmit(i2c_handle, device_addr, tx_buffer, 
                           sizeof(tx_buffer), I2C_FLAG_STOP, 1000);
        
        // 接收数据
        i2c_master_receive(i2c_handle, device_addr, rx_buffer, 
                          sizeof(rx_buffer), I2C_FLAG_STOP, 1000);
        
        // 写入设备内存
        i2c_mem_write(i2c_handle, device_addr, 0x10, 1, 
                     tx_buffer, sizeof(tx_buffer), 1000);
        
        // 读取设备内存
        i2c_mem_read(i2c_handle, device_addr, 0x10, 1, 
                    rx_buffer, sizeof(rx_buffer), 1000);
    }
}
```

### 7.3 注意事项

- I2C设备地址应该是7位或10位地址，不包括读/写位
- 对于某些平台，可能需要外部上拉电阻
- 内存地址大小通常为1或2字节，取决于设备类型

## 8. 电源管理接口

电源管理接口提供了低功耗模式控制、电源状态监控和唤醒源管理等功能。

### 8.1 接口定义

```c
// 电源管理初始化
int power_init(void);

// 进入低功耗模式
int power_enter_sleep_mode(power_sleep_mode_t mode);

// 配置唤醒源
int power_config_wakeup_source(power_wakeup_source_t source, bool enable);

// 获取重置原因
power_reset_cause_t power_get_reset_cause(void);

// 获取电源状态
int power_get_supply_voltage(uint32_t *voltage_mv);

// 注册电源事件回调
int power_register_event_callback(power_event_callback_t callback, void *user_data);
```

### 8.2 使用示例

```c
#include "power_api.h"

// 电源事件回调函数
void power_event_handler(power_event_t event, void *user_data)
{
    if (event == POWER_EVENT_SUPPLY_WARNING) {
        // 电源电压过低警告处理
    }
}

// 示例：电源管理使用
void power_example(void)
{
    uint32_t voltage;
    
    // 初始化电源管理
    power_init();
    
    // 注册事件回调
    power_register_event_callback(power_event_handler, NULL);
    
    // 获取供电电压
    power_get_supply_voltage(&voltage);
    
    // 获取重置原因
    power_reset_cause_t reset_cause = power_get_reset_cause();
    
    // 配置RTC作为唤醒源
    power_config_wakeup_source(POWER_WAKEUP_RTC, true);
    
    // 配置外部中断作为唤醒源
    power_config_wakeup_source(POWER_WAKEUP_PIN, true);
    
    // 进入停止模式
    power_enter_sleep_mode(POWER_SLEEP_STOP);
    
    // 唤醒后继续执行
}
```

### 8.3 注意事项

- 不同平台支持的低功耗模式和唤醒源可能不同
- 进入低功耗模式前应确保所有外设已正确配置
- 某些低功耗模式可能会导致某些外设停止工作

## 9. 错误处理

错误处理接口提供了统一的错误码定义、回调机制和日志输出功能。

### 9.1 接口定义

```c
// 设置错误回调函数
void error_set_callback(error_callback_t callback, void *user_data);

// 获取错误描述
const char *error_get_string(int error_code);

// 报告错误
void error_report(int error_code, const char *file, int line, const char *func);

// 检查错误并处理
int error_check(int error_code, const char *file, int line, const char *func);
```

### 9.2 使用示例

```c
#include "error_api.h"

// 错误回调函数
void error_handler(int error_code, const char *file, int line, 
                  const char *func, void *user_data)
{
    // 打印错误信息
    printf("Error %d (%s) occurred in %s at %s:%d\n",
           error_code, error_get_string(error_code),
           func, file, line);
    
    // 根据错误码处理
    if (error_code == ERROR_HARDWARE) {
        // 硬件错误处理
    }
}

// 示例：错误处理使用
void error_example(void)
{
    int result;
    
    // 设置错误回调
    error_set_callback(error_handler, NULL);
    
    // 调用API函数并检查结果
    result = some_api_function();
    if (ERROR_CHECK(result) != ERROR_NONE) {
        // 错误已由错误回调处理
        return;
    }
    
    // 手动报告错误
    if (some_condition) {
        ERROR_REPORT(ERROR_INVALID_STATE);
        return;
    }
    
    // 继续正常执行
}
```

### 9.3 注意事项

- `ERROR_CHECK`和`ERROR_REPORT`是宏，会自动添加文件名、行号和函数名
- 错误回调函数在错误发生的上下文中执行，可能是中断上下文
- 应避免在错误回调中执行耗时操作

## 10. 示例应用

以下是一个完整的示例应用，演示了如何使用架构中的各种API：

```c
/**
 * @file main.c
 * @brief 示例应用程序
 */

#include "platform_api.h"
#include "rtos_api.h"
#include "gpio_api.h"
#include "uart_api.h"
#include "error_api.h"

/* 全局变量 */
static uart_handle_t g_uart_handle;
static gpio_handle_t g_led_handle;
static rtos_sem_t g_uart_sem;
static uint8_t g_rx_buffer[64];
static uint8_t g_tx_buffer[64];

/* UART回调函数 */
static void uart_callback(uart_handle_t handle, uart_event_t event, void *user_data)
{
    if (event == UART_EVENT_RX_COMPLETE) {
        // 接收完成，发出信号量
        rtos_sem_give(g_uart_sem);
    }
}

/* LED闪烁任务 */
static void led_task(void *arg)
{
    while (1) {
        // 翻转LED状态
        gpio_toggle(g_led_handle);
        
        // 延时500ms
        rtos_thread_sleep_ms(500);
    }
}

/* UART处理任务 */
static void uart_task(void *arg)
{
    while (1) {
        // 启动异步接收
        uart_receive_it(g_uart_handle, g_rx_buffer, sizeof(g_rx_buffer));
        
        // 等待接收完成
        rtos_sem_take(g_uart_sem, UINT32_MAX);
        
        // 处理接收到的数据
        sprintf((char *)g_tx_buffer, "Received: %s\r\n", g_rx_buffer);
        
        // 发送响应
        uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 1000);
    }
}

/* 错误处理回调 */
static void error_handler(int error_code, const char *file, int line, 
                         const char *func, void *user_data)
{
    // 打印错误信息
    printf("Error %d (%s) occurred in %s at %s:%d\n",
           error_code, error_get_string(error_code),
           func, file, line);
    
    // 对于严重错误，重置系统
    if (error_code <= ERROR_CRITICAL) {
        // 系统重置
        platform_reset();
    }
}

/* 主函数 */
int main(void)
{
    rtos_thread_t led_thread, uart_thread;
    
    // 初始化平台
    platform_init();
    
    // 设置错误回调
    error_set_callback(error_handler, NULL);
    
    // 初始化RTOS
    rtos_init();
    
    // 创建信号量
    rtos_sem_create(&g_uart_sem, 0, 1);
    
    // 配置LED GPIO
    gpio_config_t led_config = {
        .port = GPIO_PORT_A,
        .pin = GPIO_PIN_5,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_LOW
    };
    
    // 初始化LED GPIO
    if (ERROR_CHECK(gpio_init(&led_config, &g_led_handle)) != ERROR_NONE) {
        return -1;
    }
    
    // 配置UART
    uart_config_t uart_config = {
        .channel = UART_CHANNEL_1,
        .baudrate = 115200,
        .data_bits = UART_DATA_BITS_8,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .mode = UART_MODE_TX_RX,
        .flow_ctrl = UART_FLOW_CTRL_NONE
    };
    
    // 初始化UART
    if (ERROR_CHECK(uart_init(&uart_config, &g_uart_handle)) != ERROR_NONE) {
        return -1;
    }
    
    // 注册UART回调函数
    uart_register_callback(g_uart_handle, uart_callback, NULL);
    
    // 创建LED任务
    rtos_thread_create(&led_thread, "LED_Task", led_task, NULL, 
                      512, RTOS_PRIORITY_LOW);
    
    // 创建UART任务
    rtos_thread_create(&uart_thread, "UART_Task", uart_task, NULL, 
                      1024, RTOS_PRIORITY_NORMAL);
    
    // 发送欢迎消息
    sprintf((char *)g_tx_buffer, "Welcome to the example application!\r\n");
    uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 1000);
    
    // 启动RTOS调度器
    rtos_start_scheduler();
    
    // 永远不会执行到这里
    return 0;
}
```

### 10.1 构建和运行

1. 确保在`config.h`中正确配置了目标平台和RTOS
2. 使用合适的工具链编译项目
3. 将程序下载到目标硬件
4. 通过串口监视器观察输出

### 10.2 功能说明

此示例应用程序演示了以下功能：

- 平台初始化和配置
- RTOS任务创建和调度
- GPIO控制（LED闪烁）
- UART通信（回显接收到的数据）
- 错误处理和报告

您可以基于此示例，开发更复杂的应用程序，添加更多功能模块。
