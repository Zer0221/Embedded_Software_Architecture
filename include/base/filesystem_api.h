/**
 * @file filesystem_api.h
 * @brief 文件系统抽象接口定义
 *
 * 该头文件定义了文件系统的统一抽象接口，支持多种文件系统类型
 * 如SPIFFS、LittleFS、FAT等，为应用层提供统一的文件操作接口
 */

#ifndef FILESYSTEM_API_H
#define FILESYSTEM_API_H

#include <stdint.h>
#include <stdbool.h>
#include "common/project_config.h"
#include "common/driver_api.h"
#include "common/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 文件系统类型 */
typedef enum {
    FS_TYPE_SPIFFS,       /**< SPIFFS文件系统 */
    FS_TYPE_LITTLEFS,     /**< LittleFS文件系统 */
    FS_TYPE_FATFS,        /**< FAT文件系统 */
    FS_TYPE_ROMFS,        /**< ROM文件系统 */
    FS_TYPE_NFS,          /**< 网络文件系统 */
    FS_TYPE_CUSTOM        /**< 自定义文件系统 */
} fs_type_t;

/* 文件系统挂载点 */
typedef struct {
    char path[64];         /**< 挂载路径 */
    fs_type_t type;        /**< 文件系统类型 */
    const char* device;    /**< 设备名或分区名 */
    bool format_if_empty;  /**< 如果为空是否格式化 */
    void* fs_config;       /**< 文件系统特定配置 */
} fs_mount_config_t;

/* 文件打开模式 */
typedef enum {
    FS_MODE_READ = 0x01,           /**< 只读模式 */
    FS_MODE_WRITE = 0x02,          /**< 只写模式 */
    FS_MODE_APPEND = 0x04,         /**< 追加模式 */
    FS_MODE_CREATE = 0x08,         /**< 如不存在则创建 */
    FS_MODE_TRUNCATE = 0x10,       /**< 清空文件内容 */
    FS_MODE_BINARY = 0x20,         /**< 二进制模式 */
    FS_MODE_TEXT = 0x40            /**< 文本模式 */
} fs_open_mode_t;

/* 文件定位起点 */
typedef enum {
    FS_SEEK_SET = 0,               /**< 文件开头 */
    FS_SEEK_CUR,                   /**< 当前位置 */
    FS_SEEK_END                    /**< 文件末尾 */
} fs_seek_mode_t;

/* 文件信息 */
typedef struct {
    char name[256];                /**< 文件名 */
    uint32_t size;                 /**< 文件大小(字节) */
    uint32_t creation_time;        /**< 创建时间(Unix时间戳) */
    uint32_t last_access_time;     /**< 最后访问时间(Unix时间戳) */
    uint32_t last_modified_time;   /**< 最后修改时间(Unix时间戳) */
    bool is_directory;             /**< 是否是目录 */
    bool is_read_only;             /**< 是否只读 */
    bool is_hidden;                /**< 是否隐藏 */
    bool is_system;                /**< 是否系统文件 */
} fs_file_info_t;

/* 文件系统信息 */
typedef struct {
    uint32_t total_size;           /**< 总大小(字节) */
    uint32_t used_size;            /**< 已用大小(字节) */
    uint32_t free_size;            /**< 可用大小(字节) */
    uint32_t block_size;           /**< 块大小(字节) */
    uint32_t max_filename_len;     /**< 最大文件名长度 */
    fs_type_t type;                /**< 文件系统类型 */
    char label[16];                /**< 卷标 */
} fs_info_t;

/* 文件系统句柄 */
typedef driver_handle_t fs_handle_t;

/* 文件句柄 */
typedef driver_handle_t fs_file_handle_t;

/* 目录句柄 */
typedef driver_handle_t fs_dir_handle_t;

/**
 * @brief 初始化文件系统模块
 * 
 * @return int 成功返回0，失败返回错误码
 */
int fs_init(void);

/**
 * @brief 反初始化文件系统模块
 * 
 * @return int 成功返回0，失败返回错误码
 */
int fs_deinit(void);

/**
 * @brief 挂载文件系统
 * 
 * @param config 挂载配置
 * @param handle 返回的文件系统句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_mount(const fs_mount_config_t* config, fs_handle_t* handle);

/**
 * @brief 卸载文件系统
 * 
 * @param handle 文件系统句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_unmount(fs_handle_t handle);

/**
 * @brief 获取文件系统信息
 * 
 * @param handle 文件系统句柄
 * @param info 返回的文件系统信息
 * @return int 成功返回0，失败返回错误码
 */
