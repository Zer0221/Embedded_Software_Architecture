/**
 * @file biometric_api.h
 * @brief 生物识别接口抽象层定义
 *
 * 该头文件定义了生物识别的统一抽象接口，包括人脸识别、指纹识别等生物特征识别功能
 */

#ifndef BIOMETRIC_API_H
#define BIOMETRIC_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"
#include "camera_api.h"

/* 生物识别句柄 */
typedef driver_handle_t biometric_handle_t;

/* 生物特征类型枚举 */
typedef enum {
    BIOMETRIC_TYPE_FACE,      /**< 人脸 */
    BIOMETRIC_TYPE_FINGERPRINT,/**< 指纹 */
    BIOMETRIC_TYPE_IRIS,      /**< 虹膜 */
    BIOMETRIC_TYPE_VOICE,     /**< 声纹 */
    BIOMETRIC_TYPE_CUSTOM     /**< 自定义特征 */
} biometric_type_t;

/* 生物特征匹配模式枚举 */
typedef enum {
    MATCH_MODE_1N,            /**< 1:N识别模式 */
    MATCH_MODE_1V1,           /**< 1:1验证模式 */
    MATCH_MODE_DETECT_ONLY    /**< 仅检测模式 */
} match_mode_t;

/* 生物特征结果枚举 */
typedef enum {
    RESULT_NO_FEATURE,        /**< 未检测到特征 */
    RESULT_FEATURE_DETECTED,  /**< 检测到特征 */
    RESULT_VERIFIED,          /**< 验证通过 */
    RESULT_REJECTED,          /**< 验证失败 */
    RESULT_UNCERTAIN          /**< 结果不确定 */
} biometric_result_t;

/* 人脸特征结构体 */
typedef struct {
    uint32_t face_id;         /**< 人脸ID */
    int16_t x;                /**< 人脸框左上角x坐标 */
    int16_t y;                /**< 人脸框左上角y坐标 */
    uint16_t width;           /**< 人脸框宽度 */
    uint16_t height;          /**< 人脸框高度 */
    float confidence;         /**< 置信度 */
    int16_t yaw;              /**< 偏航角 */
    int16_t pitch;            /**< 俯仰角 */
    int16_t roll;             /**< 滚转角 */
    uint8_t age;              /**< 估计年龄 */
    uint8_t gender;           /**< 性别(0-100,0为女性,100为男性) */
    bool has_mask;            /**< 是否佩戴口罩 */
    bool has_glasses;         /**< 是否佩戴眼镜 */
    bool live_detection;      /**< 活体检测结果 */
    char person_name[32];     /**< 人员姓名(如已识别) */
    char extra_info[64];      /**< 额外信息 */
} face_feature_t;

/* 指纹特征结构体 */
typedef struct {
    uint32_t fingerprint_id;  /**< 指纹ID */
    float confidence;         /**< 置信度 */
    uint8_t finger_index;     /**< 手指索引(0-9) */
    char person_name[32];     /**< 人员姓名(如已识别) */
} fingerprint_feature_t;

/* 生物特征配置结构体 */
typedef struct {
    biometric_type_t type;                /**< 特征类型 */
    match_mode_t mode;                    /**< 匹配模式 */
    float threshold;                      /**< 匹配阈值 */
    uint16_t timeout_ms;                  /**< 识别超时时间 */
    uint16_t max_results;                 /**< 最大结果数 */
    bool enable_live_detection;           /**< 启用活体检测(适用于人脸) */
    char database_path[128];              /**< 特征库路径 */
    void *algorithm_config;               /**< 算法特定配置 */
    size_t algorithm_config_size;         /**< 算法配置大小 */
} biometric_config_t;

/* 人脸识别特定配置 */
typedef struct {
    uint8_t min_face_size;                /**< 最小人脸尺寸占图像比例(%) */
    uint8_t max_face_count;               /**< 最大检测人脸数 */
    bool detect_age;                      /**< 是否检测年龄 */
    bool detect_gender;                   /**< 是否检测性别 */
    bool detect_mask;                     /**< 是否检测口罩 */
    bool detect_glasses;                  /**< 是否检测眼镜 */
    uint8_t live_detection_level;         /**< 活体检测等级(0-5) */
} face_algorithm_config_t;

/* 指纹识别特定配置 */
typedef struct {
    uint8_t security_level;               /**< 安全级别(0-5) */
    bool check_finger_quality;            /**< 是否检查指纹质量 */
    uint8_t min_finger_quality;           /**< 最低指纹质量要求(0-100) */
} fingerprint_algorithm_config_t;

/* 生物特征结果回调函数类型 */
typedef void (*biometric_result_callback_t)(biometric_result_t result, 
                                           void *feature_data, 
                                           void *user_data);

/**
 * @brief 初始化生物识别模块
 * 
 * @param config 生物识别配置
 * @param handle 返回的生物识别句柄
 * @return int 0表示成功，非0表示失败
 */
