/**
 * @file stm32_security.c
 * @brief STM32平台安全模块实现
 *
 * 该文件实现了STM32平台上的安全模块功能，包括加�?解密、哈希计算、安全启动和固件验证等功�?
 */

#include <string.h>
#include "feature/security_api.h"

/* STM32 HAL 加密库头文件 */
#include "stm32_hal_crypto.h" /* 根据实际STM32平台库头文件调整 */

/* 安全设备上下�?*/
typedef struct {
    uint8_t initialized;
    security_callback_t callback;
    void *callback_arg;
    security_status_t status;
    /* STM32硬件加密引擎句柄 */
    CRYP_HandleTypeDef hcryp;
    /* STM32硬件哈希引擎句柄 */
    HASH_HandleTypeDef hhash;
    /* STM32 PKA (Public Key Accelerator) 句柄 */
    PKA_HandleTypeDef hpka;
    /* 当前操作配置 */
    security_config_t current_config;
} stm32_security_ctx_t;

/* 全局设备上下�?*/
static stm32_security_ctx_t g_security_ctx;

/* 密钥存储�?*/
#define MAX_KEY_STORAGE 10
typedef struct {
    uint8_t used;
    uint32_t id;
    security_key_type_t type;
    uint16_t size;
    uint8_t data[256]; /* 根据需要调整大�?*/
} key_storage_t;

static key_storage_t g_key_storage[MAX_KEY_STORAGE];

/* 加密操作互斥�?*/
static volatile uint8_t g_security_mutex;

/* 获取互斥�?*/
static int security_lock(void)
{
    if (g_security_mutex) {
        return -1;
    }
    g_security_mutex = 1;
    return 0;
}

/* 释放互斥�?*/
static void security_unlock(void)
{
    g_security_mutex = 0;
}

/**
 * @brief 查找密钥存储�?
 * 
 * @param key_id 密钥ID
 * @return int 存储索引�?1表示未找�?
 */
