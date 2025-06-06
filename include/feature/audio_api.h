/**
 * @file audio_api.h
 * @brief 音频处理接口定义
 *
 * 该头文件定义了音频处理的统一接口，支持音频采集、播放和处理
 */

#ifndef AUDIO_API_H
#define AUDIO_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/* 音频句柄 */
typedef void* audio_handle_t;

/* 音频格式 */
typedef enum {
    AUDIO_FORMAT_PCM = 0,        /**< PCM格式 */
    AUDIO_FORMAT_MP3,            /**< MP3格式 */
    AUDIO_FORMAT_AAC,            /**< AAC格式 */
    AUDIO_FORMAT_WAV,            /**< WAV格式 */
    AUDIO_FORMAT_OPUS,           /**< OPUS格式 */
    AUDIO_FORMAT_FLAC,           /**< FLAC格式 */
    AUDIO_FORMAT_AMR             /**< AMR格式 */
} audio_format_t;

/* 音频模式 */
typedef enum {
    AUDIO_MODE_INPUT = 0,        /**< 输入模式（录音） */
    AUDIO_MODE_OUTPUT,           /**< 输出模式（播放） */
    AUDIO_MODE_DUPLEX            /**< 全双工模式（录音+播放） */
} audio_mode_t;

/* 音频采样率 */
typedef enum {
    AUDIO_SAMPLE_RATE_8K = 8000,    /**< 8kHz */
    AUDIO_SAMPLE_RATE_11K = 11025,  /**< 11.025kHz */
    AUDIO_SAMPLE_RATE_16K = 16000,  /**< 16kHz */
    AUDIO_SAMPLE_RATE_22K = 22050,  /**< 22.05kHz */
    AUDIO_SAMPLE_RATE_32K = 32000,  /**< 32kHz */
    AUDIO_SAMPLE_RATE_44K = 44100,  /**< 44.1kHz */
    AUDIO_SAMPLE_RATE_48K = 48000   /**< 48kHz */
} audio_sample_rate_t;

/* 音频位宽 */
typedef enum {
    AUDIO_BIT_WIDTH_8 = 8,       /**< 8位 */
    AUDIO_BIT_WIDTH_16 = 16,     /**< 16位 */
    AUDIO_BIT_WIDTH_24 = 24,     /**< 24位 */
    AUDIO_BIT_WIDTH_32 = 32      /**< 32位 */
} audio_bit_width_t;

/* 音频声道 */
typedef enum {
    AUDIO_CHANNEL_MONO = 1,      /**< 单声道 */
    AUDIO_CHANNEL_STEREO = 2,    /**< 立体声 */
    AUDIO_CHANNEL_QUAD = 4,      /**< 四声道 */
    AUDIO_CHANNEL_5_1 = 6,       /**< 5.1声道 */
    AUDIO_CHANNEL_7_1 = 8        /**< 7.1声道 */
} audio_channel_t;

/* 音频质量 */
typedef enum {
    AUDIO_QUALITY_LOW = 0,       /**< 低质量 */
    AUDIO_QUALITY_MEDIUM,        /**< 中等质量 */
    AUDIO_QUALITY_HIGH,          /**< 高质量 */
    AUDIO_QUALITY_BEST           /**< 最佳质量 */
} audio_quality_t;

/* 音频源类型 */
typedef enum {
    AUDIO_SOURCE_MIC = 0,        /**< 麦克风 */
    AUDIO_SOURCE_LINE_IN,        /**< 线路输入 */
    AUDIO_SOURCE_BLUETOOTH,      /**< 蓝牙输入 */
    AUDIO_SOURCE_I2S,            /**< I2S输入 */
    AUDIO_SOURCE_PDM,            /**< PDM输入 */
    AUDIO_SOURCE_USB             /**< USB输入 */
} audio_source_t;

/* 音频输出类型 */
typedef enum {
    AUDIO_SINK_SPEAKER = 0,      /**< 扬声器 */
    AUDIO_SINK_HEADPHONE,        /**< 耳机 */
    AUDIO_SINK_LINE_OUT,         /**< 线路输出 */
    AUDIO_SINK_BLUETOOTH,        /**< 蓝牙输出 */
    AUDIO_SINK_I2S,              /**< I2S输出 */
    AUDIO_SINK_USB               /**< USB输出 */
} audio_sink_t;

/* 音频效果类型 */
typedef enum {
    AUDIO_EFFECT_NONE = 0,       /**< 无效果 */
    AUDIO_EFFECT_ECHO,           /**< 回声 */
    AUDIO_EFFECT_REVERB,         /**< 混响 */
    AUDIO_EFFECT_BASS_BOOST,     /**< 低音增强 */
    AUDIO_EFFECT_NOISE_SUPPRESSION, /**< 噪声抑制 */
    AUDIO_EFFECT_EQUALIZER,      /**< 均衡器 */
    AUDIO_EFFECT_AEC,            /**< 回声消除 */
    AUDIO_EFFECT_AGC             /**< 自动增益控制 */
} audio_effect_t;

