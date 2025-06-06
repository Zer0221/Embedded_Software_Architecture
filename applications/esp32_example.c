/**
 * @file esp32_example.c
 * @brief ESP32平台示例应用程序
 *
 * 该文件展示了如何使用平台化设计的软件架构在ESP32上开发应用
 */

#include "base/platform_api.h"
#include "common/rtos_api.h"
#include "base/gpio_api.h"
#include "base/i2c_api.h"
#include "base/uart_api.h"
#include "base/spi_api.h"
#include "common/error_api.h"
#include <stdio.h>
#include <string.h>

/* 任务优先级定义 */
#define TASK_PRIORITY_LED        RTOS_PRIORITY_LOW
#define TASK_PRIORITY_SENSOR     RTOS_PRIORITY_NORMAL
#define TASK_PRIORITY_COMM       RTOS_PRIORITY_HIGH

/* 任务栈大小定义 */
#define STACK_SIZE_LED           2048
#define STACK_SIZE_SENSOR        4096
#define STACK_SIZE_COMM          4096

/* LED相关定义 */
#define LED_PIN                  2  /* ESP32开发板上的LED引脚 */
#define LED_BLINK_PERIOD_MS      500

/* I2C传感器相关定义 */
#define I2C_CHANNEL             I2C_CHANNEL_0
#define I2C_SPEED               I2C_SPEED_STANDARD
#define I2C_SENSOR_ADDR         0x68  /* 假设使用MPU6050传感器 */
#define I2C_WHO_AM_I_REG        0x75  /* MPU6050的WHO_AM_I寄存器 */
#define I2C_EXPECTED_ID         0x68  /* MPU6050的ID */

/* UART通信相关定义 */
#define UART_CHANNEL            UART_CHANNEL_0
#define UART_BAUDRATE           UART_BAUDRATE_115200

/* SPI显示屏相关定义 */
#define SPI_CHANNEL             SPI_CHANNEL_0
#define SPI_SPEED               1000000  /* 1MHz */
#define SPI_CS_PIN              5

/* 全局变量定义 */
static rtos_thread_t led_task_handle;
static rtos_thread_t sensor_task_handle;
static rtos_thread_t comm_task_handle;
static rtos_mutex_t data_mutex;
static gpio_handle_t led_handle;
static i2c_handle_t i2c_handle;
static uart_handle_t uart_handle;
static spi_handle_t spi_handle;

/* 传感器数据结构体 */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    float temperature;
} sensor_data_t;

/* 传感器数据共享变量 */
static sensor_data_t g_sensor_data;

/**
 * @brief UART接收回调函数
 */
static void uart_rx_callback(uint8_t *data, uint32_t size, void *user_data)
{
    /* 简单地回显接收到的数据 */
    uart_transmit(uart_handle, data, size, 100);
}

/**
 * @brief LED控制任务
 */
static void led_task(void *arg)
{
    int state = 0;
    
    while (1) {
        /* 切换LED状态 */
        state = !state;
        gpio_write(led_handle, state);
        
        /* 延时 */
        rtos_thread_sleep_ms(LED_BLINK_PERIOD_MS);
    }
}

/**
 * @brief 初始化MPU6050传感器
 */
static int init_mpu6050(void)
{
    uint8_t id;
    int result;
    
    /* 读取WHO_AM_I寄存器，确认传感器ID */
    result = i2c_mem_read(i2c_handle, I2C_SENSOR_ADDR, I2C_WHO_AM_I_REG, 1, &id, 1, 100);
    if (result < 0) {
        printf("Failed to read sensor ID, error: %d\n", result);
        return ERROR_HARDWARE;
    }
    
    if (id != I2C_EXPECTED_ID) {
        printf("Unexpected sensor ID: 0x%02X, expected: 0x%02X\n", id, I2C_EXPECTED_ID);
        return ERROR_HARDWARE;
    }
    
    /* 初始化MPU6050，这里简化处理，实际应用需要更多初始化步骤 */
    uint8_t pwr_mgmt_1 = 0x00;  /* 唤醒传感器 */
    result = i2c_mem_write(i2c_handle, I2C_SENSOR_ADDR, 0x6B, 1, &pwr_mgmt_1, 1, 100);
    if (result < 0) {
        printf("Failed to initialize sensor, error: %d\n", result);
        return ERROR_HARDWARE;
    }
    
    return ERROR_NONE;
}

