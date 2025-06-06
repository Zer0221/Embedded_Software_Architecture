/**
 * @file json_api.h
 * @brief JSON解析和生成接口定义
 *
 * 该头文件定义了JSON数据的解析和生成接口，适用于物联网数据交换
 */

#ifndef JSON_API_H
#define JSON_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "error_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* JSON值类型 */
typedef enum {
    JSON_TYPE_NULL,         /**< NULL值 */
    JSON_TYPE_BOOLEAN,      /**< 布尔值 */
    JSON_TYPE_NUMBER,       /**< 数值 */
    JSON_TYPE_STRING,       /**< 字符串 */
    JSON_TYPE_ARRAY,        /**< 数组 */
    JSON_TYPE_OBJECT,       /**< 对象 */
    JSON_TYPE_RAW           /**< 原始JSON */
} json_type_t;

/* JSON句柄 */
typedef void* json_handle_t;

/* JSON值句柄 */
typedef void* json_value_handle_t;

/* JSON解析选项 */
typedef struct {
    bool allow_comments;     /**< 是否允许注释 */
    bool ignore_unknown;     /**< 是否忽略未知字段 */
    bool allow_trailing_commas; /**< 是否允许尾随逗号 */
    uint32_t max_nesting;    /**< 最大嵌套深度 */
    uint32_t max_string_len; /**< 最大字符串长度 */
    uint32_t max_total_len;  /**< 最大总长度 */
} json_parse_options_t;

/* JSON生成选项 */
typedef struct {
    bool pretty;             /**< 是否美化输出 */
    uint8_t indent;          /**< 缩进空格数 */
    bool escape_slashes;     /**< 是否转义斜杠 */
    bool sort_keys;          /**< 是否对键进行排序 */
    bool ensure_ascii;       /**< 是否确保ASCII输出 */
} json_dump_options_t;

/**
 * @brief 创建JSON解析器
 * 
 * @param options 解析选项，NULL表示使用默认选项
 * @param handle 返回的解析器句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_parser(const json_parse_options_t* options, json_handle_t* handle);

/**
 * @brief 销毁JSON解析器
 * 
 * @param handle 解析器句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_destroy_parser(json_handle_t handle);

/**
 * @brief 解析JSON字符串
 * 
 * @param handle 解析器句柄
 * @param json_str JSON字符串
 * @param len 字符串长度
 * @param root 返回的根值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_parse(json_handle_t handle, const char* json_str, size_t len, json_value_handle_t* root);

/**
 * @brief 解析JSON文件
 * 
 * @param handle 解析器句柄
 * @param file_path 文件路径
 * @param root 返回的根值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_parse_file(json_handle_t handle, const char* file_path, json_value_handle_t* root);

/**
 * @brief 获取JSON值类型
 * 
 * @param value 值句柄
 * @param type 返回的类型
 * @return int 成功返回0，失败返回错误码
 */
int json_get_type(json_value_handle_t value, json_type_t* type);

/**
 * @brief 获取布尔值
 * 
 * @param value 值句柄
 * @param result 返回的布尔值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_bool(json_value_handle_t value, bool* result);

/**
 * @brief 获取整数值
 * 
 * @param value 值句柄
 * @param result 返回的整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_int(json_value_handle_t value, int32_t* result);

/**
 * @brief 获取64位整数值
 * 
 * @param value 值句柄
 * @param result 返回的64位整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_int64(json_value_handle_t value, int64_t* result);

/**
 * @brief 获取无符号整数值
 * 
 * @param value 值句柄
 * @param result 返回的无符号整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_uint(json_value_handle_t value, uint32_t* result);

/**
 * @brief 获取无符号64位整数值
 * 
 * @param value 值句柄
 * @param result 返回的无符号64位整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_uint64(json_value_handle_t value, uint64_t* result);

/**
 * @brief 获取浮点值
 * 
 * @param value 值句柄
 * @param result 返回的浮点值
 * @return int 成功返回0，失败返回错误码
 */
int json_get_double(json_value_handle_t value, double* result);

/**
 * @brief 获取字符串值
 * 
 * @param value 值句柄
 * @param result 返回的字符串值
 * @param len 返回的字符串长度
 * @return int 成功返回0，失败返回错误码
 */
int json_get_string(json_value_handle_t value, const char** result, size_t* len);

/**
 * @brief 获取字符串值到缓冲区
 * 
 * @param value 值句柄
 * @param buffer 缓冲区
 * @param buffer_size 缓冲区大小
 * @param copied_len 返回的复制长度
 * @return int 成功返回0，失败返回错误码
 */
int json_get_string_buffer(json_value_handle_t value, char* buffer, size_t buffer_size, size_t* copied_len);

/**
 * @brief 获取数组大小
 * 
 * @param array 数组句柄
 * @param size 返回的数组大小
 * @return int 成功返回0，失败返回错误码
 */
int json_get_array_size(json_value_handle_t array, size_t* size);

/**
 * @brief 获取数组元素
 * 
 * @param array 数组句柄
 * @param index 索引
 * @param element 返回的元素句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_get_array_element(json_value_handle_t array, size_t index, json_value_handle_t* element);

/**
 * @brief 获取对象成员数量
 * 
 * @param object 对象句柄
 * @param size 返回的成员数量
 * @return int 成功返回0，失败返回错误码
 */
int json_get_object_size(json_value_handle_t object, size_t* size);

/**
 * @brief 获取对象成员
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param member 返回的成员句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_get_object_member(json_value_handle_t object, const char* name, json_value_handle_t* member);

/**
 * @brief 获取对象成员名称
 * 
 * @param object 对象句柄
 * @param index 索引
 * @param name 返回的名称
 * @param len 返回的名称长度
 * @return int 成功返回0，失败返回错误码
 */
