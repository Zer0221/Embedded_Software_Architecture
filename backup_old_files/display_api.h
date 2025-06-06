/**
 * @file display_api.h
 * @brief 显示接口抽象层定义
 *
 * 该头文件定义了显示设备的统一抽象接口，使上层应用能够与底层显示硬件实现解耦
 */

#ifndef DISPLAY_API_H
#define DISPLAY_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 显示类型 */
typedef enum {
    DISPLAY_TYPE_LCD,        /**< LCD显示 */
    DISPLAY_TYPE_OLED,       /**< OLED显示 */
    DISPLAY_TYPE_LED_MATRIX, /**< LED点阵显示 */
    DISPLAY_TYPE_E_PAPER     /**< 电子墨水屏 */
} display_type_t;

/* 显示方向 */
typedef enum {
    DISPLAY_ORIENTATION_0,    /**< 0度旋转 */
    DISPLAY_ORIENTATION_90,   /**< 90度旋转 */
    DISPLAY_ORIENTATION_180,  /**< 180度旋转 */
    DISPLAY_ORIENTATION_270   /**< 270度旋转 */
} display_orientation_t;

/* 显示色彩深度 */
typedef enum {
    DISPLAY_COLOR_MONO,       /**< 单色 */
    DISPLAY_COLOR_GRAY4,      /**< 4级灰度 */
    DISPLAY_COLOR_GRAY16,     /**< 16级灰度 */
    DISPLAY_COLOR_RGB565,     /**< RGB565 */
    DISPLAY_COLOR_RGB888      /**< RGB888 */
} display_color_t;

/* 显示配置参数 */
typedef struct {
    display_type_t type;               /**< 显示类型 */
    uint16_t width;                    /**< 显示宽度(像素) */
    uint16_t height;                   /**< 显示高度(像素) */
    display_orientation_t orientation; /**< 显示方向 */
    display_color_t color_format;      /**< 色彩格式 */
    uint8_t brightness;                /**< 亮度(0-100) */
    void *driver_config;               /**< 驱动特定配置 */
} display_config_t;

/* TM1681点阵配置参数(用于driver_config) */
typedef struct {
    uint8_t grid_width;    /**< 点阵宽度 */
    uint8_t grid_height;   /**< 点阵高度 */
    uint8_t data_pin;      /**< 数据引脚 */
    uint8_t clock_pin;     /**< 时钟引脚 */
    uint8_t stb_pin;       /**< STB引脚 */
    uint8_t intensity;     /**< 亮度级别(0-7) */
} tm1681_config_t;

/* 显示设备句柄 */
typedef driver_handle_t display_handle_t;

/**
 * @brief 初始化显示设备
 * 
 * @param config 显示配置参数
 * @param handle 显示设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int display_init(const display_config_t *config, display_handle_t *handle);

/**
 * @brief 去初始化显示设备
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_deinit(display_handle_t handle);

/**
 * @brief 清除显示内容
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_clear(display_handle_t handle);

/**
 * @brief 设置像素点
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素值
 * @return int 0表示成功，非0表示失败
 */
int display_set_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t value);

/**
 * @brief 获取像素点
 * 
 * @param handle 显示设备句柄
 * @param x X坐标
 * @param y Y坐标
 * @param value 像素值指针
 * @return int 0表示成功，非0表示失败
 */
int display_get_pixel(display_handle_t handle, uint16_t x, uint16_t y, uint32_t *value);

/**
 * @brief 设置显示区域数据
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param width 宽度
 * @param height 高度
 * @param data 数据缓冲区
 * @return int 0表示成功，非0表示失败
 */
int display_set_area(display_handle_t handle, uint16_t x, uint16_t y, 
                     uint16_t width, uint16_t height, const uint8_t *data);

/**
 * @brief 刷新显示内容
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_refresh(display_handle_t handle);

/**
 * @brief 设置显示亮度
 * 
 * @param handle 显示设备句柄
 * @param brightness 亮度(0-100)
 * @return int 0表示成功，非0表示失败
 */
int display_set_brightness(display_handle_t handle, uint8_t brightness);

/**
 * @brief 设置显示方向
 * 
 * @param handle 显示设备句柄
 * @param orientation 显示方向
 * @return int 0表示成功，非0表示失败
 */
