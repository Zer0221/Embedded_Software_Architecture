/**
 * @file tm1681_driver.c
 * @brief TM1681 LED点阵驱动实现
 *
 * 该文件实现了TM1681 LED点阵驱动接口，用于驱动LP110G07点阵显示
 */

#include "base/display_api.h"
#include "base/gpio_api.h"
#include "base/timer_api.h"
#include "common/error_api.h"
#include <string.h>
#include <stdio.h>

/* 驱动版本�?*/
#define TM1681_DRIVER_VERSION "1.0.0"

/* TM1681命令定义 */
#define TM1681_CMD_SYS_DIS         0x00 /* 系统关闭 */
#define TM1681_CMD_SYS_EN          0x01 /* 系统开�?*/
#define TM1681_CMD_LED_OFF         0x02 /* LED关闭 */
#define TM1681_CMD_LED_ON          0x03 /* LED开�?*/
#define TM1681_CMD_BLINK_OFF       0x08 /* 闪烁关闭 */
#define TM1681_CMD_BLINK_2HZ       0x09 /* 2Hz闪烁 */
#define TM1681_CMD_BLINK_1HZ       0x0A /* 1Hz闪烁 */
#define TM1681_CMD_BLINK_05HZ      0x0B /* 0.5Hz闪烁 */
#define TM1681_CMD_COM_8_NMOS      0x20 /* 8COM驱动，NMOS模式 */
#define TM1681_CMD_COM_16_NMOS     0x24 /* 16COM驱动，NMOS模式 */
#define TM1681_CMD_COM_8_PMOS      0x28 /* 8COM驱动，PMOS模式 */
#define TM1681_CMD_COM_16_PMOS     0x2C /* 16COM驱动，PMOS模式 */
#define TM1681_CMD_PWM_CONTROL     0xA0 /* PWM亮度控制，后�?-F亮度级别 */

/* 数据命令 */
#define TM1681_CMD_DATA_MODE       0x40 /* 数据模式命令 */
#define TM1681_CMD_ADDRESS_MODE    0xC0 /* 地址模式命令 */

/* 数据模式设置 */
#define TM1681_DATA_WRITE          0x00 /* 写数据模�?*/
#define TM1681_DATA_READ           0x02 /* 读数据模�?*/
#define TM1681_ADDR_AUTO_INC       0x00 /* 地址自动增加 */
#define TM1681_ADDR_FIXED          0x04 /* 固定地址 */

/* LED点阵配置 */
#define TM1681_MAX_GRIDS           8    /* 最大网格数 */
#define TM1681_GRID_POINTS         8    /* 每个网格的点�?*/
#define TM1681_DELAY_US            10   /* 基本延时(us) */
#define TM1681_START_ADDR          0xC0 /* 起始地址命令 */

/* 亮度级别 (0-15) */
#define TM1681_MIN_BRIGHTNESS      0
#define TM1681_MAX_BRIGHTNESS      15

/* TM1681设备结构�?*/
typedef struct {
    display_config_t config;        /* 显示配置 */
    tm1681_config_t tm1681_config;  /* TM1681配置 */
    gpio_handle_t data_pin;         /* 数据引脚 */
    gpio_handle_t clock_pin;        /* 时钟引脚 */
    gpio_handle_t stb_pin;          /* STB引脚 */
    uint8_t intensity;              /* 亮度级别(0-15) */
    uint8_t *display_buffer;        /* 显示缓冲�?*/
    bool initialized;               /* 初始化标�?*/
    gpio_handle_t data_pin;         /* 数据引脚句柄 */
    gpio_handle_t clock_pin;        /* 时钟引脚句柄 */
    gpio_handle_t stb_pin;          /* STB引脚句柄 */
    uint8_t *display_buffer;        /* 显示缓冲�?*/
    uint16_t buffer_size;           /* 缓冲区大�?*/
    bool initialized;               /* 初始化标�?*/
} tm1681_device_t;

/* 静态设备实�?*/
static tm1681_device_t g_tm1681_device;

/**
 * @brief 微秒级延�?
 * 
 * @param us 延时时间(微秒)
 */
static void tm1681_delay_us(uint32_t us)
{
    /* 使用循环延时，实际项目中应使用定时器或系统延时函�?*/
    volatile uint32_t i;
    for (i = 0; i < us * 10; i++) {
        __asm("NOP");
    }
}

/**
 * @brief 开始TM1681通信
 */
