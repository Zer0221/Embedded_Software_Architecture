/**
 * @file ai_api.h
 * @brief 人工智能接口定义
 *
 * 该头文件定义了人工智能相关的统一接口，支持机器学习模型的加载、推理和管理
 */

#ifndef AI_API_H
#define AI_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* AI句柄 */
typedef void* ai_handle_t;

/* AI模型类型 */
typedef enum {
    AI_MODEL_TYPE_TFLITE = 0,    /**< TensorFlow Lite模型 */
    AI_MODEL_TYPE_ONNX,          /**< ONNX模型 */
    AI_MODEL_TYPE_CUSTOM,        /**< 自定义模型格式 */
    AI_MODEL_TYPE_CUBE_AI        /**< STM32 CubeAI模型 */
} ai_model_type_t;

/* AI加速器类型 */
typedef enum {
    AI_ACCEL_TYPE_NONE = 0,      /**< 无加速器，使用CPU */
    AI_ACCEL_TYPE_NPU,           /**< 神经网络处理单元 */
    AI_ACCEL_TYPE_GPU,           /**< 图形处理单元 */
    AI_ACCEL_TYPE_DSP,           /**< 数字信号处理器 */
    AI_ACCEL_TYPE_VECTOR         /**< 向量处理单元 */
} ai_accel_type_t;

/* AI模型配置 */
typedef struct {
    ai_model_type_t model_type;      /**< 模型类型 */
    ai_accel_type_t accel_type;      /**< 加速器类型 */
    uint8_t input_count;             /**< 输入张量数量 */
    uint8_t output_count;            /**< 输出张量数量 */
    const void* model_data;          /**< 模型数据指针 */
    uint32_t model_size;             /**< 模型大小（字节） */
    uint32_t workspace_size;         /**< 工作区大小（字节），0表示自动 */
    bool quantized;                  /**< 是否量化模型 */
} ai_config_t;

/* 张量信息 */
typedef struct {
    char name[32];                   /**< 张量名称 */
    uint8_t dims[4];                 /**< 张量维度 [N,H,W,C] */
    uint32_t size;                   /**< 张量大小（字节） */
    uint8_t type;                    /**< 张量数据类型 */
    void* data;                      /**< 张量数据指针 */
} ai_tensor_info_t;

/**
 * @brief 初始化AI引擎
 * 
 * @param config AI配置
 * @return ai_handle_t AI句柄，NULL表示失败
 */
ai_handle_t ai_init(ai_config_t* config);

/**
 * @brief 释放AI引擎
 * 
 * @param handle AI句柄
 * @return int 0表示成功，负值表示失败
 */
int ai_deinit(ai_handle_t handle);

/**
 * @brief 获取输入张量信息
 * 
 * @param handle AI句柄
 * @param index 输入张量索引
 * @param info 张量信息
 * @return int 0表示成功，负值表示失败
 */
int ai_get_input_info(ai_handle_t handle, uint8_t index, ai_tensor_info_t* info);

/**
 * @brief 获取输出张量信息
 * 
 * @param handle AI句柄
 * @param index 输出张量索引
 * @param info 张量信息
 * @return int 0表示成功，负值表示失败
 */
int ai_get_output_info(ai_handle_t handle, uint8_t index, ai_tensor_info_t* info);

/**
 * @brief 设置输入数据
 * 
 * @param handle AI句柄
 * @param index 输入张量索引
 * @param data 输入数据指针
 * @param size 数据大小（字节）
 * @return int 0表示成功，负值表示失败
 */
int ai_set_input(ai_handle_t handle, uint8_t index, const void* data, uint32_t size);

/**
 * @brief 获取输出数据
 * 
 * @param handle AI句柄
 * @param index 输出张量索引
 * @param data 输出数据指针
 * @param size 数据大小（字节）
 * @return int 0表示成功，负值表示失败
 */
int ai_get_output(ai_handle_t handle, uint8_t index, void* data, uint32_t size);

/**
 * @brief 运行推理
 * 
 * @param handle AI句柄
 * @return int 推理耗时（毫秒），负值表示失败
 */
int ai_run(ai_handle_t handle);

/**
 * @brief 获取模型内存使用情况
 * 
 * @param handle AI句柄
 * @param ram_size RAM使用量（字节）
 * @param flash_size Flash使用量（字节）
 * @return int 0表示成功，负值表示失败
 */
int ai_get_memory_usage(ai_handle_t handle, uint32_t* ram_size, uint32_t* flash_size);

/**
 * @brief 设置运行参数
 * 
 * @param handle AI句柄
 * @param param_id 参数ID
 * @param value 参数值
 * @return int 0表示成功，负值表示失败
 */
int ai_set_param(ai_handle_t handle, uint16_t param_id, uint32_t value);

/**
 * @brief 获取运行参数
 * 
 * @param handle AI句柄
 * @param param_id 参数ID
 * @param value 参数值指针
 * @return int 0表示成功，负值表示失败
 */
int ai_get_param(ai_handle_t handle, uint16_t param_id, uint32_t* value);

#endif /* AI_API_H */