/**
 * @brief 读取MPU6050传感器数据
 */
static int read_mpu6050_data(sensor_data_t *data)
{
    uint8_t buffer[14];
    int result;
    
    /* 读取传感器数据，从0x3B寄存器开始，连续读取14个字节 */
    result = i2c_mem_read(i2c_handle, I2C_SENSOR_ADDR, 0x3B, 1, buffer, 14, 100);
    if (result < 0) {
        printf("Failed to read sensor data, error: %d\n", result);
        return ERROR_HARDWARE;
    }
    
    /* 解析数据 */
    data->accel_x = (buffer[0] << 8) | buffer[1];
    data->accel_y = (buffer[2] << 8) | buffer[3];
    data->accel_z = (buffer[4] << 8) | buffer[5];
    
    data->temperature = ((buffer[6] << 8) | buffer[7]) / 340.0f + 36.53f;
    
    data->gyro_x = (buffer[8] << 8) | buffer[9];
    data->gyro_y = (buffer[10] << 8) | buffer[11];
    data->gyro_z = (buffer[12] << 8) | buffer[13];
    
    return ERROR_NONE;
}

/**
 * @brief 传感器任务
 */
static void sensor_task(void *arg)
{
    int result;
    
    /* 初始化MPU6050传感器 */
    result = init_mpu6050();
    if (result != ERROR_NONE) {
        printf("Sensor initialization failed, error: %d\n", result);
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        sensor_data_t local_data;
        
        /* 读取传感器数据 */
        result = read_mpu6050_data(&local_data);
        if (result == ERROR_NONE) {
            /* 更新共享数据，使用互斥锁保护 */
            rtos_mutex_lock(data_mutex, UINT32_MAX);
            memcpy(&g_sensor_data, &local_data, sizeof(sensor_data_t));
            rtos_mutex_unlock(data_mutex);
        }
        
        /* 100ms采样间隔 */
        rtos_thread_sleep_ms(100);
    }
}

/**
 * @brief 通信任务
 */
static void comm_task(void *arg)
{
    char buffer[128];
    sensor_data_t local_data;
    
    while (1) {
        /* 获取最新传感器数据，使用互斥锁保护 */
        rtos_mutex_lock(data_mutex, UINT32_MAX);
        memcpy(&local_data, &g_sensor_data, sizeof(sensor_data_t));
        rtos_mutex_unlock(data_mutex);
        
        /* 格式化数据 */
        snprintf(buffer, sizeof(buffer), 
                "Accel: X=%d, Y=%d, Z=%d\r\n"
                "Gyro: X=%d, Y=%d, Z=%d\r\n"
                "Temp: %.2f C\r\n\r\n",
                local_data.accel_x, local_data.accel_y, local_data.accel_z,
                local_data.gyro_x, local_data.gyro_y, local_data.gyro_z,
                local_data.temperature);
        
        /* 通过UART发送数据 */
        uart_transmit(uart_handle, (uint8_t *)buffer, strlen(buffer), 100);
        
        /* 500ms上报间隔 */
        rtos_thread_sleep_ms(500);
    }
}

/**
 * @brief 应用初始化
 */
