/**
 * @file camera_api.h
 * @brief 摄像头接口抽象层定义
 *
 * 该头文件定义了摄像头的统一抽象接口，用于图像采集和视频流处理
 */

#ifndef CAMERA_API_H
#define CAMERA_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/driver_api.h"

/* 摄像头句柄 */
typedef driver_handle_t camera_handle_t;

/* 图像格式枚举 */
typedef enum {
    IMAGE_FORMAT_RGB565,      /**< RGB565格式 */
    IMAGE_FORMAT_RGB888,      /**< RGB888格式 */
    IMAGE_FORMAT_YUV422,      /**< YUV422格式 */
    IMAGE_FORMAT_YUV420,      /**< YUV420格式 */
    IMAGE_FORMAT_JPEG,        /**< JPEG格式 */
    IMAGE_FORMAT_GRAYSCALE,   /**< 灰度格式 */
    IMAGE_FORMAT_RAW,         /**< 原始格式 */
    IMAGE_FORMAT_CUSTOM       /**< 自定义格式 */
} image_format_t;

/* 图像分辨率枚举 */
typedef enum {
    RESOLUTION_QVGA,          /**< 320x240 */
    RESOLUTION_VGA,           /**< 640x480 */
    RESOLUTION_HD,            /**< 1280x720 */
    RESOLUTION_FULL_HD,       /**< 1920x1080 */
    RESOLUTION_CUSTOM         /**< 自定义分辨率 */
} image_resolution_t;

/* 帧率枚举 */
typedef enum {
    FRAME_RATE_5FPS,          /**< 5帧/秒 */
    FRAME_RATE_10FPS,         /**< 10帧/秒 */
    FRAME_RATE_15FPS,         /**< 15帧/秒 */
    FRAME_RATE_20FPS,         /**< 20帧/秒 */
    FRAME_RATE_25FPS,         /**< 25帧/秒 */
    FRAME_RATE_30FPS,         /**< 30帧/秒 */
    FRAME_RATE_60FPS,         /**< 60帧/秒 */
    FRAME_RATE_CUSTOM         /**< 自定义帧率 */
} frame_rate_t;

/* 图像结构体 */
typedef struct {
    uint8_t *data;                /**< 图像数据 */
    uint32_t length;              /**< 数据长度 */
    uint32_t width;               /**< 图像宽度 */
    uint32_t height;              /**< 图像高度 */
    image_format_t format;        /**< 图像格式 */
    uint32_t timestamp;           /**< 时间戳 */
} camera_image_t;

/* 摄像头配置结构体 */
typedef struct {
    image_resolution_t resolution;    /**< 分辨率 */
    image_format_t format;            /**< 图像格式 */
    frame_rate_t frame_rate;          /**< 帧率 */
    uint16_t custom_width;            /**< 自定义宽度(仅当resolution为RESOLUTION_CUSTOM时有效) */
    uint16_t custom_height;           /**< 自定义高度(仅当resolution为RESOLUTION_CUSTOM时有效) */
    uint8_t custom_fps;               /**< 自定义帧率(仅当frame_rate为FRAME_RATE_CUSTOM时有效) */
    uint8_t brightness;               /**< 亮度(0-100) */
    uint8_t contrast;                 /**< 对比度(0-100) */
    uint8_t saturation;               /**< 饱和度(0-100) */
    uint8_t sharpness;                /**< 锐度(0-100) */
    bool auto_exposure;               /**< 自动曝光 */
    bool auto_white_balance;          /**< 自动白平衡 */
    bool night_mode;                  /**< 夜间模式 */
    uint16_t buffer_count;            /**< 缓冲区数量 */
} camera_config_t;

/* 帧回调函数类型 */
typedef void (*camera_frame_callback_t)(camera_image_t *image, void *user_data);

/**
 * @brief 初始化摄像头
 * 
 * @param config 摄像头配置参数
 * @param handle 返回的摄像头句柄
 * @return int 0表示成功，非0表示失败
 */
int camera_init(const camera_config_t *config, camera_handle_t *handle);

/**
 * @brief 启动摄像头
 * 
 * @param handle 摄像头句柄
 * @return int 0表示成功，非0表示失败
 */
