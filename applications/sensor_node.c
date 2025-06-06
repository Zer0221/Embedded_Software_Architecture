/**
 * @file main.c
 * @brief 多功能传感器节点示例应用
 *
 * 本示例演示了如何使用软件架构实现一个多功能传感器节点，
 * 包括温湿度传感器、OLED显示和无线通信功能。
 */

#include "base/platform_api.h"
#include "common/rtos_api.h"
#include "base/gpio_api.h"
#include "base/i2c_api.h"
#include "base/uart_api.h"
#include "base/power_api.h"
#include "common/error_api.h"

/* 配置参数 */
#define SENSOR_UPDATE_INTERVAL    5000    /* 传感器更新间隔（毫秒） */
#define DISPLAY_UPDATE_INTERVAL   1000    /* 显示更新间隔（毫秒） */
#define UART_BAUDRATE             115200  /* UART波特率 */
#define I2C_SPEED                 I2C_SPEED_FAST  /* I2C速度 */

/* 设备地址 */
#define SHT30_ADDR                0x44    /* SHT30温湿度传感器地址 */
#define OLED_ADDR                 0x3C    /* SSD1306 OLED显示器地址 */

/* 全局变量 */
static uart_handle_t g_uart_handle;
static i2c_handle_t g_i2c_handle;
static gpio_handle_t g_led_handle;
static gpio_handle_t g_button_handle;

static rtos_mutex_t g_data_mutex;
static rtos_sem_t g_button_sem;

static float g_temperature = 0.0f;
static float g_humidity = 0.0f;
static bool g_display_on = true;
static bool g_low_power_mode = false;

/* 接收缓冲区 */
static uint8_t g_rx_buffer[128];
static uint8_t g_tx_buffer[128];

/* 任务句柄 */
static rtos_thread_t g_sensor_task_handle;
static rtos_thread_t g_display_task_handle;
static rtos_thread_t g_comm_task_handle;

/* 按钮中断回调函数 */
static void button_irq_handler(gpio_port_t port, gpio_pin_t pin, void *user_data)
{
    /* 在中断上下文中释放信号量 */
    rtos_sem_give(g_button_sem);
}

/* UART回调函数 */
static void uart_callback(uart_handle_t handle, uart_event_t event, void *user_data)
{
    /* 处理UART事件 */
    if (event == UART_EVENT_RX_COMPLETE) {
        /* 处理接收到的命令 */
        if (strncmp((char *)g_rx_buffer, "DISPLAY:ON", 10) == 0) {
            g_display_on = true;
        } else if (strncmp((char *)g_rx_buffer, "DISPLAY:OFF", 11) == 0) {
            g_display_on = false;
        } else if (strncmp((char *)g_rx_buffer, "POWER:LOW", 9) == 0) {
            g_low_power_mode = true;
        } else if (strncmp((char *)g_rx_buffer, "POWER:NORMAL", 12) == 0) {
            g_low_power_mode = false;
        }
        
        /* 启动新的接收 */
        uart_receive_it(g_uart_handle, g_rx_buffer, sizeof(g_rx_buffer));
    }
}

/* 错误处理回调函数 */
static void error_handler(int error_code, const char *file, int line, 
                         const char *func, void *user_data)
{
    /* 格式化错误信息 */
    sprintf((char *)g_tx_buffer, "ERROR[%d]: %s at %s:%d in %s\r\n", 
           error_code, error_get_string(error_code), file, line, func);
    
    /* 通过UART发送错误信息 */
    uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 100);
    
    /* 对于严重错误，闪烁LED并重置系统 */
    if (error_code <= ERROR_CRITICAL) {
        for (int i = 0; i < 10; i++) {
            gpio_toggle(g_led_handle);
            platform_delay_ms(100);
        }
        platform_reset();
    }
}

/* 初始化SHT30温湿度传感器 */
static int init_sht30(void)
{
    uint8_t cmd[2] = {0x30, 0xA2}; /* 设置高重复性测量命令 */
    
    /* 发送命令 */
    return i2c_master_transmit(g_i2c_handle, SHT30_ADDR, cmd, sizeof(cmd), I2C_FLAG_STOP, 100);
}