int fs_get_info(fs_handle_t handle, fs_info_t* info);

/**
 * @brief 打开文件
 * 
 * @param handle 文件系统句柄
 * @param path 文件路径
 * @param mode 打开模式
 * @param file 返回的文件句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_open(fs_handle_t handle, const char* path, uint8_t mode, fs_file_handle_t* file);

/**
 * @brief 关闭文件
 * 
 * @param file 文件句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_close(fs_file_handle_t file);

/**
 * @brief 读取文件
 * 
 * @param file 文件句柄
 * @param buffer 读取缓冲区
 * @param size 要读取的字节数
 * @param bytes_read 实际读取的字节数
 * @return int 成功返回0，失败返回错误码
 */
int fs_read(fs_file_handle_t file, void* buffer, uint32_t size, uint32_t* bytes_read);

/**
 * @brief 写入文件
 * 
 * @param file 文件句柄
 * @param buffer 写入数据
 * @param size 要写入的字节数
 * @param bytes_written 实际写入的字节数
 * @return int 成功返回0，失败返回错误码
 */
int fs_write(fs_file_handle_t file, const void* buffer, uint32_t size, uint32_t* bytes_written);

/**
 * @brief 设置文件位置
 * 
 * @param file 文件句柄
 * @param offset 偏移量
 * @param mode 定位模式
 * @return int 成功返回0，失败返回错误码
 */
int fs_seek(fs_file_handle_t file, int32_t offset, fs_seek_mode_t mode);

/**
 * @brief 获取文件当前位置
 * 
 * @param file 文件句柄
 * @param position 返回的文件位置
 * @return int 成功返回0，失败返回错误码
 */
int fs_tell(fs_file_handle_t file, uint32_t* position);

/**
 * @brief 刷新文件缓冲区
 * 
 * @param file 文件句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_flush(fs_file_handle_t file);

/**
 * @brief 获取文件信息
 * 
 * @param handle 文件系统句柄
 * @param path 文件路径
 * @param info 返回的文件信息
 * @return int 成功返回0，失败返回错误码
 */
int fs_stat(fs_handle_t handle, const char* path, fs_file_info_t* info);

/**
 * @brief 删除文件
 * 
 * @param handle 文件系统句柄
 * @param path 文件路径
 * @return int 成功返回0，失败返回错误码
 */
int fs_remove(fs_handle_t handle, const char* path);

/**
 * @brief 重命名文件
 * 
 * @param handle 文件系统句柄
 * @param old_path 原文件路径
 * @param new_path 新文件路径
 * @return int 成功返回0，失败返回错误码
 */
int fs_rename(fs_handle_t handle, const char* old_path, const char* new_path);

/**
 * @brief 创建目录
 * 
 * @param handle 文件系统句柄
 * @param path 目录路径
 * @return int 成功返回0，失败返回错误码
 */
int fs_mkdir(fs_handle_t handle, const char* path);

/**
 * @brief 删除目录
 * 
 * @param handle 文件系统句柄
 * @param path 目录路径
 * @return int 成功返回0，失败返回错误码
 */
int fs_rmdir(fs_handle_t handle, const char* path);

/**
 * @brief 打开目录
 * 
 * @param handle 文件系统句柄
 * @param path 目录路径
 * @param dir 返回的目录句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_opendir(fs_handle_t handle, const char* path, fs_dir_handle_t* dir);

/**
 * @brief 关闭目录
 * 
 * @param dir 目录句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_closedir(fs_dir_handle_t dir);

/**
 * @brief 读取目录项
 * 
 * @param dir 目录句柄
 * @param info 返回的文件信息
 * @return int 成功返回0，目录结束返回1，失败返回错误码
 */
int fs_readdir(fs_dir_handle_t dir, fs_file_info_t* info);

/**
 * @brief 重置目录读取位置
 * 
 * @param dir 目录句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_rewinddir(fs_dir_handle_t dir);

/**
 * @brief 格式化文件系统
 * 
 * @param handle 文件系统句柄
 * @return int 成功返回0，失败返回错误码
 */
int fs_format(fs_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* FILESYSTEM_API_H */