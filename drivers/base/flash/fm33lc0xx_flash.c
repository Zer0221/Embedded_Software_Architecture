/**
 * @file fm33lc0xx_flash.c
 * @brief FM33LC0xx平台Flash驱动实现
 *
 * 该文件实现了FM33LC0xx平台的Flash驱动接口
 */

#include "base/flash_api.h"
#include "common/error_api.h"
#include "fm33lc0xx_fl.h"
#include <string.h>

/* FM33LC0xx Flash相关定义 */
#define FLASH_BASE_ADDRESS     0x00000000UL
#define FLASH_TOTAL_SIZE       (128 * 1024)   /* 128KB */
#define FLASH_SECTOR_SIZE      512             /* 512B */
#define FLASH_PAGE_SIZE        512             /* 512B */
#define FLASH_TIMEOUT          1000

/* FM33LC0xx Flash设备结构�?*/
typedef struct {
    flash_callback_t callback;        /* 回调函数 */
    void *arg;                        /* 回调参数 */
    volatile flash_status_t status;   /* 操作状�?*/
    bool initialized;                 /* 初始化标�?*/
} fm33lc0xx_flash_device_t;

/* Flash设备 */
static fm33lc0xx_flash_device_t g_flash_device;

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
    
    /* 初始化Flash设备 */
    memset(&g_flash_device, 0, sizeof(fm33lc0xx_flash_device_t));
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    uint8_t *dst = (uint8_t *)data;
    uint8_t *src = (uint8_t *)address;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (address < FLASH_BASE_ADDRESS || (address + size) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 直接读取数据 */
    memcpy(dst, src, size);
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    const uint8_t *src = (const uint8_t *)data;
    uint32_t addr = address;
    uint32_t remaining = size;
    uint32_t write_size;
    FlagStatus status;
    ErrorStatus flash_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (address < FLASH_BASE_ADDRESS || (address + size) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 解锁Flash */
    FL_FLASH_Unlock();
    
    /* 按字节写�?*/
    while (remaining > 0) {
        /* 确定写入大小 */
        write_size = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;
        
        /* 写入数据 */
        for (uint32_t i = 0; i < write_size; i++) {
            flash_status = FL_FLASH_Program_Byte(addr + i, src[i]);
            if (flash_status != FL_PASS) {
                /* 锁定Flash */
                FL_FLASH_Lock();
                
                /* 更新状�?*/
                dev->status = FLASH_STATUS_ERROR;
                
                /* 调用回调函数 */
                if (dev->callback) {
                    dev->callback(dev->arg, FLASH_STATUS_ERROR);
                }
                
                return ERROR_DRIVER_WRITE_FAILED;
            }
            
            /* 等待写入完成 */
            uint32_t timeout = FLASH_TIMEOUT;
            do {
                status = FL_FLASH_GetFlag(FL_FLASH_FLAG_BUSY);
                timeout--;
            } while (status == SET && timeout > 0);
            
            if (timeout == 0) {
                /* 锁定Flash */
                FL_FLASH_Lock();
                
                /* 更新状�?*/
                dev->status = FLASH_STATUS_TIMEOUT;
                
                /* 调用回调函数 */
                if (dev->callback) {
                    dev->callback(dev->arg, FLASH_STATUS_TIMEOUT);
                }
                
                return ERROR_DRIVER_TIMEOUT;
            }
        }
        
        /* 更新地址和剩余大�?*/
        addr += write_size;
        src += write_size;
        remaining -= write_size;
    }
    
    /* 锁定Flash */
    FL_FLASH_Lock();
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    uint32_t sector;
    ErrorStatus flash_status;
    FlagStatus status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (sector_address < FLASH_BASE_ADDRESS || sector_address >= (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 扇区地址对齐检�?*/
    if ((sector_address % FLASH_SECTOR_SIZE) != 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 计算扇区�?*/
    sector = (sector_address - FLASH_BASE_ADDRESS) / FLASH_SECTOR_SIZE;
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 解锁Flash */
    FL_FLASH_Unlock();
    
    /* 擦除扇区 */
    flash_status = FL_FLASH_SectorErase(sector * FLASH_SECTOR_SIZE);
    
    if (flash_status != FL_PASS) {
        /* 锁定Flash */
        FL_FLASH_Lock();
        
        /* 更新状�?*/
        dev->status = FLASH_STATUS_ERROR;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_ERROR);
        }
        
        return ERROR_DRIVER_ERASE_FAILED;
    }
    
    /* 等待擦除完成 */
    uint32_t timeout = FLASH_TIMEOUT;
    do {
        status = FL_FLASH_GetFlag(FL_FLASH_FLAG_BUSY);
        timeout--;
    } while (status == SET && timeout > 0);
    
    /* 锁定Flash */
    FL_FLASH_Lock();
    
    if (timeout == 0) {
        /* 更新状�?*/
        dev->status = FLASH_STATUS_TIMEOUT;
        
        /* 调用回调函数 */
        if (dev->callback) {
            dev->callback(dev->arg, FLASH_STATUS_TIMEOUT);
        }
        
        return ERROR_DRIVER_TIMEOUT;
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    uint32_t block_size = 4 * 1024; /* 4KB */
    uint32_t sector_count = block_size / FLASH_SECTOR_SIZE;
    uint32_t sector_address = block_address;
    int result;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (block_address < FLASH_BASE_ADDRESS || (block_address + block_size) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 块地址对齐检�?*/
    if ((block_address % block_size) != 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 擦除所有扇�?*/
    for (uint32_t i = 0; i < sector_count; i++) {
        result = flash_erase_sector(handle, sector_address);
        if (result != DRIVER_OK) {
            return result;
        }
        sector_address += FLASH_SECTOR_SIZE;
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || block_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 返回块大�?(4KB) */
    *block_size = 4 * 1024;
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || total_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 返回Flash总大�?*/
    *total_size = FLASH_TOTAL_SIZE;
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (start_address < FLASH_BASE_ADDRESS || end_address > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE) || start_address > end_address) {
        return ERROR_INVALID_PARAM;
    }
    
    /* FM33LC0xx不支持精确的地址范围保护，只能保护整个Flash */
    if (enable) {
        /* 启用写保�?*/
        FL_FLASH_EnableWriteProtection();
    } else {
        /* 禁用写保�?*/
        FL_FLASH_DisableWriteProtection();
    }
    
    return DRIVER_OK;
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || is_protected == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (address < FLASH_BASE_ADDRESS || address >= (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取写保护状�?*/
    *is_protected = (FL_FLASH_IsEnabledWriteProtection() == FL_TRUE) ? true : false;
    
    return DRIVER_OK;
}

/**
 * @brief 锁定Flash
 * 
 * @param handle Flash设备句柄
 * @return int 0表示成功，非0表示失败
 */
int flash_lock(flash_handle_t handle)
{
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 锁定Flash */
    FL_FLASH_Lock();
    
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
    fm33lc0xx_flash_device_t *dev = (fm33lc0xx_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 解锁Flash */
    FL_FLASH_Unlock();
    
    return DRIVER_OK;
}