int display_set_orientation(display_handle_t handle, display_orientation_t orientation);

/**
 * @brief 显示字符
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param ch 字符
 * @param color 前景色
 * @param bg_color 背景色
 * @return int 0表示成功，非0表示失败
 */
int display_draw_char(display_handle_t handle, uint16_t x, uint16_t y, 
                      char ch, uint32_t color, uint32_t bg_color);

/**
 * @brief 显示字符串
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param str 字符串
 * @param color 前景色
 * @param bg_color 背景色
 * @return int 0表示成功，非0表示失败
 */
int display_draw_string(display_handle_t handle, uint16_t x, uint16_t y, 
                        const char *str, uint32_t color, uint32_t bg_color);

/**
 * @brief 绘制线段
 * 
 * @param handle 显示设备句柄
 * @param x0 起点X坐标
 * @param y0 起点Y坐标
 * @param x1 终点X坐标
 * @param y1 终点Y坐标
 * @param color 线段颜色
 * @return int 0表示成功，非0表示失败
 */
int display_draw_line(display_handle_t handle, uint16_t x0, uint16_t y0, 
                      uint16_t x1, uint16_t y1, uint32_t color);

/**
 * @brief 绘制矩形
 * 
 * @param handle 显示设备句柄
 * @param x 左上角X坐标
 * @param y 左上角Y坐标
 * @param width 宽度
 * @param height 高度
 * @param color 线段颜色
 * @return int 0表示成功，非0表示失败
 */
int display_draw_rect(display_handle_t handle, uint16_t x, uint16_t y, 
                      uint16_t width, uint16_t height, uint32_t color);

/**
 * @brief 绘制填充矩形
 * 
 * @param handle 显示设备句柄
 * @param x 左上角X坐标
 * @param y 左上角Y坐标
 * @param width 宽度
 * @param height 高度
 * @param color 填充颜色
 * @return int 0表示成功，非0表示失败
 */
int display_fill_rect(display_handle_t handle, uint16_t x, uint16_t y, 
                      uint16_t width, uint16_t height, uint32_t color);

/**
 * @brief 绘制圆形
 * 
 * @param handle 显示设备句柄
 * @param x 圆心X坐标
 * @param y 圆心Y坐标
 * @param radius 半径
 * @param color 线段颜色
 * @return int 0表示成功，非0表示失败
 */
int display_draw_circle(display_handle_t handle, uint16_t x, uint16_t y, 
                        uint16_t radius, uint32_t color);

/**
 * @brief 绘制填充圆形
 * 
 * @param handle 显示设备句柄
 * @param x 圆心X坐标
 * @param y 圆心Y坐标
 * @param radius 半径
 * @param color 填充颜色
 * @return int 0表示成功，非0表示失败
 */
int display_fill_circle(display_handle_t handle, uint16_t x, uint16_t y, 
                        uint16_t radius, uint32_t color);

/**
 * @brief 显示图像
 * 
 * @param handle 显示设备句柄
 * @param x X起始坐标
 * @param y Y起始坐标
 * @param width 图像宽度
 * @param height 图像高度
 * @param data 图像数据
 * @return int 0表示成功，非0表示失败
 */
int display_draw_image(display_handle_t handle, uint16_t x, uint16_t y, 
                       uint16_t width, uint16_t height, const uint8_t *data);

/**
 * @brief 屏幕翻转显示
 * 
 * @param handle 显示设备句柄
 * @param flipped true表示翻转显示，false表示正常显示
 * @return int 0表示成功，非0表示失败
 */
int display_set_flipped(display_handle_t handle, bool flipped);

/**
 * @brief 屏幕反色显示
 * 
 * @param handle 显示设备句柄
 * @param inverted true表示反色显示，false表示正常显示
 * @return int 0表示成功，非0表示失败
 */
int display_set_inverted(display_handle_t handle, bool inverted);

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
                     display_color_t *color_format);

/**
 * @brief 进入睡眠模式
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_sleep(display_handle_t handle);

/**
 * @brief 退出睡眠模式
 * 
 * @param handle 显示设备句柄
 * @return int 0表示成功，非0表示失败
 */
int display_wakeup(display_handle_t handle);

#endif /* DISPLAY_API_H */