/* 读取SHT30温湿度数据 */
static int read_sht30(float *temperature, float *humidity)
{
    uint8_t cmd[2] = {0x2C, 0x06}; /* 单次测量命令，高重复性 */
    uint8_t data[6];
    int result;
    uint16_t temp_raw, hum_raw;
    
    /* 发送测量命令 */
    result = i2c_master_transmit(g_i2c_handle, SHT30_ADDR, cmd, sizeof(cmd), I2C_FLAG_STOP, 100);
    if (result < 0) {
        return result;
    }
    
    /* 延时等待测量完成 */
    platform_delay_ms(20);
    
    /* 读取测量结果 */
    result = i2c_master_receive(g_i2c_handle, SHT30_ADDR, data, sizeof(data), I2C_FLAG_STOP, 100);
    if (result < 0) {
        return result;
    }
    
    /* 检查CRC校验（简化处理，实际应用中应验证CRC） */
    
    /* 计算温度和湿度 */
    temp_raw = (data[0] << 8) | data[1];
    hum_raw = (data[3] << 8) | data[4];
    
    *temperature = -45.0f + 175.0f * temp_raw / 65535.0f;
    *humidity = 100.0f * hum_raw / 65535.0f;
    
    return ERROR_NONE;
}

/* 初始化SSD1306 OLED显示器 */
static int init_oled(void)
{
    uint8_t init_cmds[] = {
        0x00,  /* 命令模式 */
        0xAE,  /* 关闭显示 */
        0xD5, 0x80,  /* 设置显示时钟 */
        0xA8, 0x3F,  /* 设置多路复用比 */
        0xD3, 0x00,  /* 设置显示偏移 */
        0x40,  /* 设置显示起始行 */
        0x8D, 0x14,  /* 启用电荷泵 */
        0x20, 0x00,  /* 设置内存寻址模式 */
        0xA1,  /* 段重映射 */
        0xC8,  /* COM输出扫描方向 */
        0xDA, 0x12,  /* 设置COM引脚配置 */
        0x81, 0xCF,  /* 设置对比度 */
        0xD9, 0xF1,  /* 设置预充电周期 */
        0xDB, 0x40,  /* 设置VCOM解除选择电平 */
        0xA4,  /* 整个显示打开 */
        0xA6,  /* 正常显示（非反相） */
        0xAF   /* 打开显示 */
    };
    
    /* 发送初始化命令 */
    return i2c_master_transmit(g_i2c_handle, OLED_ADDR, init_cmds, sizeof(init_cmds), I2C_FLAG_STOP, 200);
}

/* 清除OLED显示 */
static int clear_oled(void)
{
    uint8_t clear_cmd[] = {
        0x00,  /* 命令模式 */
        0x21, 0x00, 0x7F,  /* 设置列地址范围 */
        0x22, 0x00, 0x07   /* 设置页地址范围 */
    };
    int result;
    
    /* 发送命令 */
    result = i2c_master_transmit(g_i2c_handle, OLED_ADDR, clear_cmd, sizeof(clear_cmd), I2C_FLAG_STOP, 100);
    if (result < 0) {
        return result;
    }
    
    /* 清除显存 */
    uint8_t data[129];
    data[0] = 0x40;  /* 数据模式 */
    memset(data + 1, 0, 128);
    
    /* 逐页清除 */
    for (int page = 0; page < 8; page++) {
        result = i2c_master_transmit(g_i2c_handle, OLED_ADDR, data, sizeof(data), I2C_FLAG_STOP, 100);
        if (result < 0) {
            return result;
        }
    }
    
    return ERROR_NONE;
}