static int find_key_storage(uint32_t key_id)
{
    for (int i = 0; i < MAX_KEY_STORAGE; i++) {
        if (g_key_storage[i].used && g_key_storage[i].id == key_id) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 分配密钥存储�?
 * 
 * @param key_id 密钥ID
 * @return int 存储索引�?1表示无可用空�?
 */
static int alloc_key_storage(uint32_t key_id)
{
    int idx = find_key_storage(key_id);
    if (idx >= 0) {
        return idx; /* 已存�?*/
    }
    
    for (int i = 0; i < MAX_KEY_STORAGE; i++) {
        if (!g_key_storage[i].used) {
            g_key_storage[i].used = 1;
            g_key_storage[i].id = key_id;
            return i;
        }
    }
    return -1; /* 没有可用空间 */
}

/**
 * @brief STM32平台安全模块初始�?
 */
int security_init(security_callback_t callback, void *arg, security_handle_t *handle)
{
    if (g_security_ctx.initialized) {
        *handle = (security_handle_t)&g_security_ctx;
        return 0; /* 已初始化 */
    }
    
    /* 初始化上下文 */
    memset(&g_security_ctx, 0, sizeof(g_security_ctx));
    g_security_ctx.callback = callback;
    g_security_ctx.callback_arg = arg;
    g_security_ctx.status = SECURITY_STATUS_IDLE;
    
    /* 初始化密钥存�?*/
    memset(g_key_storage, 0, sizeof(g_key_storage));
    
    /* 初始化互斥锁 */
    g_security_mutex = 0;
    
    /* 初始化STM32 HAL加密模块 */
    __HAL_RCC_CRYP_CLK_ENABLE();
    __HAL_RCC_HASH_CLK_ENABLE();
    __HAL_RCC_PKA_CLK_ENABLE();
    
    /* 初始化加密引�?*/
    g_security_ctx.hcryp.Instance = CRYP;
    g_security_ctx.hcryp.Init.DataType = CRYP_DATATYPE_8B;
    g_security_ctx.hcryp.Init.KeySize = CRYP_KEYSIZE_128B;
    g_security_ctx.hcryp.Init.pKey = NULL;
    g_security_ctx.hcryp.Init.Algorithm = CRYP_AES_ECB;
    g_security_ctx.hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;
    HAL_CRYP_Init(&g_security_ctx.hcryp);
    
    /* 初始化哈希引�?*/
    g_security_ctx.hhash.Init.DataType = HASH_DATATYPE_8B;
    HAL_HASH_Init(&g_security_ctx.hhash);
    
    /* 初始化PKA */
    HAL_PKA_Init(&g_security_ctx.hpka);
    
    g_security_ctx.initialized = 1;
    *handle = (security_handle_t)&g_security_ctx;
    
    return 0;
}

/**
 * @brief STM32平台安全模块去初始化
 */
int security_deinit(security_handle_t handle)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    /* 确保没有正在进行的操�?*/
    if (ctx->status == SECURITY_STATUS_BUSY) {
        return -1;
    }
    
    /* 释放STM32 HAL加密模块资源 */
    HAL_CRYP_DeInit(&ctx->hcryp);
    HAL_HASH_DeInit(&ctx->hhash);
    HAL_PKA_DeInit(&ctx->hpka);
    
    __HAL_RCC_CRYP_CLK_DISABLE();
    __HAL_RCC_HASH_CLK_DISABLE();
    __HAL_RCC_PKA_CLK_DISABLE();
    
    /* 清空密钥存储 */
    memset(g_key_storage, 0, sizeof(g_key_storage));
    
    /* 清空上下�?*/
    memset(ctx, 0, sizeof(stm32_security_ctx_t));
    
    return 0;
}

/**
 * @brief 生成随机�?
 */
int security_generate_random(security_handle_t handle, uint8_t *buffer, uint32_t len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !buffer || len == 0) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    /* 使用STM32的RNG模块生成随机�?*/
    RNG_HandleTypeDef hrng;
    hrng.Instance = RNG;
    
    HAL_RNG_Init(&hrng);
    
    uint32_t random;
    for (uint32_t i = 0; i < len; i++) {
        if (i % 4 == 0) {
            if (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK) {
                security_unlock();
                return -1;
            }
        }
        buffer[i] = (uint8_t)(random >> ((i % 4) * 8));
    }
    
    HAL_RNG_DeInit(&hrng);
    security_unlock();
    
    return 0;
}

/**
 * @brief 生成密钥
 */
int security_generate_key(security_handle_t handle, security_key_type_t key_type, 
                         security_algo_t algo, uint16_t key_size, 
                         uint8_t *key_buffer, uint32_t *key_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key_buffer || !key_len || 
        *key_len < (key_size / 8)) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    /* 根据算法和密钥类型生成密�?*/
    int result = 0;
    
    switch (algo) {
        case SECURITY_ALGO_AES:
            if (key_type != SECURITY_KEY_SYMMETRIC) {
                result = -1;
                break;
            }
            /* 生成AES密钥 */
            if (security_generate_random(handle, key_buffer, key_size / 8) != 0) {
                result = -1;
            } else {
                *key_len = key_size / 8;
            }
            break;
            
        case SECURITY_ALGO_RSA:
            /* 生成RSA密钥�?- 这里只是简化示�?*/
            if (key_type == SECURITY_KEY_PRIVATE || key_type == SECURITY_KEY_PUBLIC) {
                /* 在实际应用中，应该使用STM32 PKA模块或外部库生成RSA密钥�?*/
                /* 为简化示例，这里只生成随机数�?*/
                if (security_generate_random(handle, key_buffer, key_size / 8) != 0) {
                    result = -1;
                } else {
                    *key_len = key_size / 8;
                }
            } else {
                result = -1;
            }
            break;
            
        case SECURITY_ALGO_ECC:
            /* 生成ECC密钥�?- 这里只是简化示�?*/
            if (key_type == SECURITY_KEY_PRIVATE || key_type == SECURITY_KEY_PUBLIC) {
                /* 在实际应用中，应该使用STM32 PKA模块生成ECC密钥�?*/
                /* 为简化示例，这里只生成随机数�?*/
                if (security_generate_random(handle, key_buffer, key_size / 8) != 0) {
                    result = -1;
                } else {
                    *key_len = key_size / 8;
                }
            } else {
                result = -1;
            }
            break;
            
        default:
            result = -1;
            break;
    }
    
    security_unlock();
    return result;
}

