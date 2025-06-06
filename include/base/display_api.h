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
#include "common/driver_api.h"
#include "common/project_config.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    uint8_t data_pin;                   /**< 数据引脚 */
    uint8_t clk_pin;                    /**< 时钟引脚 */
    uint8_t stb_pin;                    /**< 选通引脚 */
    uint8_t grid_num;                   /**< 网格数量 */
    uint8_t segments_per_grid;          /**< 每网格段数 */
    uint8_t brightness_levels;          /**< 亮度级别 */
} tm1681_config_t;

/* 点坐标 */
typedef struct {
    int16_t x;                          /**< X坐标 */
    int16_t y;                          /**< Y坐标 */
} display_point_t;

/* 矩形区域 */
typedef struct {
    int16_t x;                          /**< 左上角X坐标 */
    int16_t y;                          /**< 左上角Y坐标 */
    uint16_t width;                     /**< 宽度 */
    uint16_t height;                    /**< 高度 */
} display_rect_t;

/* 字体信息 */
typedef struct {
    uint8_t width;                      /**< 字符宽度 */
    uint8_t height;                     /**< 字符高度 */
    uint8_t spacing;                    /**< 字符间距 */
    const uint8_t *data;                /**< 字体数据 */
} display_font_t;

/* 显示操作结果 */
typedef enum {
    DISPLAY_RESULT_OK,                  /**< 操作成功 */
    DISPLAY_RESULT_ERROR,               /**< 操作失败 */
    DISPLAY_RESULT_TIMEOUT,             /**< 操作超时 */
    DISPLAY_RESULT_NOT_SUPPORTED,       /**< 不支持的操作 */
    DISPLAY_RESULT_INVALID_PARAM        /**< 无效参数 */
} display_result_t;

/* 显示刷新模式 */
typedef enum {
    DISPLAY_REFRESH_NORMAL,             /**< 正常刷新 */
    DISPLAY_REFRESH_PARTIAL,            /**< 局部刷新 */
    DISPLAY_REFRESH_FULL                /**< 全屏刷新 */
} display_refresh_mode_t;

/* 显示状态 */
typedef struct {
    bool initialized;                   /**< 是否初始化 */
    uint8_t brightness;                 /**< 当前亮度 */
    bool is_sleeping;                   /**< 是否处于睡眠状态 */
    display_orientation_t orientation;  /**< 当前方向 */
    uint32_t refresh_count;             /**< 刷新次数 */
} display_status_t;

/**
 * @brief 初始化显示设备
 *
 * @param config 显示配置参数
 * @return api_status_t 操作状态
 */
api_status_t display_init(const display_config_t *config);

/**
 * @brief 反初始化显示设备
 *
 * @return api_status_t 操作状态
 */
api_status_t display_deinit(void);

/**
 * @brief 清除显示内容
 *
 * @return api_status_t 操作状态
 */
api_status_t display_clear(void);

/**
 * @brief 刷新显示内容
 *
 * @param mode 刷新模式
 * @param rect 刷新区域(仅用于局部刷新)
 * @return api_status_t 操作状态
 */
api_status_t display_refresh(display_refresh_mode_t mode, const display_rect_t *rect);

/**
 * @brief 设置显示亮度
 *
 * @param brightness 亮度值(0-100)
 * @return api_status_t 操作状态
 */
api_status_t display_set_brightness(uint8_t brightness);

/**
 * @brief 获取显示亮度
 *
 * @param brightness 获取到的亮度值
 * @return api_status_t 操作状态
 */
api_status_t display_get_brightness(uint8_t *brightness);

/**
 * @brief 设置显示方向
 *
 * @param orientation 显示方向
 * @return api_status_t 操作状态
 */
api_status_t display_set_orientation(display_orientation_t orientation);

/**
 * @brief 绘制像素点
 *
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色值
 * @return api_status_t 操作状态
 */
api_status_t display_draw_pixel(int16_t x, int16_t y, uint32_t color);

/**
 * @brief 绘制线段
 *
 * @param x0 起点X坐标
 * @param y0 起点Y坐标
 * @param x1 终点X坐标
 * @param y1 终点Y坐标
 * @param color 颜色值
 * @return api_status_t 操作状态
 */
api_status_t display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);

/**
 * @brief 绘制矩形
 *
 * @param rect 矩形区域
 * @param color 颜色值
 * @param filled 是否填充
 * @return api_status_t 操作状态
 */
api_status_t display_draw_rect(const display_rect_t *rect, uint32_t color, bool filled);

/**
 * @brief 绘制圆形
 *
 * @param x 圆心X坐标
 * @param y 圆心Y坐标
 * @param radius 半径
 * @param color 颜色值
 * @param filled 是否填充
 * @return api_status_t 操作状态
 */
api_status_t display_draw_circle(int16_t x, int16_t y, uint16_t radius, uint32_t color, bool filled);

/**
 * @brief 绘制字符
 *
 * @param x X坐标
 * @param y Y坐标
 * @param c 字符
 * @param font 字体
 * @param color 颜色值
 * @param bg_color 背景色
 * @return api_status_t 操作状态
 */
api_status_t display_draw_char(int16_t x, int16_t y, char c, const display_font_t *font, uint32_t color, uint32_t bg_color);

/**
 * @brief 绘制字符串
 *
 * @param x X坐标
 * @param y Y坐标
 * @param str 字符串
 * @param font 字体
 * @param color 颜色值
 * @param bg_color 背景色
 * @return api_status_t 操作状态
 */
api_status_t display_draw_string(int16_t x, int16_t y, const char *str, const display_font_t *font, uint32_t color, uint32_t bg_color);

/**
 * @brief 绘制位图
 *
 * @param x X坐标
 * @param y Y坐标
 * @param width 位图宽度
 * @param height 位图高度
 * @param bitmap 位图数据
 * @return api_status_t 操作状态
 */
api_status_t display_draw_bitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *bitmap);

/**
 * @brief 绘制图像
 *
 * @param rect 图像区域
 * @param data 图像数据
 * @param format 图像格式
 * @return api_status_t 操作状态
 */
api_status_t display_draw_image(const display_rect_t *rect, const void *data, display_color_t format);

/**
 * @brief 进入睡眠模式
 *
 * @return api_status_t 操作状态
 */
api_status_t display_sleep(void);

/**
 * @brief 退出睡眠模式
 *
 * @return api_status_t 操作状态
 */
api_status_t display_wakeup(void);

/**
 * @brief 获取显示状态
 *
 * @param status 状态信息
 * @return api_status_t 操作状态
 */
api_status_t display_get_status(display_status_t *status);

/**
 * @brief 获取显示信息
 *
 * @param width 宽度
 * @param height 高度
 * @param color_format 色彩格式
 * @return api_status_t 操作状态
 */
api_status_t display_get_info(uint16_t *width, uint16_t *height, display_color_t *color_format);

/**
 * @brief 反转显示内容
 *
 * @param invert 是否反转
 * @return api_status_t 操作状态
 */
api_status_t display_invert(bool invert);

/**
 * @brief 设置显示对比度
 *
 * @param contrast 对比度(0-100)
 * @return api_status_t 操作状态
 */
api_status_t display_set_contrast(uint8_t contrast);

/**
 * @brief 执行设备特定命令
 *
 * @param cmd 命令
 * @param arg 参数
 * @return api_status_t 操作状态
 */
api_status_t display_ioctl(uint32_t cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_API_H */