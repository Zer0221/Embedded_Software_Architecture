/**
 * @file esp32_flash.c
 * @brief ESP32平台Flash驱动实现
 *
 * 该文件实现了ESP32平台的Flash驱动接口
 */

#include "base/flash_api.h"
#include "common/error_api.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <string.h>

/* ESP32 Flash相关定义 */
#define FLASH_BASE_ADDRESS     0x00000000UL
#define FLASH_SECTOR_SIZE      4096            /* 4KB */
#define FLASH_BLOCK_SIZE       65536           /* 64KB */
#define FLASH_TIMEOUT          5000

/* ESP32 Flash设备结构�?*/
typedef struct {
    flash_callback_t callback;        /* 回调函数 */
    void *arg;                        /* 回调参数 */
    volatile flash_status_t status;   /* 操作状�?*/
    bool initialized;                 /* 初始化标�?*/
} esp32_flash_device_t;

/* Flash设备 */
static esp32_flash_device_t g_flash_device;

/**
 * @brief 初始化Flash设备
 * 
 * @param callback Flash操作完成回调函数
 * @param arg 传递给回调函数的参�?
 * @param handle Flash设备句柄指针
 * @return int 0表示成功，非0表示失败
 */
int flash_init(flash_callback_t callback, void *arg, flash_handle_t *handle)
{
    /* 参数检�?*/
    if (handle == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始�?*/
    if (g_flash_device.initialized) {
        return ERROR_ALREADY_INITIALIZED;
    }
    
    /* 初始化ESP32 SPI Flash */
    esp_err_t err = spi_flash_init();
    if (err != ESP_OK) {
        return ERROR_DRIVER_INIT_FAILED;
    }
    
    /* 初始化Flash设备 */
    memset(&g_flash_device, 0, sizeof(esp32_flash_device_t));
    g_flash_device.callback = callback;
    g_flash_device.arg = arg;
    g_flash_device.status = FLASH_STATUS_IDLE;
    g_flash_device.initialized = true;
    
    /* 返回句柄 */
    *handle = (flash_handle_t)&g_flash_device;
    
    return DRIVER_OK;
}

/**
 * @brief 去初始化Flash设备
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_deinit(flash_handle_t handle)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 清除初始化标�?*/
    dev->initialized = false;
    
    return DRIVER_OK;
}

/**
 * @brief 读取Flash数据
 * 
 * @param handle Flash设备句柄
 * @param address 起始地址
 * @param data 数据缓冲�?
 * @param size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int flash_read(flash_handle_t handle, uint32_t address, void *data, uint32_t size)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 读取数据 */
    err = spi_flash_read(address, data, size);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = FLASH_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_READ_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, FLASH_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 写入Flash数据
 * 
 * @param handle Flash设备句柄
 * @param address 起始地址
 * @param data 数据缓冲�?
 * @param size 数据大小(字节)
 * @return int 0表示成功，非0表示失败
 */
int flash_write(flash_handle_t handle, uint32_t address, const void *data, uint32_t size)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 写入数据 */
    err = spi_flash_write(address, data, size);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = FLASH_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_WRITE_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, FLASH_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 擦除Flash扇区
 * 
 * @param handle Flash设备句柄
 * @param sector_address 扇区地址
 * @return int 0表示成功，非0表示失败
 */
int flash_erase_sector(flash_handle_t handle, uint32_t sector_address)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 扇区地址对齐检�?*/
    if ((sector_address % FLASH_SECTOR_SIZE) != 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 擦除扇区 */
    err = spi_flash_erase_sector(sector_address / FLASH_SECTOR_SIZE);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = FLASH_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_ERASE_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, FLASH_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 擦除Flash�?
 * 
 * @param handle Flash设备句柄
 * @param block_address 块地址
 * @return int 0表示成功，非0表示失败
 */
int flash_erase_block(flash_handle_t handle, uint32_t block_address)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    esp_err_t err;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 块地址对齐检�?*/
    if ((block_address % FLASH_BLOCK_SIZE) != 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 擦除�?*/
    err = spi_flash_erase_range(block_address, FLASH_BLOCK_SIZE);
    
    if (err != ESP_OK) {
        /* 更新状�?*/
        dev->status = FLASH_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_ERASE_FAILED;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_COMPLETE;
    
    /* 调用回调函数 */
    if (dev->callback) {
        dev->callback(dev->arg, FLASH_STATUS_COMPLETE);
    }
    
    return DRIVER_OK;
}

/**
 * @brief 获取Flash状�?
 * 
 * @param handle Flash设备句柄
 * @param status Flash状态指�?
 * @return int 0表示成功，非0表示失败
 */
int flash_get_status(flash_handle_t handle, flash_status_t *status)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || status == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取状�?*/
    *status = dev->status;
    
    return DRIVER_OK;
}

/**
 * @brief 获取Flash扇区大小
 * 
 * @param handle Flash设备句柄
 * @param sector_size 扇区大小指针
 * @return int 0表示成功，非0表示失败
 */
int flash_get_sector_size(flash_handle_t handle, uint32_t *sector_size)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || sector_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 返回扇区大小 */
    *sector_size = FLASH_SECTOR_SIZE;
    
    return DRIVER_OK;
}

/**
 * @brief 获取Flash块大�?
 * 
 * @param handle Flash设备句柄
 * @param block_size 块大小指�?
 * @return int 0表示成功，非0表示失败
 */
int flash_get_block_size(flash_handle_t handle, uint32_t *block_size)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || block_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 返回块大�?*/
    *block_size = FLASH_BLOCK_SIZE;
    
    return DRIVER_OK;
}

/**
 * @brief 获取Flash总大�?
 * 
 * @param handle Flash设备句柄
 * @param total_size Flash总大小指�?
 * @return int 0表示成功，非0表示失败
 */
int flash_get_total_size(flash_handle_t handle, uint32_t *total_size)
{
    esp32_flash_device_t *dev = (esp32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || total_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取Flash总大�?*/
    *total_size = spi_flash_get_chip_size();
    
    return DRIVER_OK;
}

/**
 * @brief 设置Flash保护
 * 
 * @param handle Flash设备句柄
 * @param start_address 起始地址
 * @param end_address 结束地址
 * @param enable 是否启用保护
 * @return int 0表示成功，非0表示失败
 */
int flash_set_protection(flash_handle_t handle, uint32_t start_address, uint32_t end_address, bool enable)
{
    /* ESP32 SPI Flash API不支持地址范围保护，返回不支持 */
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 获取Flash保护状�?
 * 
 * @param handle Flash设备句柄
 * @param address 地址
 * @param is_protected 保护状态指�?
 * @return int 0表示成功，非0表示失败
 */
int flash_get_protection(flash_handle_t handle, uint32_t address, bool *is_protected)
{
    /* ESP32 SPI Flash API不支持获取保护状态，返回不支�?*/
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief 锁定Flash
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_lock(flash_handle_t handle)
{
    /* ESP32 SPI Flash API内部处理锁定，这里返回成�?*/
    return DRIVER_OK;
}

/**
 * @brief 解锁Flash
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_unlock(flash_handle_t handle)
{
    /* ESP32 SPI Flash API内部处理解锁，这里返回成�?*/
    return DRIVER_OK;
}