int camera_start(camera_handle_t handle);

/**
 * @brief 停止摄像头
 * 
 * @param handle 摄像头句柄
 * @return int 0表示成功，非0表示失败
 */
int camera_stop(camera_handle_t handle);

/**
 * @brief 注册帧回调
 * 
 * @param handle 摄像头句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int camera_register_frame_callback(camera_handle_t handle, 
                                  camera_frame_callback_t callback, 
                                  void *user_data);

/**
 * @brief 捕获单帧图像
 * 
 * @param handle 摄像头句柄
 * @param image 返回的图像结构体指针
 * @param timeout_ms 超时时间(毫秒)
 * @return int 0表示成功，非0表示失败
 */
int camera_capture_image(camera_handle_t handle, 
                        camera_image_t *image, 
                        uint32_t timeout_ms);

/**
 * @brief 释放图像
 * 
 * @param image 图像结构体指针
 * @return int 0表示成功，非0表示失败
 */
int camera_release_image(camera_image_t *image);

/**
 * @brief 设置摄像头参数
 * 
 * @param handle 摄像头句柄
 * @param param_id 参数ID
 * @param value 参数值
 * @return int 0表示成功，非0表示失败
 */
int camera_set_parameter(camera_handle_t handle, 
                        uint32_t param_id, 
                        int32_t value);

/**
 * @brief 获取摄像头参数
 * 
 * @param handle 摄像头句柄
 * @param param_id 参数ID
 * @param value 返回的参数值
 * @return int 0表示成功，非0表示失败
 */
int camera_get_parameter(camera_handle_t handle, 
                        uint32_t param_id, 
                        int32_t *value);

/**
 * @brief 设置区域曝光
 * 
 * @param handle 摄像头句柄
 * @param x 区域左上角x坐标
 * @param y 区域左上角y坐标
 * @param width 区域宽度
 * @param height 区域高度
 * @return int 0表示成功，非0表示失败
 */
int camera_set_exposure_area(camera_handle_t handle, 
                            uint16_t x, 
                            uint16_t y, 
                            uint16_t width, 
                            uint16_t height);

/**
 * @brief 设置区域对焦
 * 
 * @param handle 摄像头句柄
 * @param x 区域左上角x坐标
 * @param y 区域左上角y坐标
 * @param width 区域宽度
 * @param height 区域高度
 * @return int 0表示成功，非0表示失败
 */
int camera_set_focus_area(camera_handle_t handle, 
                         uint16_t x, 
                         uint16_t y, 
                         uint16_t width, 
                         uint16_t height);

/**
 * @brief 转换图像格式
 * 
 * @param src_image 源图像
 * @param dst_format 目标格式
 * @param dst_image 目标图像
 * @return int 0表示成功，非0表示失败
 */
int camera_convert_format(const camera_image_t *src_image, 
                         image_format_t dst_format, 
                         camera_image_t *dst_image);

/**
 * @brief 调整图像大小
 * 
 * @param src_image 源图像
 * @param dst_width 目标宽度
 * @param dst_height 目标高度
 * @param dst_image 目标图像
 * @return int 0表示成功，非0表示失败
 */
int camera_resize_image(const camera_image_t *src_image, 
                       uint16_t dst_width, 
                       uint16_t dst_height, 
                       camera_image_t *dst_image);

/**
 * @brief 镜像图像
 * 
 * @param src_image 源图像
 * @param horizontal 是否水平镜像
 * @param vertical 是否垂直镜像
 * @param dst_image 目标图像
 * @return int 0表示成功，非0表示失败
 */
int camera_mirror_image(const camera_image_t *src_image, 
                       bool horizontal, 
                       bool vertical, 
                       camera_image_t *dst_image);

/**
 * @brief 旋转图像
 * 
 * @param src_image 源图像
 * @param angle 旋转角度(90/180/270)
 * @param dst_image 目标图像
 * @return int 0表示成功，非0表示失败
 */
int camera_rotate_image(const camera_image_t *src_image, 
                       uint16_t angle, 
                       camera_image_t *dst_image);

#endif /* CAMERA_API_H */