/* 音频配置 */
typedef struct {
    audio_mode_t mode;           /**< 音频模式 */
    audio_format_t format;       /**< 音频格式 */
    audio_sample_rate_t sample_rate; /**< 采样率 */
    audio_bit_width_t bit_width; /**< 位宽 */
    audio_channel_t channel;     /**< 声道 */
    audio_source_t source;       /**< 音频源类型 */
    audio_sink_t sink;           /**< 音频输出类型 */
    uint32_t buffer_size;        /**< 缓冲区大小（字节） */
    uint8_t volume;              /**< 音量（0-100） */
    bool use_dma;                /**< 是否使用DMA */
} audio_config_t;

/* 音频缓冲区状态 */
typedef struct {
    uint32_t total_size;         /**< 缓冲区总大小（字节） */
    uint32_t available_size;     /**< 可用大小（字节） */
    uint32_t used_size;          /**< 已用大小（字节） */
} audio_buffer_state_t;

/* 音频回调函数类型 */
typedef void (*audio_data_cb_t)(uint8_t *data, uint32_t len, void *user_data);
typedef void (*audio_event_cb_t)(uint8_t event, void *user_data);

/**
 * @brief 初始化音频
 * 
 * @param config 音频配置
 * @param data_cb 数据回调函数
 * @param event_cb 事件回调函数
 * @param user_data 用户数据，会传递给回调函数
 * @return audio_handle_t 音频句柄，NULL表示失败
 */
audio_handle_t audio_init(audio_config_t *config, audio_data_cb_t data_cb, audio_event_cb_t event_cb, void *user_data);

/**
 * @brief 释放音频
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_deinit(audio_handle_t handle);

/**
 * @brief 启动音频
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_start(audio_handle_t handle);

/**
 * @brief 停止音频
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_stop(audio_handle_t handle);

/**
 * @brief 暂停音频
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_pause(audio_handle_t handle);

/**
 * @brief 恢复音频
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_resume(audio_handle_t handle);

/**
 * @brief 写入音频数据（播放）
 * 
 * @param handle 音频句柄
 * @param data 数据
 * @param len 数据长度
 * @return int 实际写入的字节数，负值表示失败
 */
int audio_write(audio_handle_t handle, const uint8_t *data, uint32_t len);

/**
 * @brief 读取音频数据（录音）
 * 
 * @param handle 音频句柄
 * @param data 数据缓冲区
 * @param len 要读取的字节数
 * @return int 实际读取的字节数，负值表示失败
 */
int audio_read(audio_handle_t handle, uint8_t *data, uint32_t len);

/**
 * @brief 设置音量
 * 
 * @param handle 音频句柄
 * @param volume 音量（0-100）
 * @return int 0表示成功，负值表示失败
 */
int audio_set_volume(audio_handle_t handle, uint8_t volume);

/**
 * @brief 获取音量
 * 
 * @param handle 音频句柄
 * @param volume 音量
 * @return int 0表示成功，负值表示失败
 */
int audio_get_volume(audio_handle_t handle, uint8_t *volume);

/**
 * @brief 静音
 * 
 * @param handle 音频句柄
 * @param mute 是否静音
 * @return int 0表示成功，负值表示失败
 */
int audio_set_mute(audio_handle_t handle, bool mute);

/**
 * @brief 设置采样率
 * 
 * @param handle 音频句柄
 * @param sample_rate 采样率
 * @return int 0表示成功，负值表示失败
 */
int audio_set_sample_rate(audio_handle_t handle, audio_sample_rate_t sample_rate);

/**
 * @brief 应用音频效果
 * 
 * @param handle 音频句柄
 * @param effect 效果类型
 * @param params 效果参数
 * @param params_size 参数大小
 * @return int 0表示成功，负值表示失败
 */
int audio_apply_effect(audio_handle_t handle, audio_effect_t effect, void *params, uint32_t params_size);

/**
 * @brief 移除音频效果
 * 
 * @param handle 音频句柄
 * @param effect 效果类型
 * @return int 0表示成功，负值表示失败
 */
int audio_remove_effect(audio_handle_t handle, audio_effect_t effect);

/**
 * @brief 获取缓冲区状态
 * 
 * @param handle 音频句柄
 * @param state 缓冲区状态
 * @return int 0表示成功，负值表示失败
 */
int audio_get_buffer_state(audio_handle_t handle, audio_buffer_state_t *state);

/**
 * @brief 清空缓冲区
 * 
 * @param handle 音频句柄
 * @return int 0表示成功，负值表示失败
 */
int audio_flush_buffer(audio_handle_t handle);

/**
 * @brief 获取当前音频时间
 * 
 * @param handle 音频句柄
 * @param time_ms 时间（毫秒）
 * @return int 0表示成功，负值表示失败
 */
int audio_get_time(audio_handle_t handle, uint32_t *time_ms);

/**
 * @brief 设置音频时间（定位）
 * 
 * @param handle 音频句柄
 * @param time_ms 时间（毫秒）
 * @return int 0表示成功，负值表示失败
 */
int audio_set_time(audio_handle_t handle, uint32_t time_ms);

/**
 * @brief 检测是否支持特定格式
 * 
 * @param handle 音频句柄
 * @param format 音频格式
 * @return int 0表示支持，负值表示不支持
 */
int audio_is_format_supported(audio_handle_t handle, audio_format_t format);

#endif /* AUDIO_API_H */
