/**
 * @file stm32_flash.c
 * @brief STM32平台Flash驱动实现
 *
 * 该文件实现了STM32平台的Flash驱动接口
 */

#include "base/flash_api.h"
#include "common/error_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* STM32 Flash扇区大小定义 */
#define FLASH_SECTOR_0_SIZE    (16 * 1024)   /* 16KB */
#define FLASH_SECTOR_1_SIZE    (16 * 1024)   /* 16KB */
#define FLASH_SECTOR_2_SIZE    (16 * 1024)   /* 16KB */
#define FLASH_SECTOR_3_SIZE    (16 * 1024)   /* 16KB */
#define FLASH_SECTOR_4_SIZE    (64 * 1024)   /* 64KB */
#define FLASH_SECTOR_5_SIZE    (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_6_SIZE    (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_7_SIZE    (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_8_SIZE    (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_9_SIZE    (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_10_SIZE   (128 * 1024)  /* 128KB */
#define FLASH_SECTOR_11_SIZE   (128 * 1024)  /* 128KB */

/* STM32 Flash基础地址 */
#define FLASH_BASE_ADDRESS     0x08000000UL

/* STM32 Flash总大�?(1MB) */
#define FLASH_TOTAL_SIZE       (1024 * 1024)

/* STM32 Flash超时时间 (ms) */
#define FLASH_TIMEOUT          5000

/* STM32 Flash扇区数量 */
#define FLASH_SECTOR_COUNT     12

/* 扇区信息结构�?*/
typedef struct {
    uint32_t start_address;
    uint32_t size;
    uint8_t sector_number;
} flash_sector_info_t;

/* 扇区信息�?*/
static const flash_sector_info_t g_sector_info[] = {
    {FLASH_BASE_ADDRESS, FLASH_SECTOR_0_SIZE, FLASH_SECTOR_0},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE, FLASH_SECTOR_1_SIZE, FLASH_SECTOR_1},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE, FLASH_SECTOR_2_SIZE, FLASH_SECTOR_2},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE, FLASH_SECTOR_3_SIZE, FLASH_SECTOR_3},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE, FLASH_SECTOR_4_SIZE, FLASH_SECTOR_4},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE, FLASH_SECTOR_5_SIZE, FLASH_SECTOR_5},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE, FLASH_SECTOR_6_SIZE, FLASH_SECTOR_6},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE + FLASH_SECTOR_6_SIZE, FLASH_SECTOR_7_SIZE, FLASH_SECTOR_7},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE + FLASH_SECTOR_6_SIZE + FLASH_SECTOR_7_SIZE, FLASH_SECTOR_8_SIZE, FLASH_SECTOR_8},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE + FLASH_SECTOR_6_SIZE + FLASH_SECTOR_7_SIZE + FLASH_SECTOR_8_SIZE, FLASH_SECTOR_9_SIZE, FLASH_SECTOR_9},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE + FLASH_SECTOR_6_SIZE + FLASH_SECTOR_7_SIZE + FLASH_SECTOR_8_SIZE + FLASH_SECTOR_9_SIZE, FLASH_SECTOR_10_SIZE, FLASH_SECTOR_10},
    {FLASH_BASE_ADDRESS + FLASH_SECTOR_0_SIZE + FLASH_SECTOR_1_SIZE + FLASH_SECTOR_2_SIZE + FLASH_SECTOR_3_SIZE + FLASH_SECTOR_4_SIZE + FLASH_SECTOR_5_SIZE + FLASH_SECTOR_6_SIZE + FLASH_SECTOR_7_SIZE + FLASH_SECTOR_8_SIZE + FLASH_SECTOR_9_SIZE + FLASH_SECTOR_10_SIZE, FLASH_SECTOR_11_SIZE, FLASH_SECTOR_11},
};

/* STM32 Flash设备结构�?*/
typedef struct {
    flash_callback_t callback;        /* 回调函数 */
    void *arg;                        /* 回调参数 */
    volatile flash_status_t status;   /* 操作状�?*/
    bool initialized;                 /* 初始化标�?*/
} stm32_flash_device_t;

/* Flash设备 */
static stm32_flash_device_t g_flash_device;