int biometric_init(const biometric_config_t *config, biometric_handle_t *handle);

/**
 * @brief 注册生物特征结果回调
 * 
 * @param handle 生物识别句柄
 * @param callback 回调函数
 * @param user_data 用户数据指针
 * @return int 0表示成功，非0表示失败
 */
int biometric_register_result_callback(biometric_handle_t handle, 
                                      biometric_result_callback_t callback, 
                                      void *user_data);

/**
 * @brief 启动生物识别
 * 
 * @param handle 生物识别句柄
 * @return int 0表示成功，非0表示失败
 */
int biometric_start(biometric_handle_t handle);

/**
 * @brief 停止生物识别
 * 
 * @param handle 生物识别句柄
 * @return int 0表示成功，非0表示失败
 */
int biometric_stop(biometric_handle_t handle);

/**
 * @brief 处理图像进行人脸识别
 * 
 * @param handle 生物识别句柄
 * @param image 图像数据
 * @param features 人脸特征数组
 * @param max_features 数组最大容量
 * @param actual_count 实际检测到的人脸数量
 * @return int 0表示成功，非0表示失败
 */
int biometric_process_face_image(biometric_handle_t handle, 
                                const camera_image_t *image, 
                                face_feature_t *features, 
                                size_t max_features, 
                                size_t *actual_count);

/**
 * @brief 处理指纹数据
 * 
 * @param handle 生物识别句柄
 * @param fingerprint_data 指纹数据
 * @param data_length 数据长度
 * @param feature 指纹特征结构体指针
 * @return int 0表示成功，非0表示失败
 */
int biometric_process_fingerprint(biometric_handle_t handle, 
                                 const uint8_t *fingerprint_data, 
                                 size_t data_length, 
                                 fingerprint_feature_t *feature);

/**
 * @brief 添加生物特征到数据库
 * 
 * @param handle 生物识别句柄
 * @param person_id 人员ID
 * @param person_name 人员姓名
 * @param feature_data 特征数据
 * @param feature_id 返回的特征ID
 * @return int 0表示成功，非0表示失败
 */
int biometric_add_feature(biometric_handle_t handle, 
                         const char *person_id, 
                         const char *person_name, 
                         const void *feature_data, 
                         uint32_t *feature_id);

/**
 * @brief 从数据库删除生物特征
 * 
 * @param handle 生物识别句柄
 * @param feature_id 特征ID
 * @return int 0表示成功，非0表示失败
 */
int biometric_delete_feature(biometric_handle_t handle, uint32_t feature_id);

/**
 * @brief 从数据库删除人员所有特征
 * 
 * @param handle 生物识别句柄
 * @param person_id 人员ID
 * @return int 0表示成功，非0表示失败
 */
int biometric_delete_person(biometric_handle_t handle, const char *person_id);

/**
 * @brief 清空特征数据库
 * 
 * @param handle 生物识别句柄
 * @return int 0表示成功，非0表示失败
 */
int biometric_clear_database(biometric_handle_t handle);

/**
 * @brief 验证生物特征(1:1模式)
 * 
 * @param handle 生物识别句柄
 * @param person_id 人员ID
 * @param feature_data 特征数据
 * @param result 验证结果
 * @param confidence 置信度
 * @return int 0表示成功，非0表示失败
 */
int biometric_verify_feature(biometric_handle_t handle, 
                            const char *person_id, 
                            const void *feature_data, 
                            biometric_result_t *result, 
                            float *confidence);

/**
 * @brief 识别生物特征(1:N模式)
 * 
 * @param handle 生物识别句柄
 * @param feature_data 特征数据
 * @param person_id 返回的人员ID
 * @param person_id_len 人员ID缓冲区长度
 * @param result 识别结果
 * @param confidence 置信度
 * @return int 0表示成功，非0表示失败
 */
int biometric_identify_feature(biometric_handle_t handle, 
                              const void *feature_data, 
                              char *person_id, 
                              size_t person_id_len, 
                              biometric_result_t *result, 
                              float *confidence);

/**
 * @brief 设置识别参数
 * 
 * @param handle 生物识别句柄
 * @param param_name 参数名
 * @param value 参数值
 * @return int 0表示成功，非0表示失败
 */
int biometric_set_parameter(biometric_handle_t handle, 
                           const char *param_name, 
                           const void *value, 
                           size_t value_size);

/**
 * @brief 获取识别参数
 * 
 * @param handle 生物识别句柄
 * @param param_name 参数名
 * @param value 参数值缓冲区
 * @param value_size 缓冲区大小
 * @param actual_size 实际参数大小
 * @return int 0表示成功，非0表示失败
 */
int biometric_get_parameter(biometric_handle_t handle, 
                           const char *param_name, 
                           void *value, 
                           size_t value_size, 
                           size_t *actual_size);

#endif /* BIOMETRIC_API_H */
