# FM33LC0xx平台LED矩阵显示驱动使用指南

## 概述

本文档提供了如何在FM33LC0xx微控制器平台上使用抽象层API开发LED矩阵显示应用的详细指导。我们的抽象层设计实现了硬件与应用的解耦，便于代码重用和跨平台移植。本指南重点介绍TM1681 LED点阵驱动器的应用开发。

## 目录

1. [硬件准备](#1-硬件准备)
2. [软件架构](#2-软件架构)
3. [基本API使用](#3-基本api使用)
4. [显示效果实现](#4-显示效果实现)
5. [性能优化](#5-性能优化)
6. [故障排除](#6-故障排除)

## 1. 硬件准备

### 1.1 所需组件

- FM33LC0xx系列微控制器（如FM33LC04x）
- TM1681 LED点阵驱动芯片
- 7×7（或其他尺寸）LED点阵显示屏
- 连接线、电阻等辅助元件

### 1.2 硬件连接

TM1681驱动芯片需要以下连接：

```
FM33LC0xx       TM1681
-----------------------
PA0    <-----> DATA
PA1    <-----> CLOCK
PA2    <-----> STB
VCC    <-----> VDD
GND    <-----> GND
```

## 2. 软件架构

我们采用分层设计，从底向上包括：

```
+---------------------+
|    应用层           |
+---------------------+
|    抽象接口层       |
+---------------------+
|    驱动实现层       |
+---------------------+
|    平台适配层       |
+---------------------+
|    硬件             |
+---------------------+
```

### 2.1 核心文件

- `display_api.h`: 显示设备抽象接口定义
- `tm1681_driver.c`: TM1681 LED矩阵驱动实现
- `fm33lc0xx_gpio.c`: FM33LC0xx平台的GPIO驱动
- `fm33lc0xx_platform.c`: FM33LC0xx平台适配
- `fm33lc0xx_matrix_display_example.c`: 应用示例

## 3. 基本API使用

### 3.1 初始化显示设备

```c
/* TM1681配置 */
tm1681_config_t tm1681_config = {
    .grid_width = 7,          /* 点阵宽度 */
    .grid_height = 7,         /* 点阵高度 */
    .data_pin = 0,            /* PA0 */
    .clock_pin = 1,           /* PA1 */
    .stb_pin = 2,             /* PA2 */
    .intensity = 5            /* 亮度级别 (0-7) */
};

/* 显示配置 */
display_config_t display_config = {
    .type = DISPLAY_TYPE_LED_MATRIX,
    .width = 7,
    .height = 7,
    .orientation = DISPLAY_ORIENTATION_0,
    .color_format = DISPLAY_COLOR_MONO,
    .brightness = 70,          /* 亮度百分比 */
    .driver_config = &tm1681_config
};

/* 初始化显示 */
display_handle_t display;
if (display_init(&display_config, &display) != 0) {
    /* 错误处理 */
}
```

### 3.2 像素操作

```c
/* 设置像素 */
display_set_pixel(display, x, y, 1);  /* 点亮像素 */
display_set_pixel(display, x, y, 0);  /* 熄灭像素 */

/* 获取像素状态 */
uint32_t pixel_value;
display_get_pixel(display, x, y, &pixel_value);

/* 清除所有像素 */
display_clear(display);

/* 刷新显示 */
display_refresh(display);
```

### 3.3 控制亮度

```c
/* 设置亮度 (0-100) */
display_set_brightness(display, 70);  /* 70% 亮度 */
```

### 3.4 绘制位图

```c
/* 位图数据 (7x7点阵的笑脸) */
const uint8_t smile_bitmap[7] = {
    0b00111000,
    0b01000100,
    0b10100010,
    0b10000010,
    0b10100010,
    0b01000100,
    0b00111000
};

/* 显示位图 */
for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 7; x++) {
        uint8_t pixel = (smile_bitmap[y] >> (6 - x)) & 0x01;
        display_set_pixel(display, x, y, pixel);
    }
}
display_refresh(display);
```

## 4. 显示效果实现

### 4.1 动画实现

```c
/* 循环显示动画帧 */
for (int i = 0; i < frame_count; i++) {
    /* 清除显示 */
    display_clear(display);
    
    /* 绘制当前帧 */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t pixel = (frames[i][y] >> (width - 1 - x)) & 0x01;
            display_set_pixel(display, x, y, pixel);
        }
    }
    
    /* 刷新显示并等待 */
    display_refresh(display);
    timer_delay_ms(200);  /* 帧延时 */
}
```

### 4.2 滚动文本

```c
void scroll_text(display_handle_t display, const uint8_t text[][7], int text_len) {
    for (int i = 0; i < text_len; i++) {
        /* 显示当前字符 */
        display_frame(display, text[i]);
        timer_delay_ms(600);
        
        /* 滚动到下一个字符 */
        for (int j = 0; j < 7; j++) {
            /* 清除显示 */
            display_clear(display);
            
            /* 绘制当前字符滚出 */
            for (int y = 0; y < 7; y++) {
                for (int x = j; x < 7; x++) {
                    uint8_t pixel = (text[i][y] >> (6 - (x - j))) & 0x01;
                    display_set_pixel(display, x, y, pixel);
                }
            }
            
            /* 绘制下一个字符滚入 */
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < j; x++) {
                    uint8_t pixel = (text[(i + 1) % text_len][y] >> (6 - (7 - j + x))) & 0x01;
                    display_set_pixel(display, x, y, pixel);
                }
            }
            
            /* 刷新显示并等待 */
            display_refresh(display);
            timer_delay_ms(100);
        }
    }
}
```

### 4.3 波纹效果

```c
void ripple_effect(display_handle_t display) {
    for (int r = 0; r <= 3; r++) {
        /* 清除显示 */
        display_clear(display);
        
        /* 绘制圆环 */
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                int dx = x - 3;
                int dy = y - 3;
                int dist = (dx * dx + dy * dy);
                
                if (dist <= r * r && dist > (r - 1) * (r - 1)) {
                    display_set_pixel(display, x, y, 1);
                }
            }
        }
        
        /* 刷新显示并等待 */
        display_refresh(display);
        timer_delay_ms(100);
    }
}
```

### 4.4 下雨效果

```c
void rain_effect(display_handle_t display) {
    uint8_t drops[7] = {0};
    
    /* 随机初始化雨滴位置 */
    for (int x = 0; x < 7; x++) {
        drops[x] = rand() % 7;
    }
    
    for (int f = 0; f < 30; f++) {
        /* 清除显示 */
        display_clear(display);
        
        /* 更新和绘制雨滴 */
        for (int x = 0; x < 7; x++) {
            display_set_pixel(display, x, drops[x], 1);
            
            /* 向下移动雨滴 */
            drops[x] = (drops[x] + 1) % 7;
            
            /* 随机创建新雨滴 */
            if (rand() % 10 == 0) {
                drops[x] = 0;
            }
        }
        
        /* 刷新显示并等待 */
        display_refresh(display);
        timer_delay_ms(200);
    }
}
```

## 5. 性能优化

### 5.1 减少刷新次数

避免频繁调用`display_refresh()`，尽量在完成所有像素设置后才刷新显示：

```c
// 不推荐
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        display_set_pixel(display, x, y, bitmap[y][x]);
        display_refresh(display);  // 频繁刷新
    }
}

// 推荐
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        display_set_pixel(display, x, y, bitmap[y][x]);
    }
}
display_refresh(display);  // 只在所有像素设置完成后刷新一次
```

### 5.2 使用批量更新

对于复杂的显示内容，可以先准备好完整的显示缓冲区，然后一次性更新：

```c
uint8_t buffer[7];
memset(buffer, 0, sizeof(buffer));

/* 在缓冲区中设置像素 */
buffer[y] |= (1 << (7 - x));

/* 一次性更新整个显示 */
for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 7; x++) {
        uint8_t pixel = (buffer[y] >> (7 - 1 - x)) & 0x01;
        display_set_pixel(display, x, y, pixel);
    }
}
display_refresh(display);
```

### 5.3 优化GPIO操作

TM1681通信过程中的GPIO操作可能是性能瓶颈，可以考虑使用直接寄存器访问或DMA方式优化：

```c
/* 直接寄存器访问（比抽象API更快） */
#define DATA_PORT   GPIOA
#define DATA_PIN    LL_GPIO_PIN_0
#define CLOCK_PORT  GPIOA
#define CLOCK_PIN   LL_GPIO_PIN_1

/* 设置数据位 */
if (data & 0x01) {
    LL_GPIO_SetOutputPin(DATA_PORT, DATA_PIN);
} else {
    LL_GPIO_ResetOutputPin(DATA_PORT, DATA_PIN);
}
```

## 6. 故障排除

### 6.1 显示不亮

- 检查电源连接
- 确认DATA/CLOCK/STB引脚连接正确
- 验证亮度设置不为0
- 检查是否正确初始化TM1681
- 确认`display_refresh()`被调用

### 6.2 显示错误

- 检查点阵尺寸配置是否正确
- 确认位图数据格式与点阵排列一致
- 验证x/y坐标映射是否正确

### 6.3 显示闪烁

- 检查时序是否符合TM1681要求
- 确认电源电压稳定
- 减少刷新频率

### 6.4 调试技巧

- 使用逻辑分析仪检查DATA/CLOCK/STB信号
- 添加调试日志跟踪函数调用
- 尝试显示简单图案（如单个像素）进行验证
- 使用示波器检查时序是否符合TM1681要求
