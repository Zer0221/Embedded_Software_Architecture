/**
 * @file fm33lc0xx_matrix_display_example.c
 * @brief FM33LC0xx平台点阵显示示例应用
 *
 * 该文件实现了一个基于FM33LC0xx平台的点阵显示示例应用，
 * 展示如何使用抽象接口操作TM1681驱动的LED点阵显示
 */

#include "base/display_api.h"
#include "base/timer_api.h"
#include "base/gpio_api.h"
#include "base/platform_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 显示参数 */
#define MATRIX_WIDTH        7  /* 点阵宽度 */
#define MATRIX_HEIGHT       7  /* 点阵高度 */
#define DATA_PIN            0  /* PA0 */
#define CLOCK_PIN           1  /* PA1 */
#define STB_PIN             2  /* PA2 */
#define MATRIX_BRIGHTNESS   70 /* 70% 亮度 */

/* 延时参数 */
#define ANIMATION_DELAY_MS  200

/* 动画帧定义 */
static const uint8_t ANIMATION_FRAMES[][7] = {
    /* 笑脸 */
    {
        0b00111000,
        0b01000100,
        0b10100010,
        0b10000010,
        0b10100010,
        0b01000100,
        0b00111000
    },
    /* 心形 */
    {
        0b00000000,
        0b01100110,
        0b11111110,
        0b11111110,
        0b01111100,
        0b00111000,
        0b00010000
    },
    /* 箭头向上 */
    {
        0b00010000,
        0b00111000,
        0b01010100,
        0b10010010,
        0b00010000,
        0b00010000,
        0b00010000
    },
    /* 箭头向下 */
    {
        0b00010000,
        0b00010000,
        0b00010000,
        0b10010010,
        0b01010100,
        0b00111000,
        0b00010000
    },
    /* 星形 */
    {
        0b00010000,
        0b00010000,
        0b01010100,
        0b00111000,
        0b01010100,
        0b00010000,
        0b00010000
    }
};

#define ANIMATION_FRAME_COUNT (sizeof(ANIMATION_FRAMES) / sizeof(ANIMATION_FRAMES[0]))

/* 显示一个动画帧 */
static void display_frame(display_handle_t display, const uint8_t frame[])
{
    /* 清除显示 */
    display_clear(display);
    
    /* 设置像素 */
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint8_t pixel = (frame[y] >> (MATRIX_WIDTH - 1 - x)) & 0x01;
            display_set_pixel(display, x, y, pixel);
        }
    }
    
    /* 刷新显示 */
    display_refresh(display);
}

/* 运行动画 */
static void run_animation(display_handle_t display)
{
    /* 循环显示所有帧 */
    for (int i = 0; i < ANIMATION_FRAME_COUNT; i++) {
        display_frame(display, ANIMATION_FRAMES[i]);
        timer_delay_ms(ANIMATION_DELAY_MS);
    }
}

/* 波纹效果 */
static void ripple_effect(display_handle_t display)
{
    for (int r = 0; r <= MATRIX_WIDTH / 2; r++) {
        /* 清除显示 */
        display_clear(display);
        
        /* 绘制圆环 */
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x++) {
                int dx = x - MATRIX_WIDTH / 2;
                int dy = y - MATRIX_HEIGHT / 2;
                int dist = (dx * dx + dy * dy);
                
                if (dist <= r * r && dist > (r - 1) * (r - 1)) {
                    display_set_pixel(display, x, y, 1);
                }
            }
        }
        
        /* 刷新显示 */
        display_refresh(display);
        
        /* 延时 */
        timer_delay_ms(ANIMATION_DELAY_MS / 2);
    }
}

/**
 * @brief 下雨效果
 * 
 * @param display 显示设备句柄
 */
static void rain_effect(display_handle_t display)
{
    /* 清除显示 */
    display_clear(display);
    display_refresh(display);
    
    /* 创建雨滴 */
    uint8_t drops[MATRIX_WIDTH] = {0};
    
    /* 随机初始化雨滴位置 */
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        drops[x] = rand() % MATRIX_HEIGHT;
    }
    
    /* 运行雨滴动画 */
    for (int f = 0; f < 30; f++) {
        /* 清除显示 */
        display_clear(display);
        
        /* 更新和绘制雨滴 */
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            display_set_pixel(display, x, drops[x], 1);
            
            /* 向下移动雨滴 */
            drops[x] = (drops[x] + 1) % MATRIX_HEIGHT;
            
            /* 随机创建新雨滴 */
            if (rand() % 10 == 0) {
                drops[x] = 0;
            }
        }
        
        /* 刷新显示 */
        display_refresh(display);
        
        /* 延时 */
        timer_delay_ms(ANIMATION_DELAY_MS);
    }
}

