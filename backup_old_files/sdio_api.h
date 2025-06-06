/**
 * @file sdio_api.h
 * @brief SDIO接口抽象层定义
 *
 * 该头文件定义了SDIO的统一抽象接口，使上层应用能够与底层SDIO硬件实现解耦
 */

#ifndef SDIO_API_H
#define SDIO_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* SDIO卡类型 */
typedef enum {
    SDIO_CARD_TYPE_UNKNOWN,    /**< 未知卡类型 */
    SDIO_CARD_TYPE_SD,         /**< SD卡 */
    SDIO_CARD_TYPE_SDHC,       /**< SDHC卡 */
    SDIO_CARD_TYPE_SDXC,       /**< SDXC卡 */
    SDIO_CARD_TYPE_MMC,        /**< MMC卡 */
    SDIO_CARD_TYPE_EMMC        /**< eMMC卡 */
} sdio_card_type_t;

/* SDIO总线宽度 */
typedef enum {
    SDIO_BUS_WIDTH_1BIT,      /**< 1位总线宽度 */
    SDIO_BUS_WIDTH_4BIT,      /**< 4位总线宽度 */
    SDIO_BUS_WIDTH_8BIT       /**< 8位总线宽度 */
} sdio_bus_width_t;

/* SDIO时钟频率 */
typedef enum {
    SDIO_FREQ_DEFAULT,        /**< 默认频率 */
    SDIO_FREQ_HIGH_SPEED,     /**< 高速模式 */
    SDIO_FREQ_SDR12,          /**< SDR12模式 */
    SDIO_FREQ_SDR25,          /**< SDR25模式 */
    SDIO_FREQ_SDR50,          /**< SDR50模式 */
    SDIO_FREQ_SDR104,         /**< SDR104模式 */
    SDIO_FREQ_DDR50           /**< DDR50模式 */
} sdio_freq_mode_t;

/* SDIO操作状态 */
typedef enum {
    SDIO_STATUS_IDLE,         /**< 空闲状态 */
    SDIO_STATUS_BUSY,         /**< 忙状态 */
    SDIO_STATUS_COMPLETE,     /**< 操作完成 */
    SDIO_STATUS_ERROR,        /**< 操作错误 */
    SDIO_STATUS_TIMEOUT,      /**< 操作超时 */
    SDIO_STATUS_NO_CARD       /**< 无卡 */
} sdio_status_t;

/* SDIO配置参数 */
typedef struct {
    sdio_bus_width_t bus_width;    /**< 总线宽度 */
    sdio_freq_mode_t freq_mode;    /**< 频率模式 */
    bool enable_4bit;              /**< 是否启用4位模式 */
    bool enable_high_speed;        /**< 是否启用高速模式 */
    bool enable_dma;               /**< 是否启用DMA */
} sdio_config_t;

/* SDIO卡信息 */
typedef struct {
    sdio_card_type_t card_type;        /**< 卡类型 */
    uint32_t card_capacity;            /**< 卡容量(字节) */
    uint32_t block_size;               /**< 块大小(字节) */
    uint32_t block_count;              /**< 块数量 */
    char card_name[16];                /**< 卡名称 */
    char manufacturer_id[8];           /**< 制造商ID */
    char product_name[32];             /**< 产品名称 */
    char serial_number[32];            /**< 序列号 */
    uint8_t manufacturing_date[2];     /**< 生产日期 */
} sdio_card_info_t;

/* SDIO回调函数类型 */
typedef void (*sdio_callback_t)(void *arg, sdio_status_t status);

/* SDIO设备句柄 */
typedef driver_handle_t sdio_handle_t;

/**
 * @brief 初始化SDIO设备
 * 
 * @param config SDIO配置参数
 * @param callback SDIO操作完成回调函数
 * @param arg 传递给回调函数的参数
 * @param handle SDIO设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_init(const sdio_config_t *config, sdio_callback_t callback, void *arg, sdio_handle_t *handle);

/**
 * @brief 去初始化SDIO设备
 * 
 * @param handle SDIO设备句柄
 * @return int 0表示成功，非0表示失败
 */
int sdio_deinit(sdio_handle_t handle);

/**
 * @brief 检测SD卡
 * 
 * @param handle SDIO设备句柄
 * @param card_inserted 卡插入状态指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_detect_card(sdio_handle_t handle, bool *card_inserted);

/**
 * @brief 获取SD卡信息
 * 
 * @param handle SDIO设备句柄
 * @param card_info 卡信息指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_card_info(sdio_handle_t handle, sdio_card_info_t *card_info);

/**
 * @brief 读取SD卡数据
 * 
 * @param handle SDIO设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲区
 * @param block_count 块数量
 * @return int 0表示成功，非0表示失败
 */
int sdio_read_blocks(sdio_handle_t handle, uint32_t block_addr, void *data, uint32_t block_count);

/**
 * @brief 写入SD卡数据
 * 
 * @param handle SDIO设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲区
 * @param block_count 块数量
 * @return int 0表示成功，非0表示失败
 */
int sdio_write_blocks(sdio_handle_t handle, uint32_t block_addr, const void *data, uint32_t block_count);

/**
 * @brief 擦除SD卡数据
 * 
 * @param handle SDIO设备句柄
 * @param start_block 起始块地址
 * @param end_block 结束块地址
 * @return int 0表示成功，非0表示失败
 */
int sdio_erase_blocks(sdio_handle_t handle, uint32_t start_block, uint32_t end_block);

/**
 * @brief 获取SDIO操作状态
 * 
 * @param handle SDIO设备句柄
 * @param status 状态指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_status(sdio_handle_t handle, sdio_status_t *status);

/**
 * @brief 设置SDIO总线宽度
 * 
 * @param handle SDIO设备句柄
 * @param bus_width 总线宽度
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_bus_width(sdio_handle_t handle, sdio_bus_width_t bus_width);

/**
 * @brief 设置SDIO频率模式
 * 
 * @param handle SDIO设备句柄
 * @param freq_mode 频率模式
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_freq_mode(sdio_handle_t handle, sdio_freq_mode_t freq_mode);

/**
 * @brief 使能SDIO写保护检测
 * 
 * @param handle SDIO设备句柄
 * @param enable 是否启用
 * @return int 0表示成功，非0表示失败
 */
int sdio_enable_write_protect(sdio_handle_t handle, bool enable);

/**
 * @brief 获取SDIO写保护状态
 * 
 * @param handle SDIO设备句柄
 * @param is_protected 保护状态指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_write_protect(sdio_handle_t handle, bool *is_protected);

/**
 * @brief 获取SDIO块大小
 * 
 * @param handle SDIO设备句柄
 * @param block_size 块大小指针
 * @return int 0表示成功，非0表示失败
 */
int sdio_get_block_size(sdio_handle_t handle, uint32_t *block_size);

/**
 * @brief 设置SDIO块大小
 * 
 * @param handle SDIO设备句柄
 * @param block_size 块大小
 * @return int 0表示成功，非0表示失败
 */
int sdio_set_block_size(sdio_handle_t handle, uint32_t block_size);

/**
 * @brief 执行SDIO命令
 * 
 * @param handle SDIO设备句柄
 * @param cmd 命令
 * @param arg 参数
 * @param resp 响应缓冲区
 * @param resp_len 响应长度
 * @return int 0表示成功，非0表示失败
 */
int sdio_execute_command(sdio_handle_t handle, uint8_t cmd, uint32_t arg, uint32_t *resp, uint32_t resp_len);

#endif /* SDIO_API_H */