int json_get_object_member_name(json_value_handle_t object, size_t index, const char** name, size_t* len);

/**
 * @brief 获取对象成员值
 * 
 * @param object 对象句柄
 * @param index 索引
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_get_object_member_value(json_value_handle_t object, size_t index, json_value_handle_t* value);

/**
 * @brief 创建NULL值
 * 
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_null(json_value_handle_t* value);

/**
 * @brief 创建布尔值
 * 
 * @param b 布尔值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_bool(bool b, json_value_handle_t* value);

/**
 * @brief 创建整数值
 * 
 * @param n 整数值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_int(int32_t n, json_value_handle_t* value);

/**
 * @brief 创建64位整数值
 * 
 * @param n 64位整数值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_int64(int64_t n, json_value_handle_t* value);

/**
 * @brief 创建无符号整数值
 * 
 * @param n 无符号整数值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_uint(uint32_t n, json_value_handle_t* value);

/**
 * @brief 创建无符号64位整数值
 * 
 * @param n 无符号64位整数值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_uint64(uint64_t n, json_value_handle_t* value);

/**
 * @brief 创建浮点值
 * 
 * @param n 浮点值
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_double(double n, json_value_handle_t* value);

/**
 * @brief 创建字符串值
 * 
 * @param str 字符串
 * @param len 字符串长度，-1表示使用strlen()计算
 * @param value 返回的值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_string(const char* str, int len, json_value_handle_t* value);

/**
 * @brief 创建空数组
 * 
 * @param array 返回的数组句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_array(json_value_handle_t* array);

/**
 * @brief 创建空对象
 * 
 * @param object 返回的对象句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_create_object(json_value_handle_t* object);

/**
 * @brief 添加数组元素
 * 
 * @param array 数组句柄
 * @param element 元素句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append(json_value_handle_t array, json_value_handle_t element);

/**
 * @brief 向数组添加NULL值
 * 
 * @param array 数组句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append_null(json_value_handle_t array);

/**
 * @brief 向数组添加布尔值
 * 
 * @param array 数组句柄
 * @param b 布尔值
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append_bool(json_value_handle_t array, bool b);

/**
 * @brief 向数组添加整数值
 * 
 * @param array 数组句柄
 * @param n 整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append_int(json_value_handle_t array, int32_t n);

/**
 * @brief 向数组添加浮点值
 * 
 * @param array 数组句柄
 * @param n 浮点值
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append_double(json_value_handle_t array, double n);

/**
 * @brief 向数组添加字符串值
 * 
 * @param array 数组句柄
 * @param str 字符串
 * @param len 字符串长度，-1表示使用strlen()计算
 * @return int 成功返回0，失败返回错误码
 */
int json_array_append_string(json_value_handle_t array, const char* str, int len);

/**
 * @brief 添加对象成员
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param value 成员值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_member(json_value_handle_t object, const char* name, json_value_handle_t value);

/**
 * @brief 向对象添加NULL值
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_null(json_value_handle_t object, const char* name);

/**
 * @brief 向对象添加布尔值
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param b 布尔值
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_bool(json_value_handle_t object, const char* name, bool b);

/**
 * @brief 向对象添加整数值
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param n 整数值
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_int(json_value_handle_t object, const char* name, int32_t n);

/**
 * @brief 向对象添加浮点值
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param n 浮点值
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_double(json_value_handle_t object, const char* name, double n);

/**
 * @brief 向对象添加字符串值
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @param str 字符串
 * @param len 字符串长度，-1表示使用strlen()计算
 * @return int 成功返回0，失败返回错误码
 */
int json_object_set_string(json_value_handle_t object, const char* name, const char* str, int len);

/**
 * @brief 删除对象成员
 * 
 * @param object 对象句柄
 * @param name 成员名称
 * @return int 成功返回0，失败返回错误码
 */
int json_object_remove_member(json_value_handle_t object, const char* name);

/**
 * @brief 将JSON值转换为字符串
 * 
 * @param value 值句柄
 * @param options 生成选项，NULL表示使用默认选项
 * @param out_str 返回的字符串，使用后需要调用json_free_string释放
 * @return int 成功返回0，失败返回错误码
 */
int json_dump_string(json_value_handle_t value, const json_dump_options_t* options, char** out_str);

/**
 * @brief 将JSON值转换为文件
 * 
 * @param value 值句柄
 * @param file_path 文件路径
 * @param options 生成选项，NULL表示使用默认选项
 * @return int 成功返回0，失败返回错误码
 */
int json_dump_file(json_value_handle_t value, const char* file_path, const json_dump_options_t* options);

/**
 * @brief 复制JSON值
 * 
 * @param value 源值句柄
 * @param copy 返回的复制值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_deep_copy(json_value_handle_t value, json_value_handle_t* copy);

/**
 * @brief 释放JSON值
 * 
 * @param value 值句柄
 * @return int 成功返回0，失败返回错误码
 */
int json_free_value(json_value_handle_t value);

/**
 * @brief 释放JSON字符串
 * 
 * @param str 字符串
 * @return int 成功返回0，失败返回错误码
 */
int json_free_string(char* str);

/**
 * @brief 获取最后一个错误信息
 * 
 * @param handle 解析器句柄
 * @return const char* 错误信息
 */
const char* json_get_last_error(json_handle_t handle);

/**
 * @brief 获取最后一个错误位置
 * 
 * @param handle 解析器句柄
 * @param line 返回的行号
 * @param column 返回的列号
 * @return int 成功返回0，失败返回错误码
 */
int json_get_last_error_position(json_handle_t handle, size_t* line, size_t* column);

#ifdef __cplusplus
}
#endif

#endif /* JSON_API_H */