/**
 * @brief 导入密钥
 */
int security_import_key(security_handle_t handle, security_key_type_t key_type, 
                       uint32_t key_id, const uint8_t *key_data, uint32_t key_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key_data || key_len == 0 || 
        key_len > sizeof(g_key_storage[0].data)) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int idx = alloc_key_storage(key_id);
    if (idx < 0) {
        security_unlock();
        return -1;
    }
    
    g_key_storage[idx].type = key_type;
    g_key_storage[idx].size = key_len * 8; /* 转换为位 */
    memcpy(g_key_storage[idx].data, key_data, key_len);
    
    security_unlock();
    return 0;
}

/**
 * @brief 导出密钥
 */
int security_export_key(security_handle_t handle, uint32_t key_id, 
                       uint8_t *key_data, uint32_t *key_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key_data || !key_len) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int idx = find_key_storage(key_id);
    if (idx < 0) {
        security_unlock();
        return -1;
    }
    
    uint32_t size = g_key_storage[idx].size / 8; /* 转换为字�?*/
    if (*key_len < size) {
        security_unlock();
        return -1;
    }
    
    memcpy(key_data, g_key_storage[idx].data, size);
    *key_len = size;
    
    security_unlock();
    return 0;
}

/**
 * @brief 删除密钥
 */
int security_delete_key(security_handle_t handle, uint32_t key_id)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int idx = find_key_storage(key_id);
    if (idx < 0) {
        security_unlock();
        return -1;
    }
    
    /* 安全擦除密钥数据 */
    memset(&g_key_storage[idx], 0, sizeof(key_storage_t));
    
    security_unlock();
    return 0;
}

/**
 * @brief 配置加密引擎
 */