/* 在OLED上显示文本 */
static int oled_display_text(uint8_t page, uint8_t column, const char *text)
{
    uint8_t set_pos[] = {
        0x00,  /* 命令模式 */
        0xB0 | (page & 0x07),  /* 设置页地址 */
        0x00 | (column & 0x0F),  /* 设置低4位列地址 */
        0x10 | ((column >> 4) & 0x0F)  /* 设置高4位列地址 */
    };
    int result;
    
    /* 设置位置 */
    result = i2c_master_transmit(g_i2c_handle, OLED_ADDR, set_pos, sizeof(set_pos), I2C_FLAG_STOP, 100);
    if (result < 0) {
        return result;
    }
    
    /* 简单字体，仅用于演示，实际应用中应使用字体库 */
    /* 每个字符使用8x8像素 */
    uint8_t data[2];
    data[0] = 0x40;  /* 数据模式 */
    
    /* 发送文本数据 */
    while (*text) {
        char c = *text++;
        
        /* 简单ASCII字符映射，仅用于演示 */
        for (int i = 0; i < 8; i++) {
            /* 简单模式，每个字符用8个字节表示 */
            switch (c) {
                case '0': data[1] = (uint8_t[]){0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, 0x00}[i]; break;
                case '1': data[1] = (uint8_t[]){0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}[i]; break;
                case '2': data[1] = (uint8_t[]){0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00}[i]; break;
                case '3': data[1] = (uint8_t[]){0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00, 0x00}[i]; break;
                case '4': data[1] = (uint8_t[]){0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00, 0x00}[i]; break;
                case '5': data[1] = (uint8_t[]){0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00}[i]; break;
                case '6': data[1] = (uint8_t[]){0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00}[i]; break;
                case '7': data[1] = (uint8_t[]){0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00, 0x00}[i]; break;
                case '8': data[1] = (uint8_t[]){0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}[i]; break;
                case '9': data[1] = (uint8_t[]){0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x00}[i]; break;
                case '.': data[1] = (uint8_t[]){0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}[i]; break;
                case '%': data[1] = (uint8_t[]){0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00}[i]; break;
                case 'C': data[1] = (uint8_t[]){0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00, 0x00}[i]; break;
                case 'T': data[1] = (uint8_t[]){0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00}[i]; break;
                case 'H': data[1] = (uint8_t[]){0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00, 0x00}[i]; break;
                case 'R': data[1] = (uint8_t[]){0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00, 0x00}[i]; break;
                case ':': data[1] = (uint8_t[]){0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00}[i]; break;
                case ' ': data[1] = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}[i]; break;
                default:  data[1] = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}[i]; break;
            }
            
            /* 发送字符数据 */
            result = i2c_master_transmit(g_i2c_handle, OLED_ADDR, data, sizeof(data), i < 7 ? I2C_FLAG_NO_STOP : I2C_FLAG_STOP, 100);
            if (result < 0) {
                return result;
            }
        }
    }
    
    return ERROR_NONE;
}

/* 传感器任务 */
static void sensor_task(void *arg)
{
    float temp, hum;
    
    /* 初始化温湿度传感器 */
    if (ERROR_CHECK(init_sht30()) != ERROR_NONE) {
        return;
    }
    
    while (1) {
        /* 读取传感器数据 */
        if (read_sht30(&temp, &hum) == ERROR_NONE) {
            /* 获取互斥锁 */
            rtos_mutex_lock(g_data_mutex, UINT32_MAX);
            
            /* 更新数据 */
            g_temperature = temp;
            g_humidity = hum;
            
            /* 释放互斥锁 */
            rtos_mutex_unlock(g_data_mutex);
            
            /* 发送数据到UART */
            sprintf((char *)g_tx_buffer, "T:%.2f,H:%.2f\r\n", temp, hum);
            uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 100);
        }
        
        /* 闪烁LED指示传感器活动 */
        gpio_toggle(g_led_handle);
        
        /* 根据功耗模式调整休眠时间 */
        if (g_low_power_mode) {
            rtos_thread_sleep_ms(SENSOR_UPDATE_INTERVAL * 5);
        } else {
            rtos_thread_sleep_ms(SENSOR_UPDATE_INTERVAL);
        }
    }
}

/* 显示任务 */
static void display_task(void *arg)
{
    char temp_str[16];
    char hum_str[16];
    float temp, hum;
    
    /* 初始化OLED显示器 */
    if (ERROR_CHECK(init_oled()) != ERROR_NONE) {
        return;
    }
    
    /* 清除显示 */
    clear_oled();
    
    /* 显示标题 */
    oled_display_text(0, 0, "TEMP & HUM SENSOR");
    
    while (1) {
        /* 如果显示开启，则更新显示 */
        if (g_display_on) {
            /* 获取互斥锁 */
            rtos_mutex_lock(g_data_mutex, UINT32_MAX);
            
            /* 读取数据 */
            temp = g_temperature;
            hum = g_humidity;
            
            /* 释放互斥锁 */
            rtos_mutex_unlock(g_data_mutex);
            
            /* 格式化数据 */
            sprintf(temp_str, "TEMP: %.2f C", temp);
            sprintf(hum_str, "HUM:  %.2f %%", hum);
            
            /* 更新显示 */
            oled_display_text(2, 0, temp_str);
            oled_display_text(4, 0, hum_str);
            
            /* 显示当前模式 */
            if (g_low_power_mode) {
                oled_display_text(6, 0, "MODE: LOW POWER");
            } else {
                oled_display_text(6, 0, "MODE: NORMAL");
            }
        }
        
        /* 根据功耗模式调整休眠时间 */
        if (g_low_power_mode) {
            rtos_thread_sleep_ms(DISPLAY_UPDATE_INTERVAL * 3);
        } else {
            rtos_thread_sleep_ms(DISPLAY_UPDATE_INTERVAL);
        }
    }
}

