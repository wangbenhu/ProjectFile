#ifndef WIFI_CONFIG_STORAGE_H
#define WIFI_CONFIG_STORAGE_H

#include <stddef.h>

#include "wifi_config_codec.h"

typedef struct {
    void *context;
    int (*lock)(void *context);
    void (*unlock)(void *context);
    int (*ready_locked)(void *context);
    int (*replace_locked)(void *context,
                          const char *content,
                          size_t content_len);
    int (*read_locked)(void *context,
                       char *buffer,
                       size_t buffer_size,
                       size_t *content_len);
} WifiConfigStorageOps;
/**
 * @brief WIFI配置存储操作
 * @param ops 存储操作符
 * @param config WIFI配置
 * @return int 0 成功 -1 失败
 */
int wifi_config_storage_set(const WifiConfigStorageOps *ops,
                            const WifiConfig_t *config);
/**
 * @brief WIFI配置存储操作
 * @param ops 存储操作符
 * @param config WIFI配置
 * @return int 0 成功 -1 失败
 */
int wifi_config_storage_get(const WifiConfigStorageOps *ops,
                            WifiConfig_t *config);

typedef struct {
    void *context;
    int (*lock)(void *context);
    void (*unlock)(void *context);
    int (*ready_locked)(void *context);
    int (*replace_locked)(void *context,
                          const char *content,
                          size_t content_len);
    int (*read_locked)(void *context,
                       char *buffer,
                       size_t buffer_size,
                       size_t *content_len);
} FenceConfigStorageOps;
/**
 * @brief 围栏配置存储操作
 * @param ops 存储操作符
 * @param config 围栏配置
 * @return int 0 成功 -1 失败
 */
int fence_config_storage_set(const FenceConfigStorageOps *ops,
                             const FenceConfig_t *config);
/**
 * @brief 围栏配置存储操作
 * @param ops 存储操作符
 * @param config 围栏配置
 * @return int 0 成功 -1 失败
 */
int fence_config_storage_get(const FenceConfigStorageOps *ops,
                             FenceConfig_t *config);

typedef struct {
    void *context;
    int not_found_error;
    int regular_file_type;
    int (*remove)(void *context, const char *path);
    int (*open)(void *context, const char *path);
    int (*write)(void *context, const char *content, size_t content_len);
    int (*sync)(void *context);
    int (*close)(void *context);
    int (*stat)(void *context,
                const char *path,
                int *type,
                size_t *size);
    int (*rename)(void *context,
                  const char *old_path,
                  const char *new_path);
} TempFileReplaceOps;
/**
 * @brief 临时文件替换操作
 * @param ops 替换操作符
 * @param temp_path 临时文件路径
 * @param target_path 目标文件路径
 * @param content 内容
 * @param content_len 内容长度
 * @return int 0 成功 -1 失败
 */
int temp_file_replace(const TempFileReplaceOps *ops,
                      const char *temp_path,
                      const char *target_path,
                      const char *content,
                      size_t content_len);
typedef struct {
    void *context;
    int (*lock)(void *context);
    void (*unlock)(void *context);
    int (*ready_locked)(void *context);
    int (*replace_locked)(void *context,
                          const char *content,
                          size_t content_len);
    int (*read_locked)(void *context,
                       char *buffer,
                       size_t buffer_size,
                       size_t *content_len);
    int (*has_data_locked)(void *context, int *has_data);
} ConfigTableStorageOps;
/**
 * @brief 配置表存储操作
 * @param ops 存储操作符
 * @param config 配置表
 * @return int 0 成功 -1 失败
 */
int config_table_storage_set(const ConfigTableStorageOps *ops,
                             const ConfigTable_t *config);
/**
 * @brief 配置表存储操作
 * @param ops 存储操作符
 * @param config 配置表
 * @return int 0 成功 -1 失败
 */
int config_table_storage_get(const ConfigTableStorageOps *ops,
                             ConfigTable_t *config);
/**
 * @brief 配置表存储操作
 * @param ops 存储操作符
 * @param default_config 默认配置表
 * @return int 0 成功 -1 失败
 */
int config_table_storage_init_if_empty(
    const ConfigTableStorageOps *ops,
    const ConfigTable_t *default_config);

#endif