/**
 * @brief 滚动文本效果
 * 
 * @param display 显示设备句柄
 * @param text 要滚动的文本
 * @param text_len 文本长度
 */
static void scroll_text_effect(display_handle_t display, const uint8_t text[][7], int text_len)
{
    for (int i = 0; i < text_len; i++) {
        /* 显示当前字符 */
        display_frame(display, text[i]);
        timer_delay_ms(ANIMATION_DELAY_MS * 3);
        
        /* 滚动效果 */
        for (int j = 0; j < MATRIX_WIDTH; j++) {
            /* 清除显示 */
            display_clear(display);
            
            /* 绘制当前字符滚出 */
            for (int y = 0; y < MATRIX_HEIGHT; y++) {
                for (int x = j; x < MATRIX_WIDTH; x++) {
                    uint8_t pixel = (text[i][y] >> (MATRIX_WIDTH - 1 - (x - j))) & 0x01;
                    display_set_pixel(display, x, y, pixel);
                }
            }
            
            /* 绘制下一个字符滚入 */
            for (int y = 0; y < MATRIX_HEIGHT; y++) {
                for (int x = 0; x < j; x++) {
                    uint8_t pixel = (text[(i + 1) % text_len][y] >> (MATRIX_WIDTH - 1 - (MATRIX_WIDTH - j + x))) & 0x01;
                    display_set_pixel(display, x, y, pixel);
                }
            }
            
            /* 刷新显示 */
            display_refresh(display);
            
            /* 延时 */
            timer_delay_ms(ANIMATION_DELAY_MS / 2);
        }
    }
}

/* 字母数字定义 */
static const uint8_t ALPHA_NUMERIC[][7] = {
    /* 字母H */
    {
        0b10000010,
        0b10000010,
        0b10000010,
        0b11111110,
        0b10000010,
        0b10000010,
        0b10000010
    },
    /* 字母I */
    {
        0b01111100,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b01111100
    },
    /* 数字8 */
    {
        0b01111100,
        0b10000010,
        0b10000010,
        0b01111100,
        0b10000010,
        0b10000010,
        0b01111100
    }
};

/* 运行滚动文本 */
static void run_text_animation(display_handle_t display)
{
    scroll_text_effect(display, ALPHA_NUMERIC, sizeof(ALPHA_NUMERIC) / sizeof(ALPHA_NUMERIC[0]));
}

/* 主函数 */
int main(void)
{
    /* 初始化平台 */
    platform_init();
    
    /* TM1681配置 */
    tm1681_config_t tm1681_config = {
        .grid_width = MATRIX_WIDTH,
        .grid_height = MATRIX_HEIGHT,
        .data_pin = DATA_PIN,
        .clock_pin = CLOCK_PIN,
        .stb_pin = STB_PIN,
        .intensity = 5  /* 亮度级别 (0-7) */
    };
    
    /* 显示配置 */
    display_config_t display_config = {
        .type = DISPLAY_TYPE_LED_MATRIX,
        .width = MATRIX_WIDTH,
        .height = MATRIX_HEIGHT,
        .orientation = DISPLAY_ORIENTATION_0,
        .color_format = DISPLAY_COLOR_MONO,
        .brightness = MATRIX_BRIGHTNESS,
        .driver_config = &tm1681_config
    };
    
    /* 初始化显示 */
    display_handle_t display;
    if (display_init(&display_config, &display) != 0) {
        /* 初始化失败，闪烁LED指示错误 */
        gpio_handle_t led;
        gpio_config_t led_config = {
            .port = GPIO_PORT_C,
            .pin = 13,
            .mode = GPIO_MODE_OUTPUT_PP,
            .pull = GPIO_PULL_NONE,
            .speed = GPIO_SPEED_LOW
        };
        
        gpio_init(&led_config, &led);
        
        while (1) {
            gpio_toggle(led);
            timer_delay_ms(100);
        }
    }
    
    /* 设置亮度 */
    display_set_brightness(display, MATRIX_BRIGHTNESS);
    
    /* 主循环 */
    while (1) {
        /* 运行基本动画 */
        for (int i = 0; i < 2; i++) {
            run_animation(display);
        }
        
        /* 波纹效果 */
        for (int i = 0; i < 2; i++) {
            ripple_effect(display);
        }
        
        /* 下雨效果 */
        rain_effect(display);
        
        /* 字母数字滚动显示 */
        run_text_animation(display);
        
        /* 短暂暂停 */
        display_clear(display);
        display_refresh(display);
        timer_delay_ms(500);
    }
    
    /* 不会执行到这里 */
    display_deinit(display);
    platform_deinit();
    
    return 0;
}