static int app_init(void)
{
    int result;
    
    /* 初始化平台 */
    result = platform_init();
    if (result != ERROR_NONE) {
        printf("Platform initialization failed, error: %d\n", result);
        return result;
    }
    
    /* 创建互斥锁 */
    result = rtos_mutex_create(&data_mutex);
    if (result != RTOS_OK) {
        printf("Failed to create mutex, error: %d\n", result);
        return ERROR_RTOS;
    }
    
    /* 初始化LED GPIO */
    gpio_config_t gpio_config = {
        .pin = LED_PIN,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_LOW
    };
    
    result = gpio_init(&gpio_config, &led_handle);
    if (result != ERROR_NONE) {
        printf("GPIO initialization failed, error: %d\n", result);
        return result;
    }
    
    /* 初始化I2C */
    i2c_config_t i2c_config = {
        .channel = I2C_CHANNEL,
        .speed = I2C_SPEED,
        .addr_10bit = false
    };
    
    result = i2c_init(&i2c_config, &i2c_handle);
    if (result != ERROR_NONE) {
        printf("I2C initialization failed, error: %d\n", result);
        return result;
    }
    
    /* 初始化UART */
    uart_config_t uart_config = {
        .channel = UART_CHANNEL,
        .baudrate = UART_BAUDRATE,
        .data_bits = UART_DATA_BITS_8,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .flow_control = UART_FLOW_CONTROL_NONE
    };
    
    result = uart_init(&uart_config, &uart_handle);
    if (result != ERROR_NONE) {
        printf("UART initialization failed, error: %d\n", result);
        return result;
    }
    
    /* 注册UART接收回调 */
    uart_register_rx_callback(uart_handle, uart_rx_callback, NULL);
    
    /* 初始化SPI */
    spi_config_t spi_config = {
        .channel = SPI_CHANNEL,
        .mode = SPI_MODE_0,
        .bit_order = SPI_BIT_ORDER_MSB_FIRST,
        .data_width = SPI_DATA_WIDTH_8BIT,
        .cs_mode = SPI_CS_MODE_SOFTWARE,
        .clock_hz = SPI_SPEED,
        .cs_pin = SPI_CS_PIN
    };
    
    result = spi_init(&spi_config, &spi_handle);
    if (result != ERROR_NONE) {
        printf("SPI initialization failed, error: %d\n", result);
        return result;
    }
    
    return ERROR_NONE;
}

/**
 * @brief 创建应用任务
 */
static int create_tasks(void)
{
    int result;
    
    /* 创建LED任务 */
    result = rtos_thread_create(&led_task_handle, "LED_Task", led_task, 
                               NULL, STACK_SIZE_LED, TASK_PRIORITY_LED);
    if (result != RTOS_OK) {
        printf("Failed to create LED task, error: %d\n", result);
        return ERROR_RTOS;
    }
    
    /* 创建传感器任务 */
    result = rtos_thread_create(&sensor_task_handle, "Sensor_Task", sensor_task, 
                               NULL, STACK_SIZE_SENSOR, TASK_PRIORITY_SENSOR);
    if (result != RTOS_OK) {
        printf("Failed to create sensor task, error: %d\n", result);
        return ERROR_RTOS;
    }
    
    /* 创建通信任务 */
    result = rtos_thread_create(&comm_task_handle, "Comm_Task", comm_task, 
                               NULL, STACK_SIZE_COMM, TASK_PRIORITY_COMM);
    if (result != RTOS_OK) {
        printf("Failed to create communication task, error: %d\n", result);
        return ERROR_RTOS;
    }
    
    return ERROR_NONE;
}

/**
 * @brief 应用主函数
 */
int esp32_example_main(void)
{
    int result;
    
    printf("ESP32 Example Application Starting...\n");
    
    /* 初始化RTOS */
    result = rtos_init();
    if (result != RTOS_OK) {
        printf("RTOS initialization failed, error: %d\n", result);
        return -1;
    }
    
    /* 初始化应用 */
    result = app_init();
    if (result != ERROR_NONE) {
        printf("Application initialization failed, error: %d\n", result);
        return -1;
    }
    
    /* 创建应用任务 */
    result = create_tasks();
    if (result != ERROR_NONE) {
        printf("Task creation failed, error: %d\n", result);
        return -1;
    }
    
    printf("All tasks created, starting scheduler...\n");
    
    /* 启动RTOS调度器 */
    result = rtos_start_scheduler();
    if (result != RTOS_OK) {
        printf("Failed to start scheduler, error: %d\n", result);
        return -1;
    }
    
    /* 调度器启动后不应该返回此处 */
    return 0;
}