static void tm1681_start(tm1681_device_t *dev)
{
    /* STB高电�?*/
    gpio_write(dev->stb_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
    /* 时钟高电�?*/
    gpio_write(dev->clock_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
    /* STB拉低开始通信 */
    gpio_write(dev->stb_pin, GPIO_LEVEL_LOW);
    tm1681_delay_us(TM1681_DELAY_US);
}

/**
 * @brief 结束TM1681通信
 */
static void tm1681_stop(tm1681_device_t *dev)
{
    /* 时钟低电�?*/
    gpio_write(dev->clock_pin, GPIO_LEVEL_LOW);
    tm1681_delay_us(TM1681_DELAY_US);
    /* STB拉高结束通信 */
    gpio_write(dev->stb_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
}

/**
 * @brief 向TM1681发送一个字�?
 * 
 * @param dev 设备实例
 * @param data 要发送的数据
 */
static void tm1681_write_byte(tm1681_device_t *dev, uint8_t data)
{
    uint8_t i;
    
    for (i = 0; i < 8; i++) {
        /* 时钟低电�?*/
        gpio_write(dev->clock_pin, GPIO_LEVEL_LOW);
        tm1681_delay_us(TM1681_DELAY_US);
        
        /* 设置数据�?*/
        if (data & 0x01) {
            gpio_write(dev->data_pin, GPIO_LEVEL_HIGH);
        } else {
            gpio_write(dev->data_pin, GPIO_LEVEL_LOW);
        }
        
        /* 数据右移 */
        data >>= 1;
        tm1681_delay_us(TM1681_DELAY_US);
        
        /* 时钟高电平，数据锁存 */
        gpio_write(dev->clock_pin, GPIO_LEVEL_HIGH);
        tm1681_delay_us(TM1681_DELAY_US);
    }
}

/**
 * @brief 发送命令到TM1681
 * 
 * @param dev 设备实例
 * @param cmd 命令
 */
static void tm1681_send_command(tm1681_device_t *dev, uint8_t cmd)
{
    tm1681_start(dev);
    tm1681_write_byte(dev, cmd);
    tm1681_stop(dev);
}

/**
 * @brief 设置显示亮度
 * 
 * @param dev 设备实例
 * @param intensity 亮度级别(0-15)
 */
static void tm1681_set_brightness(tm1681_device_t *dev, uint8_t intensity)
{
    /* 限制亮度范围 */
    if (intensity > TM1681_MAX_BRIGHTNESS) {
        intensity = TM1681_MAX_BRIGHTNESS;
    }
    
    /* 发送PWM控制命令 */
    tm1681_send_command(dev, TM1681_CMD_PWM_CONTROL | intensity);
    
    /* 保存亮度设置 */
    dev->intensity = intensity;
}

/**
 * @brief 初始化TM1681硬件
 * 
 * @param dev 设备实例
 * @return int 0表示成功，非0表示失败
 */
static int tm1681_hardware_init(tm1681_device_t *dev)
{
    gpio_config_t gpio_config;
    
    /* 配置数据引脚 */
    memset(&gpio_config, 0, sizeof(gpio_config));
    gpio_config.mode = GPIO_MODE_OUTPUT_PP;
    gpio_config.pull = GPIO_PULL_NONE;
    gpio_config.speed = GPIO_SPEED_HIGH;
    
    /* 初始化数据引�?*/
    if (gpio_init(dev->tm1681_config.data_pin, &gpio_config, &dev->data_pin) != 0) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 初始化时钟引�?*/
    if (gpio_init(dev->tm1681_config.clock_pin, &gpio_config, &dev->clock_pin) != 0) {
        gpio_deinit(dev->data_pin);
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 初始化STB引脚 */
    if (gpio_init(dev->tm1681_config.stb_pin, &gpio_config, &dev->stb_pin) != 0) {
        gpio_deinit(dev->data_pin);
        gpio_deinit(dev->clock_pin);
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 设置初始电平 */
    gpio_write(dev->data_pin, GPIO_LEVEL_HIGH);
    gpio_write(dev->clock_pin, GPIO_LEVEL_HIGH);
    gpio_write(dev->stb_pin, GPIO_LEVEL_HIGH);
    
    /* 初始化TM1681 */
    tm1681_send_command(dev, TM1681_CMD_SYS_DIS);       /* 关闭系统振荡�?*/
    tm1681_send_command(dev, TM1681_CMD_COM_8_NMOS);    /* 设置�?COM, NMOS模式 */
    tm1681_send_command(dev, TM1681_CMD_SYS_EN);        /* 开启系统振荡器 */
    tm1681_send_command(dev, TM1681_CMD_LED_ON);        /* 开启LED显示 */
    tm1681_send_command(dev, TM1681_CMD_BLINK_OFF);     /* 关闭闪烁 */
    
    /* 设置亮度 */
    uint8_t intensity = (dev->tm1681_config.intensity * TM1681_MAX_BRIGHTNESS) / 7;
    tm1681_set_brightness(dev, intensity);
    
    return DRIVER_OK;
}

/**
 * @brief 设置TM1681显示数据
 * 
 * @param dev 设备实例
 * @param address 起始地址
 * @param data 数据缓冲�?
 * @param len 数据长度
 */
/**
 * @brief 初始化显示设�?
 * 
 * @param config 显示配置参数
 * @param handle 显示设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int display_init(const display_config_t *config, display_handle_t *handle)
{
    tm1681_device_t *dev = &g_tm1681_device;
    int ret;
    
    /* 参数检�?*/
    if (config == NULL || handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查显示类�?*/
    if (config->type != DISPLAY_TYPE_LED_MATRIX) {
        return ERROR_NOT_SUPPORTED;
    }
    
    /* 检查驱动配�?*/
    if (config->driver_config == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 复制配置 */
    memcpy(&dev->config, config, sizeof(display_config_t));
    memcpy(&dev->tm1681_config, config->driver_config, sizeof(tm1681_config_t));
    
    /* 分配显示缓冲�?*/
    uint16_t buffer_size = dev->config.width * dev->config.height / 8;
    if (buffer_size == 0) {
        buffer_size = 1;
    }
    
    dev->display_buffer = (uint8_t *)malloc(buffer_size);
    if (dev->display_buffer == NULL) {
        return ERROR_MEMORY_ALLOC_FAILED;
    }
    
    /* 清空显示缓冲�?*/
    memset(dev->display_buffer, 0, buffer_size);
    
    /* 初始化TM1681硬件 */
    ret = tm1681_hardware_init(dev);
    if (ret != DRIVER_OK) {
        free(dev->display_buffer);
        dev->display_buffer = NULL;
        return ret;
    }
    
    /* 设置初始化标�?*/
    dev->initialized = true;
    
    /* 返回设备句柄 */
    *handle = (display_handle_t)dev;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化显示设备
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_deinit(display_handle_t handle)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 关闭显示 */
    tm1681_send_command(dev, TM1681_CMD_LED_OFF);
    tm1681_send_command(dev, TM1681_CMD_SYS_DIS);
    
    /* 释放GPIO资源 */
    gpio_deinit(dev->data_pin);
    gpio_deinit(dev->clock_pin);
    gpio_deinit(dev->stb_pin);
    
    /* 释放显示缓冲�?*/
    if (dev->display_buffer != NULL) {
        free(dev->display_buffer);
        dev->display_buffer = NULL;
    }
    
    /* 清除初始化标�?*/
    dev->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 清除显示内容
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_clear(display_handle_t handle)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 清空显示缓冲�?*/
    uint16_t buffer_size = dev->config.width * dev->config.height / 8;
    if (buffer_size == 0) {
        buffer_size = 1;
    }
    memset(dev->display_buffer, 0, buffer_size);
    
    /* 清除TM1681显示数据 */
    uint8_t clear_data[16] = {0};
    for (uint8_t addr = 0; addr < 16; addr += 2) {
        tm1681_set_display_data(dev, addr, clear_data, 2);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 刷新显示
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_refresh(display_handle_t handle)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算每行所需字节�?*/
    uint8_t bytes_per_row = (dev->config.width + 7) / 8;
    
    /* 将显示缓冲区数据写入TM1681 */
    for (uint8_t row = 0; row < dev->config.height; row++) {
        tm1681_set_display_data(dev, row * 2, &dev->display_buffer[row * bytes_per_row], bytes_per_row);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置像素�?
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素�?
 * @return int 0表示成功，非0表示失败
 */
int display_set_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t value)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查坐标是否超出范�?*/
    if (x >= dev->config.width || y >= dev->config.height) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算像素所在字节和�?*/
    uint16_t byte_index = (y * ((dev->config.width + 7) / 8)) + (x / 8);
    uint8_t bit_index = x % 8;
    
    /* 设置或清除像�?*/
    if (value) {
        dev->display_buffer[byte_index] |= (1 << bit_index);
    } else {
        dev->display_buffer[byte_index] &= ~(1 << bit_index);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取像素�?
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素值指�?
 * @return int 0表示成功，非0表示失败
 */
int display_get_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t *value)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || value == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查坐标是否超出范�?*/
    if (x >= dev->config.width || y >= dev->config.height) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算像素所在字节和�?*/
    uint16_t byte_index = (y * ((dev->config.width + 7) / 8)) + (x / 8);
    uint8_t bit_index = x % 8;
    
    /* 获取像素�?*/
    *value = (dev->display_buffer[byte_index] & (1 << bit_index)) ? 1 : 0;
    
    return DRIVER_OK;
}

/**
 * @brief 设置亮度
 * 
 * @param handle 显示设备句柄
 * @param brightness 亮度�?0-100)
 * @return int 0表示成功，非0表示失败
 */
int display_set_brightness(display_handle_t handle, uint8_t brightness)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 限制亮度范围 */
    if (brightness > 100) {
        brightness = 100;
    }
    
    /* 映射亮度值到TM1681亮度级别(0-15) */
    uint8_t intensity = (brightness * TM1681_MAX_BRIGHTNESS) / 100;
    
    /* 设置亮度 */
    tm1681_set_brightness(dev, intensity);
    
    return DRIVER_OK;
}

/**
 * @brief 绘制位图
 * 
 * @param handle 显示设备句柄
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param width 位图宽度
 * @param height 位图高度
 * @param bitmap 位图数据
 * @return int 0表示成功，非0表示失败
 */
int display_draw_bitmap(display_handle_t handle, uint16_t x, uint16_t y, 
                        uint16_t width, uint16_t height, const uint8_t *bitmap)
{
    tm1681_device_t *dev = (tm1681_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || bitmap == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查坐标和尺寸是否有效 */
    if (x >= dev->config.width || y >= dev->config.height) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 限制位图尺寸 */
    if (x + width > dev->config.width) {
        width = dev->config.width - x;
    }
    if (y + height > dev->config.height) {
        height = dev->config.height - y;
    }
    
    /* 计算位图每行字节�?*/
    uint8_t bitmap_bytes_per_row = (width + 7) / 8;
    
    /* 绘制位图 */
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            uint16_t bitmap_byte_index = row * bitmap_bytes_per_row + col / 8;
            uint8_t bitmap_bit_index = col % 8;
            uint8_t pixel_value = (bitmap[bitmap_byte_index] & (1 << bitmap_bit_index)) ? 1 : 0;
            
            display_set_pixel(handle, x + col, y + row, pixel_value);
        }
    }
    
    return DRIVER_OK;
}

/* 全局设备实例 */
static tm1681_device_t g_tm1681_device;

/* 延时函数 */
static void tm1681_delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < us * 10; i++) {
        __NOP();
    }
}

/* 开始传�?*/
static void tm1681_start(tm1681_device_t *device)
{
    gpio_write(device->stb_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
    gpio_write(device->clock_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
    gpio_write(device->stb_pin, GPIO_LEVEL_LOW);
    tm1681_delay_us(TM1681_DELAY_US);
}

/* 结束传输 */
static void tm1681_stop(tm1681_device_t *device)
{
    gpio_write(device->clock_pin, GPIO_LEVEL_LOW);
    tm1681_delay_us(TM1681_DELAY_US);
    gpio_write(device->stb_pin, GPIO_LEVEL_LOW);
    tm1681_delay_us(TM1681_DELAY_US);
    gpio_write(device->clock_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
    gpio_write(device->stb_pin, GPIO_LEVEL_HIGH);
    tm1681_delay_us(TM1681_DELAY_US);
}

/* 写一个字�?*/
static void tm1681_write_byte(tm1681_device_t *device, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        gpio_write(device->clock_pin, GPIO_LEVEL_LOW);
        tm1681_delay_us(TM1681_DELAY_US);
        
        /* 先写LSB (最低位) */
        gpio_write(device->data_pin, (data & 0x01) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
        tm1681_delay_us(TM1681_DELAY_US);
        
        gpio_write(device->clock_pin, GPIO_LEVEL_HIGH);
        tm1681_delay_us(TM1681_DELAY_US);
        
        data >>= 1;
    }
}

/* 写命�?*/
static void tm1681_write_cmd(tm1681_device_t *device, uint8_t cmd)
{
    tm1681_start(device);
    tm1681_write_byte(device, cmd);
    tm1681_stop(device);
}

/* 写数�?*/
static void tm1681_write_data(tm1681_device_t *device, uint8_t addr, uint8_t data)
{
    tm1681_write_cmd(device, TM1681_CMD_ADDRESS_MODE | addr);
    tm1681_write_byte(device, data);
}

/**
 * @brief 初始化显示设�?
 * 
 * @param config 显示配置参数
 * @param handle 显示设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int display_init(const display_config_t *config, display_handle_t *handle)
{
    /* 检查参数有效�?*/
    if (config == NULL || handle == NULL || config->driver_config == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查显示类�?*/
    if (config->type != DISPLAY_TYPE_LED_MATRIX) {
        return DRIVER_ERROR_UNSUPPORTED;
    }
    
    /* 检查是否已初始�?*/
    if (g_tm1681_device.initialized) {
        return DRIVER_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    memcpy(&g_tm1681_device.config, config, sizeof(display_config_t));
    memcpy(&g_tm1681_device.tm1681_config, config->driver_config, sizeof(tm1681_config_t));
    
    /* 分配显示缓冲�?*/
    g_tm1681_device.buffer_size = config->width * ((config->height + 7) / 8);
    g_tm1681_device.display_buffer = (uint8_t *)malloc(g_tm1681_device.buffer_size);
    if (g_tm1681_device.display_buffer == NULL) {
        return DRIVER_ERROR_OUT_OF_MEMORY;
    }
    
    /* 清空显示缓冲�?*/
    memset(g_tm1681_device.display_buffer, 0, g_tm1681_device.buffer_size);
    
    /* 初始化GPIO引脚 */
    tm1681_config_t *tm_config = (tm1681_config_t *)config->driver_config;
    
    /* 数据引脚配置 */
    gpio_config_t data_pin_config = {
        .port = GPIO_PORT_A + (tm_config->data_pin / 16),
        .pin = tm_config->data_pin % 16,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH
    };
    
    /* 时钟引脚配置 */
    gpio_config_t clock_pin_config = {
        .port = GPIO_PORT_A + (tm_config->clock_pin / 16),
        .pin = tm_config->clock_pin % 16,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH
    };
    
    /* STB引脚配置 */
    gpio_config_t stb_pin_config = {
        .port = GPIO_PORT_A + (tm_config->stb_pin / 16),
        .pin = tm_config->stb_pin % 16,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH
    };
    
    /* 初始化GPIO引脚 */
    if (gpio_init(&data_pin_config, &g_tm1681_device.data_pin) != DRIVER_OK) {
        free(g_tm1681_device.display_buffer);
        return DRIVER_ERROR_PERIPHERAL;
    }
    
    if (gpio_init(&clock_pin_config, &g_tm1681_device.clock_pin) != DRIVER_OK) {
        gpio_deinit(g_tm1681_device.data_pin);
        free(g_tm1681_device.display_buffer);
        return DRIVER_ERROR_PERIPHERAL;
    }
    
    if (gpio_init(&stb_pin_config, &g_tm1681_device.stb_pin) != DRIVER_OK) {
        gpio_deinit(g_tm1681_device.data_pin);
        gpio_deinit(g_tm1681_device.clock_pin);
        free(g_tm1681_device.display_buffer);
        return DRIVER_ERROR_PERIPHERAL;
    }
    
    /* 设置初始电平 */
    gpio_write(g_tm1681_device.data_pin, GPIO_LEVEL_LOW);
    gpio_write(g_tm1681_device.clock_pin, GPIO_LEVEL_LOW);
    gpio_write(g_tm1681_device.stb_pin, GPIO_LEVEL_HIGH);
    
    /* 初始化TM1681 */
    
    /* 选择显示模式 */
    uint8_t display_mode = TM1681_DISPLAY_7_GRIDS;
    if (config->width <= 4) {
        display_mode = TM1681_DISPLAY_4_GRIDS;
    } else if (config->width <= 5) {
        display_mode = TM1681_DISPLAY_5_GRIDS;
    } else if (config->width <= 6) {
        display_mode = TM1681_DISPLAY_6_GRIDS;
    }
    
    /* 发送初始化命令 */
    tm1681_write_cmd(&g_tm1681_device, TM1681_CMD_DISPLAY_MODE | display_mode | TM1681_DATA_NORMAL);
    tm1681_write_cmd(&g_tm1681_device, TM1681_CMD_DATA_MODE | TM1681_ADDR_AUTO_INC);
    
    /* 设置亮度 */
    uint8_t brightness = tm_config->intensity & TM1681_BRIGHTNESS_MASK;
    tm1681_write_cmd(&g_tm1681_device, TM1681_CMD_DISPLAY_CTRL | TM1681_DISPLAY_ON | brightness);
    
    /* 清空显示 */
    for (uint8_t i = 0; i < 16; i++) {
        tm1681_write_data(&g_tm1681_device, i, 0x00);
    }
    
    /* 标记为已初始�?*/
    g_tm1681_device.initialized = true;
    
    /* 返回句柄 */
    *handle = (display_handle_t)&g_tm1681_device;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化显示设备
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_deinit(display_handle_t handle)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 关闭显示 */
    tm1681_write_cmd(device, TM1681_CMD_DISPLAY_CTRL | TM1681_DISPLAY_OFF);
    
    /* 释放资源 */
    gpio_deinit(device->data_pin);
    gpio_deinit(device->clock_pin);
    gpio_deinit(device->stb_pin);
    
    /* 释放显示缓冲�?*/
    if (device->display_buffer != NULL) {
        free(device->display_buffer);
        device->display_buffer = NULL;
    }
    
    /* 清除初始化标�?*/
    device->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 清除显示内容
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_clear(display_handle_t handle)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 清空缓冲�?*/
    memset(device->display_buffer, 0, device->buffer_size);
    
    /* 清空显示 */
    tm1681_write_cmd(device, TM1681_CMD_DATA_MODE | TM1681_ADDR_AUTO_INC);
    tm1681_start(device);
    tm1681_write_byte(device, TM1681_CMD_ADDRESS_MODE);
    
    for (uint8_t i = 0; i < 16; i++) {
        tm1681_write_byte(device, 0x00);
    }
    
    tm1681_stop(device);
    
    return DRIVER_OK;
}

/**
 * @brief 设置像素�?
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素�?
 * @return int 0表示成功，非0表示失败
 */
int display_set_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t value)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查坐标是否在范围�?*/
    if (x >= device->config.width || y >= device->config.height) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 计算缓冲区位�?*/
    uint16_t byte_idx = x + (y / 8) * device->config.width;
    uint8_t bit_idx = y % 8;
    
    /* 设置或清除像�?*/
    if (value) {
        device->display_buffer[byte_idx] |= (1 << bit_idx);
    } else {
        device->display_buffer[byte_idx] &= ~(1 << bit_idx);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取像素�?
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素值指�?
 * @return int 0表示成功，非0表示失败
 */
int display_get_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t *value)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized || value == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查坐标是否在范围�?*/
    if (x >= device->config.width || y >= device->config.height) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 计算缓冲区位�?*/
    uint16_t byte_idx = x + (y / 8) * device->config.width;
    uint8_t bit_idx = y % 8;
    
    /* 获取像素�?*/
    *value = (device->display_buffer[byte_idx] & (1 << bit_idx)) ? 1 : 0;
    
    return DRIVER_OK;
}

/**
 * @brief 设置显示区域数据
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param width 宽度
 * @param height 高度
 * @param data 数据缓冲�?
 * @return int 0表示成功，非0表示失败
 */
int display_set_area(display_handle_t handle, uint16_t x, uint16_t y, 
                     uint16_t width, uint16_t height, const uint8_t *data)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized || data == NULL) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 检查坐标和尺寸是否在范围内 */
    if (x >= device->config.width || y >= device->config.height ||
        x + width > device->config.width || y + height > device->config.height) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 逐像素设置区�?*/
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            uint16_t src_byte_idx = i + j * width / 8;
            uint8_t src_bit_idx = (j * width + i) % 8;
            
            uint32_t pixel_value = (data[src_byte_idx] & (1 << src_bit_idx)) ? 1 : 0;
            display_set_pixel(handle, x + i, y + j, pixel_value);
        }
    }
    
    return DRIVER_OK;
}

/**
 * @brief 刷新显示内容
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_refresh(display_handle_t handle)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 设置自动地址增加模式 */
    tm1681_write_cmd(device, TM1681_CMD_DATA_MODE | TM1681_ADDR_AUTO_INC);
    
    /* 写入显示数据 */
    for (uint8_t grid = 0; grid < device->config.width; grid++) {
        /* 设置地址 */
        tm1681_write_cmd(device, TM1681_CMD_ADDRESS_MODE | grid);
        
        /* 计算该列的数�?*/
        uint8_t column_data = 0;
        for (uint8_t row = 0; row < device->config.height; row++) {
            uint16_t byte_idx = grid + (row / 8) * device->config.width;
            uint8_t bit_idx = row % 8;
            
            if (device->display_buffer[byte_idx] & (1 << bit_idx)) {
                column_data |= (1 << row);
            }
        }
        
        /* 写入数据 */
        tm1681_write_byte(device, column_data);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 设置显示亮度
 * 
 * @param handle 显示设备句柄
 * @param brightness 亮度(0-100)
 * @return int 0表示成功，非0表示失败
 */
int display_set_brightness(display_handle_t handle, uint8_t brightness)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 将亮度转换为0-7范围 */
    uint8_t tm1681_brightness = (brightness * 7) / 100;
    
    /* 设置亮度 */
    tm1681_write_cmd(device, TM1681_CMD_DISPLAY_CTRL | TM1681_DISPLAY_ON | tm1681_brightness);
    
    return DRIVER_OK;
}

/**
 * @brief 设置显示方向
 * 
 * @param handle 显示设备句柄
 * @param orientation 显示方向
 * @return int 0表示成功，非0表示失败
 */
int display_set_orientation(display_handle_t handle, display_orientation_t orientation)
{
    /* TM1681不支持旋转，返回不支�?*/
    return DRIVER_ERROR_UNSUPPORTED;
}

/**
 * @brief 显示字符
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param ch 字符
 * @param color 前景�?
 * @param bg_color 背景�?
 * @return int 0表示成功，非0表示失败
 */
int display_draw_char(display_handle_t handle, uint16_t x, uint16_t y, 
                      char ch, uint32_t color, uint32_t bg_color)
{
    /* TM1681点阵显示暂不支持字符显示 */
    return DRIVER_ERROR_UNSUPPORTED;
}

/**
 * @brief 显示字符�?
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param str 字符�?
 * @param color 前景�?
 * @param bg_color 背景�?
 * @return int 0表示成功，非0表示失败
 */
int display_draw_string(display_handle_t handle, uint16_t x, uint16_t y, 
                        const char *str, uint32_t color, uint32_t bg_color)
{
    /* TM1681点阵显示暂不支持字符显示 */
    return DRIVER_ERROR_UNSUPPORTED;
}

/**
 * @brief 获取显示设备信息
 * 
 * @param handle 显示设备句柄
 * @param width 宽度指针
 * @param height 高度指针
 * @param color_format 色彩格式指针
 * @return int 0表示成功，非0表示失败
 */
int display_get_info(display_handle_t handle, uint16_t *width, uint16_t *height, 
                     display_color_t *color_format)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 返回显示信息 */
    if (width != NULL) {
        *width = device->config.width;
    }
    
    if (height != NULL) {
        *height = device->config.height;
    }
    
    if (color_format != NULL) {
        *color_format = device->config.color_format;
    }
    
    return DRIVER_OK;
}

/**
 * @brief 进入睡眠模式
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_sleep(display_handle_t handle)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 关闭显示 */
    tm1681_write_cmd(device, TM1681_CMD_DISPLAY_CTRL | TM1681_DISPLAY_OFF);
    
    return DRIVER_OK;
}

/**
 * @brief 退出睡眠模�?
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_wakeup(display_handle_t handle)
{
    tm1681_device_t *device = (tm1681_device_t *)handle;
    
    /* 检查参数有效�?*/
    if (device != &g_tm1681_device || !device->initialized) {
        return DRIVER_ERROR_INVALID_PARAMETER;
    }
    
    /* 打开显示 */
    uint8_t brightness = device->tm1681_config.intensity & TM1681_BRIGHTNESS_MASK;
    tm1681_write_cmd(device, TM1681_CMD_DISPLAY_CTRL | TM1681_DISPLAY_ON | brightness);
    
    return DRIVER_OK;
}

/**
 * @brief 获取驱动版本
 * 
 * @return const char* 版本字符�?
 */
const char* display_get_version(void)
{
    return TM1681_DRIVER_VERSION;
}