static int config_crypto_engine(stm32_security_ctx_t *ctx, const security_config_t *config, uint32_t key_id)
{
    int idx = find_key_storage(key_id);
    if (idx < 0) {
        return -1;
    }
    
    /* 配置加密算法和模�?*/
    switch (config->algo) {
        case SECURITY_ALGO_AES:
            switch (config->mode) {
                case SECURITY_MODE_ECB:
                    ctx->hcryp.Init.Algorithm = CRYP_AES_ECB;
                    break;
                case SECURITY_MODE_CBC:
                    ctx->hcryp.Init.Algorithm = CRYP_AES_CBC;
                    break;
                case SECURITY_MODE_CTR:
                    ctx->hcryp.Init.Algorithm = CRYP_AES_CTR;
                    break;
                case SECURITY_MODE_GCM:
                    ctx->hcryp.Init.Algorithm = CRYP_AES_GCM;
                    break;
                case SECURITY_MODE_CCM:
                    ctx->hcryp.Init.Algorithm = CRYP_AES_CCM;
                    break;
                default:
                    return -1;
            }
            
            /* 配置密钥大小 */
            switch (g_key_storage[idx].size) {
                case 128:
                    ctx->hcryp.Init.KeySize = CRYP_KEYSIZE_128B;
                    break;
                case 192:
                    ctx->hcryp.Init.KeySize = CRYP_KEYSIZE_192B;
                    break;
                case 256:
                    ctx->hcryp.Init.KeySize = CRYP_KEYSIZE_256B;
                    break;
                default:
                    return -1;
            }
            
            /* 设置密钥 */
            ctx->hcryp.Init.pKey = g_key_storage[idx].data;
            
            /* 设置初始化向�?*/
            if (config->mode != SECURITY_MODE_ECB) {
                if (!config->iv || config->iv_len < 16) {
                    return -1;
                }
                ctx->hcryp.Init.pInitVect = config->iv;
            }
            
            /* 重新初始化加密引�?*/
            if (HAL_CRYP_DeInit(&ctx->hcryp) != HAL_OK) {
                return -1;
            }
            
            if (HAL_CRYP_Init(&ctx->hcryp) != HAL_OK) {
                return -1;
            }
            
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

/**
 * @brief 加密数据
 */
int security_encrypt(security_handle_t handle, const security_config_t *config, 
                    uint32_t key_id, const uint8_t *input, uint32_t input_len,
                    uint8_t *output, uint32_t *output_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !config || !input || !output || !output_len ||
        input_len == 0 || *output_len < input_len) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int result = 0;
    ctx->status = SECURITY_STATUS_BUSY;
    
    /* 配置加密引擎 */
    if (config_crypto_engine(ctx, config, key_id) != 0) {
        ctx->status = SECURITY_STATUS_ERROR;
        security_unlock();
        return -1;
    }
    
    /* 执行加密操作 */
    HAL_StatusTypeDef status;
    switch (config->algo) {
        case SECURITY_ALGO_AES:
            /* 处理GCM/CCM模式下的AAD数据 */
            if ((config->mode == SECURITY_MODE_GCM || config->mode == SECURITY_MODE_CCM) &&
                config->aad && config->aad_len > 0) {
                status = HAL_CRYPEx_AESGCM_SetHeaderPhase(&ctx->hcryp, config->aad, config->aad_len, TIMEOUT_VALUE);
                if (status != HAL_OK) {
                    result = -1;
                    break;
                }
            }
            
            /* 执行加密 */
            status = HAL_CRYP_Encrypt(&ctx->hcryp, (uint32_t *)input, input_len, (uint32_t *)output, TIMEOUT_VALUE);
            if (status != HAL_OK) {
                result = -1;
            } else {
                *output_len = input_len;
            }
            break;
            
        default:
            result = -1;
            break;
    }
    
    if (result == 0) {
        ctx->status = SECURITY_STATUS_COMPLETE;
    } else {
        ctx->status = SECURITY_STATUS_ERROR;
    }
    
    security_unlock();
    
    /* 调用回调函数 */
    if (ctx->callback) {
        ctx->callback(ctx->callback_arg, ctx->status);
    }
    
    return result;
}

/**
 * @brief 解密数据
 */
int security_decrypt(security_handle_t handle, const security_config_t *config, 
                    uint32_t key_id, const uint8_t *input, uint32_t input_len,
                    uint8_t *output, uint32_t *output_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !config || !input || !output || !output_len ||
        input_len == 0 || *output_len < input_len) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int result = 0;
    ctx->status = SECURITY_STATUS_BUSY;
    
    /* 配置解密引擎 */
    if (config_crypto_engine(ctx, config, key_id) != 0) {
        ctx->status = SECURITY_STATUS_ERROR;
        security_unlock();
        return -1;
    }
    
    /* 执行解密操作 */
    HAL_StatusTypeDef status;
    switch (config->algo) {
        case SECURITY_ALGO_AES:
            /* 处理GCM/CCM模式下的AAD数据 */
            if ((config->mode == SECURITY_MODE_GCM || config->mode == SECURITY_MODE_CCM) &&
                config->aad && config->aad_len > 0) {
                status = HAL_CRYPEx_AESGCM_SetHeaderPhase(&ctx->hcryp, config->aad, config->aad_len, TIMEOUT_VALUE);
                if (status != HAL_OK) {
                    result = -1;
                    break;
                }
            }
            
            /* 执行解密 */
            status = HAL_CRYP_Decrypt(&ctx->hcryp, (uint32_t *)input, input_len, (uint32_t *)output, TIMEOUT_VALUE);
            if (status != HAL_OK) {
                result = -1;
            } else {
                *output_len = input_len;
            }
            break;
            
        default:
            result = -1;
            break;
    }
    
    if (result == 0) {
        ctx->status = SECURITY_STATUS_COMPLETE;
    } else {
        ctx->status = SECURITY_STATUS_ERROR;
    }
    
    security_unlock();
    
    /* 调用回调函数 */
    if (ctx->callback) {
        ctx->callback(ctx->callback_arg, ctx->status);
    }
    
    return result;
}

