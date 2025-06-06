/**
 * @file media_manager_api.h
 * @brief 媒体资源管理接口抽象层定义
 *
 * 该头文件定义了媒体资源管理的统一抽象接口，包括图片、视频等媒体文件的存储、加载和管理
 */

#ifndef MEDIA_MANAGER_API_H
#define MEDIA_MANAGER_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 媒体管理器句柄 */
typedef driver_handle_t media_manager_handle_t;

/* 媒体类型枚举 */
typedef enum {
    MEDIA_TYPE_IMAGE,         /**< 图片 */
    MEDIA_TYPE_VIDEO,         /**< 视频 */
    MEDIA_TYPE_AUDIO,         /**< 音频 */
    MEDIA_TYPE_DOCUMENT,      /**< 文档 */
    MEDIA_TYPE_OTHER          /**< 其他 */
} media_type_t;

/* 媒体格式枚举 */
typedef enum {
    /* 图片格式 */
    MEDIA_FORMAT_JPEG,        /**< JPEG格式 */
    MEDIA_FORMAT_PNG,         /**< PNG格式 */
    MEDIA_FORMAT_BMP,         /**< BMP格式 */
    MEDIA_FORMAT_GIF,         /**< GIF格式 */
    
    /* 视频格式 */
    MEDIA_FORMAT_MP4,         /**< MP4格式 */
    MEDIA_FORMAT_AVI,         /**< AVI格式 */
    
    /* 音频格式 */
    MEDIA_FORMAT_MP3,         /**< MP3格式 */
    MEDIA_FORMAT_WAV,         /**< WAV格式 */
    
    /* 其他格式 */
    MEDIA_FORMAT_UNKNOWN      /**< 未知格式 */
} media_format_t;

/* 媒体信息结构体 */
typedef struct {
    char filename[64];            /**< 文件名 */
    media_type_t type;            /**< 媒体类型 */
    media_format_t format;        /**< 媒体格式 */
    uint32_t size;                /**< 文件大小 */
    uint32_t width;               /**< 宽度(图片/视频) */
    uint32_t height;              /**< 高度(图片/视频) */
    uint32_t duration;            /**< 时长(视频/音频) */
    uint32_t created_time;        /**< 创建时间 */
    uint32_t modified_time;       /**< 修改时间 */
    char checksum[32];            /**< 校验和 */
    bool encrypted;               /**< 是否加密 */
} media_info_t;

/* 媒体管理器配置参数 */
typedef struct {
    char *storage_path;           /**< 存储路径 */
    uint32_t max_storage_size;    /**< 最大存储大小(字节) */
    bool enable_encryption;       /**< 是否启用加密 */
    char *encryption_key;         /**< 加密密钥 */
    uint16_t cache_size;          /**< 缓存大小 */
} media_manager_config_t;

/**
 * @brief 初始化媒体管理器
 * 
 * @param config 管理器配置参数
 * @param handle 管理器句柄指针
 * @return int 0表示成功，非0表示失败
 */
int media_manager_init(const media_manager_config_t *config, media_manager_handle_t *handle);

/**
 * @brief 保存媒体文件
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param data 数据指针
 * @param data_len 数据长度
 * @param type 媒体类型
 * @param format 媒体格式
 * @return int 0表示成功，非0表示失败
 */
int media_manager_save_file(media_manager_handle_t handle, 
                           const char *filename, 
                           const void *data, 
                           size_t data_len, 
                           media_type_t type, 
                           media_format_t format);

/**
 * @brief 加载媒体文件
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param buffer 缓冲区指针
 * @param buffer_size 缓冲区大小
 * @param actual_size 实际读取的大小
 * @return int 0表示成功，非0表示失败
 */
int media_manager_load_file(media_manager_handle_t handle, 
                           const char *filename, 
                           void *buffer, 
                           size_t buffer_size, 
                           size_t *actual_size);

/**
 * @brief 删除媒体文件
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @return int 0表示成功，非0表示失败
 */
int media_manager_delete_file(media_manager_handle_t handle, const char *filename);

/**
 * @brief 获取媒体文件信息
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param info 媒体信息结构体指针
 * @return int 0表示成功，非0表示失败
 */
int media_manager_get_file_info(media_manager_handle_t handle, 
                               const char *filename, 
                               media_info_t *info);

/**
 * @brief 列出媒体文件
 * 
 * @param handle 管理器句柄
 * @param type 媒体类型，MEDIA_TYPE_OTHER表示所有类型
 * @param info_array 媒体信息数组
 * @param array_size 数组大小
 * @param actual_count 实际文件数量
 * @return int 0表示成功，非0表示失败
 */
int media_manager_list_files(media_manager_handle_t handle, 
                            media_type_t type, 
                            media_info_t *info_array, 
                            size_t array_size, 
                            size_t *actual_count);

/**
 * @brief 检查存储空间
 * 
 * @param handle 管理器句柄
 * @param total_space 总空间(字节)
 * @param free_space 可用空间(字节)
 * @return int 0表示成功，非0表示失败
 */
int media_manager_check_storage(media_manager_handle_t handle, 
                               uint64_t *total_space, 
                               uint64_t *free_space);

/**
 * @brief 重命名媒体文件
 * 
 * @param handle 管理器句柄
 * @param old_filename 旧文件名
 * @param new_filename 新文件名
 * @return int 0表示成功，非0表示失败
 */
int media_manager_rename_file(media_manager_handle_t handle, 
                             const char *old_filename, 
                             const char *new_filename);

/**
 * @brief 为媒体文件生成缩略图
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param thumb_filename 缩略图文件名
 * @param width 缩略图宽度
 * @param height 缩略图高度
 * @return int 0表示成功，非0表示失败
 */
int media_manager_generate_thumbnail(media_manager_handle_t handle, 
                                    const char *filename, 
                                    const char *thumb_filename, 
                                    uint32_t width, 
                                    uint32_t height);

/**
 * @brief 设置媒体文件属性
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param attr_name 属性名
 * @param attr_value 属性值
 * @return int 0表示成功，非0表示失败
 */
int media_manager_set_attribute(media_manager_handle_t handle, 
                               const char *filename, 
                               const char *attr_name, 
                               const char *attr_value);

/**
 * @brief 获取媒体文件属性
 * 
 * @param handle 管理器句柄
 * @param filename 文件名
 * @param attr_name 属性名
 * @param attr_value 属性值缓冲区
 * @param value_size 缓冲区大小
 * @return int 0表示成功，非0表示失败
 */
int media_manager_get_attribute(media_manager_handle_t handle, 
                               const char *filename, 
                               const char *attr_name, 
                               char *attr_value, 
                               size_t value_size);

#endif /* MEDIA_MANAGER_API_H */