/* 获取地址所在的扇区�?*/
static int get_sector_from_address(uint32_t address, uint8_t *sector)
{
    for (int i = 0; i < FLASH_SECTOR_COUNT; i++) {
        if (address >= g_sector_info[i].start_address && 
            address < (g_sector_info[i].start_address + g_sector_info[i].size)) {
            *sector = g_sector_info[i].sector_number;
            return DRIVER_OK;
        }
    }
    
    return ERROR_INVALID_PARAM;
}

/* 地址对齐检�?*/
static bool is_address_aligned(uint32_t address)
{
    /* STM32F4地址需要按4字节对齐 */
    return ((address & 0x3) == 0);
}

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
    memset(&g_flash_device, 0, sizeof(stm32_flash_device_t));
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    uint32_t *src = (uint32_t *)data;
    uint32_t end_address = address + size;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || data == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (address < FLASH_BASE_ADDRESS || end_address > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址对齐检�?*/
    if (!is_address_aligned(address) || (size % 4) != 0) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 写入数据 */
    for (uint32_t i = 0; i < size / 4; i++) {
        hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + (i * 4), src[i]);
        
        if (hal_status != HAL_OK) {
            /* 锁定Flash */
            HAL_FLASH_Lock();
            
            /* 更新状�?*/
            dev->status = FLASH_STATUS_ERROR;
            
            /* 调用回调函数 */
            if (dev->callback) {
                dev->callback(dev->arg, FLASH_STATUS_ERROR);
            }
            
            return ERROR_DRIVER_WRITE_FAILED;
        }
    }
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error = 0;
    uint8_t sector;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (sector_address < FLASH_BASE_ADDRESS || sector_address >= (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取扇区�?*/
    if (get_sector_from_address(sector_address, &sector) != DRIVER_OK) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 更新状�?*/
    dev->status = FLASH_STATUS_BUSY;
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 擦除扇区 */
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = sector;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    hal_status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    if (hal_status != HAL_OK) {
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
    /* STM32 F4不区分扇区和块，使用扇区擦除 */
    return flash_erase_sector(handle, block_address);
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || sector_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* STM32 F4的扇区大小不固定，返回最小的扇区大小 */
    *sector_size = FLASH_SECTOR_0_SIZE;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || block_size == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* STM32 F4不区分扇区和块，返回最大的扇区大小 */
    *block_size = FLASH_SECTOR_11_SIZE;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    uint8_t start_sector, end_sector;
    FLASH_OBProgramInitTypeDef ob_init;
    HAL_StatusTypeDef hal_status;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (start_address < FLASH_BASE_ADDRESS || end_address > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE) || start_address > end_address) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取起始和结束扇�?*/
    if (get_sector_from_address(start_address, &start_sector) != DRIVER_OK ||
        get_sector_from_address(end_address, &end_sector) != DRIVER_OK) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 解锁Flash和Option Bytes */
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    
    /* 配置Option Bytes */
    HAL_FLASHEx_OBGetConfig(&ob_init);
    
    if (enable) {
        /* 启用写保�?*/
        for (uint8_t sector = start_sector; sector <= end_sector; sector++) {
            ob_init.WRPSector |= (1 << sector);
        }
    } else {
        /* 禁用写保�?*/
        for (uint8_t sector = start_sector; sector <= end_sector; sector++) {
            ob_init.WRPSector &= ~(1 << sector);
        }
    }
    
    /* 编程Option Bytes */
    hal_status = HAL_FLASHEx_OBProgram(&ob_init);
    
    /* 锁定Option Bytes和Flash */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    
    if (hal_status != HAL_OK) {
        return ERROR_DRIVER_OPERATION_FAILED;
    }
    
    /* 启动Option Bytes加载 */
    HAL_FLASH_OB_Launch();
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    FLASH_OBProgramInitTypeDef ob_init;
    uint8_t sector;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized || is_protected == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 地址范围检�?*/
    if (address < FLASH_BASE_ADDRESS || address >= (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取扇区�?*/
    if (get_sector_from_address(address, &sector) != DRIVER_OK) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 获取Option Bytes配置 */
    HAL_FLASHEx_OBGetConfig(&ob_init);
    
    /* 检查写保护状�?*/
    *is_protected = (ob_init.WRPSector & (1 << sector)) ? true : false;
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
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
    stm32_flash_device_t *dev = (stm32_flash_device_t *)handle;
    
    /* 参数检�?*/
    if (dev == NULL || !dev->initialized) {
        return ERROR_INVALID_PARAM;
    }
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    return DRIVER_OK;
}

