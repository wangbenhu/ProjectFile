#ifndef WIFI_CONFIG_CODEC_H
#define WIFI_CONFIG_CODEC_H

#include <stddef.h>

#include "wifi_config_types.h"

#define WIFI_CONFIG_TEXT_MAX_SIZE 256// WIFI配置文本最大大小
#define FENCE_CONFIG_TEXT_MAX_SIZE 1152U// 围栏配置文本最大大小

#define CONFIG_TABLE_FIELD_CAPACITY(type, name, header, kind, default_value) \
    + (sizeof(header) - 1U) + 10U + 2U

enum {
    CONFIG_TABLE_TEXT_MAX_SIZE =
        1U CONFIG_TABLE_FIELDS(CONFIG_TABLE_FIELD_CAPACITY)
};

#undef CONFIG_TABLE_FIELD_CAPACITY
/*
* @brief 编码WIFI配置
* @param config WIFI配置
* @param output 输出缓冲区
* @param output_size 输出缓冲区大小
* @return 编码后的字节数
 */
int wifi_config_encode(const WifiConfig_t *config,
                       char *output,
                       size_t output_size);
/*
* @brief 解码WIFI配置
* @param input 输入缓冲区
* @param input_len 输入缓冲区大小
* @param config 输出的WIFI配置
* @return 0 on success; a negative value on failure
 */
int wifi_config_decode(const char *input,
                       size_t input_len,
                       WifiConfig_t *config);
/*
* @brief 编码围栏配置
* @param config 围栏配置
* @param output 输出缓冲区
* @param output_size 输出缓冲区大小
* @return 编码后的字节数
 */
int fence_config_encode(const FenceConfig_t *config,
                        char *output,
                        size_t output_size);
/*
* @brief 解码围栏配置
* @param input 输入缓冲区
* @param input_len 输入缓冲区大小
* @param config 输出的围栏配置
* @return 解码后的字节数
 */
int fence_config_decode(const char *input,
                        size_t input_len,
                        FenceConfig_t *config);
/*
* @brief 编码配置表
* @param config 配置表
* @param output 输出缓冲区
* @param output_size 输出缓冲区大小
* @return 编码后的字节数
 */
int config_table_encode(const ConfigTable_t *config,
                        char *output,
                        size_t output_size);
/*
* @brief 解码配置表
* @param input 输入缓冲区
* @param input_len 输入缓冲区大小
* @param config 输出的配置表
* @return 解码后的字节数
 */
int config_table_decode(const char *input,
                        size_t input_len,
                        ConfigTable_t *config);
/*
* @brief 解码配置表（扩展）
* @param input 输入缓冲区
* @param input_len 输入缓冲区大小
* @param config 输出的配置表
* @param needs_upgrade 输出是否需要升级
* @return 解码后的字节数
 */
int config_table_decode_ex(const char *input,
                           size_t input_len,
                           ConfigTable_t *config,
                           int *needs_upgrade);

#endif