/**
 * @brief 配置哈希引擎
 */
static int config_hash_engine(stm32_security_ctx_t *ctx, security_hash_t hash_type)
{
    /* 配置哈希算法 */
    switch (hash_type) {
        case SECURITY_HASH_MD5:
            ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_MD5;
            break;
        case SECURITY_HASH_SHA1:
            ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA1;
            break;
        case SECURITY_HASH_SHA224:
            ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA224;
            break;
        case SECURITY_HASH_SHA256:
            ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA256;
            break;
        default:
            return -1;
    }
    
    /* 重新初始化哈希引�?*/
    if (HAL_HASH_DeInit(&ctx->hhash) != HAL_OK) {
        return -1;
    }
    
    if (HAL_HASH_Init(&ctx->hhash) != HAL_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 计算哈希�?
 */
int security_hash(security_handle_t handle, security_hash_t hash_type,
                 const uint8_t *input, uint32_t input_len,
                 uint8_t *output, uint32_t *output_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !input || !output || !output_len ||
        input_len == 0) {
        return -1;
    }
    
    /* 验证输出缓冲区大�?*/
    uint32_t required_size;
    switch (hash_type) {
        case SECURITY_HASH_MD5:
            required_size = 16;
            break;
        case SECURITY_HASH_SHA1:
            required_size = 20;
            break;
        case SECURITY_HASH_SHA224:
            required_size = 28;
            break;
        case SECURITY_HASH_SHA256:
            required_size = 32;
            break;
        case SECURITY_HASH_CRC32:
            required_size = 4;
            break;
        default:
            return -1;
    }
    
    if (*output_len < required_size) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int result = 0;
    ctx->status = SECURITY_STATUS_BUSY;
    
    /* 处理特殊的CRC32哈希 */
    if (hash_type == SECURITY_HASH_CRC32) {
        /* 使用STM32 HAL CRC模块计算CRC32 */
        CRC_HandleTypeDef hcrc;
        hcrc.Instance = CRC;
        hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
        hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
        hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
        hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
        hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
        
        if (HAL_CRC_Init(&hcrc) != HAL_OK) {
            result = -1;
        } else {
            uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)input, input_len);
            memcpy(output, &crc, 4);
            *output_len = 4;
            HAL_CRC_DeInit(&hcrc);
        }
    } else {
        /* 配置哈希引擎 */
        if (config_hash_engine(ctx, hash_type) != 0) {
            result = -1;
        } else {
            /* 执行哈希计算 */
            if (HAL_HASH_Start(&ctx->hhash, (uint8_t *)input, input_len, output, TIMEOUT_VALUE) != HAL_OK) {
                result = -1;
            } else {
                *output_len = required_size;
            }
        }
    }
    
    if (result == 0) {
        ctx->status = SECURITY_STATUS_COMPLETE;
    } else {
        ctx->status = SECURITY_STATUS_ERROR;
    }
    
    security_unlock();
    
    /* 调用回调函数 */
    if (ctx->callback) {
        ctx->callback(ctx->callback_arg, ctx->status);
    }
    
    return result;
}

/**
 * @brief 数字签名
 */