/* 通信任务 */
static void comm_task(void *arg)
{
    /* 启动异步接收 */
    uart_receive_it(g_uart_handle, g_rx_buffer, sizeof(g_rx_buffer));
    
    /* 发送欢迎消息 */
    sprintf((char *)g_tx_buffer, "Sensor Node Started\r\nCommands:\r\n- DISPLAY:ON/OFF\r\n- POWER:LOW/NORMAL\r\n");
    uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 100);
    
    while (1) {
        /* 等待按钮按下 */
        if (rtos_sem_take(g_button_sem, UINT32_MAX) == RTOS_OK) {
            /* 消抖 */
            rtos_thread_sleep_ms(50);
            
            /* 如果按钮仍然按下 */
            if (gpio_read(g_button_handle) == GPIO_PIN_RESET) {
                /* 切换显示状态 */
                g_display_on = !g_display_on;
                
                /* 发送状态变更消息 */
                sprintf((char *)g_tx_buffer, "Display: %s\r\n", g_display_on ? "ON" : "OFF");
                uart_transmit(g_uart_handle, g_tx_buffer, strlen((char *)g_tx_buffer), 100);
                
                /* 如果显示关闭，则清除显示 */
                if (!g_display_on) {
                    clear_oled();
                }
            }
        }
    }
}

/* 主函数 */
int main(void)
{
    /* 初始化平台 */
    platform_init();
    
    /* 设置错误回调 */
    error_set_callback(error_handler, NULL);
    
    /* 初始化RTOS */
    rtos_init();
    
    /* 创建互斥锁和信号量 */
    rtos_mutex_create(&g_data_mutex);
    rtos_sem_create(&g_button_sem, 0, 1);
    
    /* 配置LED GPIO */
    gpio_config_t led_config = {
        .port = GPIO_PORT_A,
        .pin = GPIO_PIN_5,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_LOW
    };
    
    /* 初始化LED GPIO */
    if (ERROR_CHECK(gpio_init(&led_config, &g_led_handle)) != ERROR_NONE) {
        return -1;
    }
    
    /* 配置按钮GPIO */
    gpio_config_t button_config = {
        .port = GPIO_PORT_C,
        .pin = GPIO_PIN_13,
        .mode = GPIO_MODE_IT_FALLING,
        .pull = GPIO_PULL_UP
    };
    
    /* 初始化按钮GPIO */
    if (ERROR_CHECK(gpio_init(&button_config, &g_button_handle)) != ERROR_NONE) {
        return -1;
    }
    
    /* 注册按钮中断回调 */
    gpio_register_irq_callback(g_button_handle, button_irq_handler, NULL);
    gpio_enable_irq(g_button_handle);
    
    /* 配置I2C */
    i2c_config_t i2c_config = {
        .channel = I2C_CHANNEL_1,
        .speed = I2C_SPEED,
        .addr_10bit = false
    };
    
    /* 初始化I2C */
    if (ERROR_CHECK(i2c_init(&i2c_config, &g_i2c_handle)) != ERROR_NONE) {
        return -1;
    }
    
    /* 配置UART */
    uart_config_t uart_config = {
        .channel = UART_CHANNEL_1,
        .baudrate = UART_BAUDRATE,
        .data_bits = UART_DATA_BITS_8,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .mode = UART_MODE_TX_RX,
        .flow_ctrl = UART_FLOW_CTRL_NONE
    };
    
    /* 初始化UART */
    if (ERROR_CHECK(uart_init(&uart_config, &g_uart_handle)) != ERROR_NONE) {
        return -1;
    }
    
    /* 注册UART回调 */
    uart_register_callback(g_uart_handle, uart_callback, NULL);
    
    /* 初始化电源管理 */
    power_init();
    
    /* 创建任务 */
    rtos_thread_create(&g_sensor_task_handle, "Sensor", sensor_task, NULL, 1024, RTOS_PRIORITY_NORMAL);
    rtos_thread_create(&g_display_task_handle, "Display", display_task, NULL, 1024, RTOS_PRIORITY_NORMAL);
    rtos_thread_create(&g_comm_task_handle, "Comm", comm_task, NULL, 1024, RTOS_PRIORITY_NORMAL);
    
    /* 启动RTOS调度器 */
    rtos_start_scheduler();
    
    /* 永远不会执行到这里 */
    return 0;
}