int security_sign(security_handle_t handle, uint32_t key_id,
                 const uint8_t *input, uint32_t input_len,
                 uint8_t *signature, uint32_t *sig_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !input || !signature || !sig_len ||
        input_len == 0) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int result = 0;
    ctx->status = SECURITY_STATUS_BUSY;
    
    /* 查找密钥 */
    int idx = find_key_storage(key_id);
    if (idx < 0 || g_key_storage[idx].type != SECURITY_KEY_PRIVATE) {
        result = -1;
    } else {
        /* 根据密钥类型执行签名操作 - 简化示�?*/
        /* 在实际应用中，应该使用STM32 PKA模块或外部库实现签名 */
        /* 为简化示例，这里只计算输入数据的哈希值作为签�?*/
        uint8_t hash[32];
        uint32_t hash_len = sizeof(hash);
        
        if (security_hash(handle, SECURITY_HASH_SHA256, input, input_len, hash, &hash_len) != 0) {
            result = -1;
        } else if (*sig_len < hash_len) {
            result = -1;
        } else {
            memcpy(signature, hash, hash_len);
            *sig_len = hash_len;
        }
    }
    
    if (result == 0) {
        ctx->status = SECURITY_STATUS_COMPLETE;
    } else {
        ctx->status = SECURITY_STATUS_ERROR;
    }
    
    security_unlock();
    
    /* 调用回调函数 */
    if (ctx->callback) {
        ctx->callback(ctx->callback_arg, ctx->status);
    }
    
    return result;
}

/**
 * @brief 验证签名
 */
int security_verify(security_handle_t handle, uint32_t key_id,
                   const uint8_t *input, uint32_t input_len,
                   const uint8_t *signature, uint32_t sig_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !input || !signature ||
        input_len == 0 || sig_len == 0) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    int result = 0;
    ctx->status = SECURITY_STATUS_BUSY;
    
    /* 查找密钥 */
    int idx = find_key_storage(key_id);
    if (idx < 0 || g_key_storage[idx].type != SECURITY_KEY_PUBLIC) {
        result = -1;
    } else {
        /* 根据密钥类型执行验证操作 - 简化示�?*/
        /* 在实际应用中，应该使用STM32 PKA模块或外部库实现签名验证 */
        /* 为简化示例，这里只计算输入数据的哈希值并与签名比�?*/
        uint8_t hash[32];
        uint32_t hash_len = sizeof(hash);
        
        if (security_hash(handle, SECURITY_HASH_SHA256, input, input_len, hash, &hash_len) != 0) {
            result = -1;
        } else if (sig_len != hash_len) {
            result = -1;
        } else if (memcmp(signature, hash, hash_len) != 0) {
            result = -1;
        }
    }
    
    if (result == 0) {
        ctx->status = SECURITY_STATUS_COMPLETE;
    } else {
        ctx->status = SECURITY_STATUS_ERROR;
    }
    
    security_unlock();
    
    /* 调用回调函数 */
    if (ctx->callback) {
        ctx->callback(ctx->callback_arg, ctx->status);
    }
    
    return result;
}

/**
 * @brief 安全启动验证
 */
int security_secure_boot_verify(security_handle_t handle)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    /* 实际应用中，应该验证固件签名、检查固件完整性等 */
    /* 为简化示例，这里直接返回成功 */
    return 0;
}

/**
 * @brief 固件更新验证
 */
int security_verify_firmware(security_handle_t handle,
                            const uint8_t *firmware_addr, uint32_t firmware_size,
                            const uint8_t *signature, uint32_t sig_len,
                            firmware_validation_result_t *result)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !firmware_addr || !signature || !result ||
        firmware_size == 0 || sig_len == 0) {
        return -1;
    }
    
    if (security_lock() != 0) {
        return -1;
    }
    
    /* 实际应用中，应该验证固件签名、检查固件版本和兼容性等 */
    /* 为简化示例，这里只计算固件的哈希值并与签名比�?*/
    uint8_t hash[32];
    uint32_t hash_len = sizeof(hash);
    
    if (security_hash(handle, SECURITY_HASH_SHA256, firmware_addr, firmware_size, hash, &hash_len) != 0) {
        *result = FIRMWARE_CORRUPTED;
    } else if (sig_len != hash_len) {
        *result = FIRMWARE_INVALID_SIGNATURE;
    } else if (memcmp(signature, hash, hash_len) != 0) {
        *result = FIRMWARE_INVALID_SIGNATURE;
    } else {
        /* 检查固件头部以验证版本和平台兼容�?*/
        /* 这里是简化示例，实际应用中应该解析固件头�?*/
        *result = FIRMWARE_VALID;
    }
    
    security_unlock();
    return 0;
}

/**
 * @brief 加密存储写入
 */
int security_secure_storage_write(security_handle_t handle,
                                 const char *key,
                                 const uint8_t *data, uint32_t data_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key || !data || data_len == 0) {
        return -1;
    }
    
    /* 实际应用中，应该使用安全存储区（如STM32的OTP或保护区域Flash）存储加密数�?*/
    /* 为简化示例，这里不实现实际的存储操作 */
    return 0;
}

/**
 * @brief 加密存储读取
 */
int security_secure_storage_read(security_handle_t handle,
                                const char *key,
                                uint8_t *data, uint32_t *data_len)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key || !data || !data_len) {
        return -1;
    }
    
    /* 实际应用中，应该从安全存储区读取并解密数�?*/
    /* 为简化示例，这里不实现实际的读取操作 */
    *data_len = 0;
    return 0;
}

/**
 * @brief 删除安全存储�?
 */
int security_secure_storage_delete(security_handle_t handle, const char *key)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !key) {
        return -1;
    }
    
    /* 实际应用中，应该删除安全存储区中的指定项 */
    /* 为简化示例，这里不实现实际的删除操作 */
    return 0;
}

/**
 * @brief 获取安全模块状�?
 */
int security_get_status(security_handle_t handle, security_status_t *status)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !status) {
        return -1;
    }
    
    *status = ctx->status;
    return 0;
}

/**
 * @brief 获取支持的算�?
 */
int security_get_supported_algorithms(security_handle_t handle, 
                                     security_algo_t *algos, uint32_t *count)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !algos || !count || *count == 0) {
        return -1;
    }
    
    /* STM32平台支持的加密算�?*/
    security_algo_t supported[] = {
        SECURITY_ALGO_AES,
        SECURITY_ALGO_RSA,
        SECURITY_ALGO_ECC
    };
    
    uint32_t supported_count = sizeof(supported) / sizeof(supported[0]);
    uint32_t copy_count = (*count < supported_count) ? *count : supported_count;
    
    memcpy(algos, supported, copy_count * sizeof(security_algo_t));
    *count = copy_count;
    
    return 0;
}

/**
 * @brief 获取支持的哈希算�?
 */
int security_get_supported_hashes(security_handle_t handle, 
                                 security_hash_t *hashes, uint32_t *count)
{
    stm32_security_ctx_t *ctx = (stm32_security_ctx_t *)handle;
    
    if (!ctx || !ctx->initialized || !hashes || !count || *count == 0) {
        return -1;
    }
    
    /* STM32平台支持的哈希算�?*/
    security_hash_t supported[] = {
        SECURITY_HASH_MD5,
        SECURITY_HASH_SHA1,
        SECURITY_HASH_SHA224,
        SECURITY_HASH_SHA256,
        SECURITY_HASH_CRC32
    };
    
    uint32_t supported_count = sizeof(supported) / sizeof(supported[0]);
    uint32_t copy_count = (*count < supported_count) ? *count : supported_count;
    
    memcpy(hashes, supported, copy_count * sizeof(security_hash_t));
    *count = copy_count;
    
    return 0;
}
