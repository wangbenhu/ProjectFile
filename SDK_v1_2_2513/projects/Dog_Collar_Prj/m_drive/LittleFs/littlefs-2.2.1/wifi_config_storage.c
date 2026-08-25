#include "wifi_config_storage.h"

#include <string.h>

static int valid_ops(const WifiConfigStorageOps *ops)
{
    return ops && ops->lock && ops->unlock && ops->ready_locked &&
           ops->replace_locked && ops->read_locked;
}

int wifi_config_storage_set(const WifiConfigStorageOps *ops,
                            const WifiConfig_t *config)
{
    char encoded[WIFI_CONFIG_TEXT_MAX_SIZE];
    int encoded_len;
    int result;

    if (!valid_ops(ops)) {
        return -1;
    }
    encoded_len = wifi_config_encode(config, encoded, sizeof(encoded));
    if (encoded_len < 0) {
        return encoded_len;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->replace_locked(ops->context,
                                 encoded,
                                 (size_t)encoded_len);
    ops->unlock(ops->context);
    return result < 0 ? -12 : 0;
}

int wifi_config_storage_get(const WifiConfigStorageOps *ops,
                            WifiConfig_t *config)
{
    char encoded[WIFI_CONFIG_TEXT_MAX_SIZE];
    size_t encoded_len = 0;
    int result;

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    if (!valid_ops(ops)) {
        return -2;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->read_locked(ops->context,
                              encoded,
                              sizeof(encoded),
                              &encoded_len);
    ops->unlock(ops->context);
    if (result < 0) {
        return -12;
    }

    result = wifi_config_decode(encoded, encoded_len, config);
    if (result < 0) {
        memset(config, 0, sizeof(*config));
        return -13;
    }
    return 0;
}

static int valid_fence_ops(const FenceConfigStorageOps *ops)
{
    return ops && ops->lock && ops->unlock && ops->ready_locked &&
           ops->replace_locked && ops->read_locked;
}

/**
 * @brief 围栏配置表存储操作
 * @param ops 存储操作符
 * @param config 围栏配置表
 * @return int 0 成功 -1 失败
 */
int fence_config_storage_set(const FenceConfigStorageOps *ops,
                             const FenceConfig_t *config)
{
    char encoded[FENCE_CONFIG_TEXT_MAX_SIZE];
    int encoded_len;
    int result;

    if (!valid_fence_ops(ops)) {
        return -1;
    }
    encoded_len = fence_config_encode(config, encoded, sizeof(encoded));
    if (encoded_len < 0) {
        return encoded_len;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->replace_locked(ops->context, encoded, (size_t)encoded_len);
    ops->unlock(ops->context);
    return result != 0 ? -12 : 0;
}

int fence_config_storage_get(const FenceConfigStorageOps *ops,
                             FenceConfig_t *config)
{
    char encoded[FENCE_CONFIG_TEXT_MAX_SIZE];
    size_t encoded_len = 0U;
    int result;

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    if (!valid_fence_ops(ops)) {
        return -2;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->read_locked(ops->context,
                              encoded,
                              sizeof(encoded),
                              &encoded_len);
    ops->unlock(ops->context);
    if (result != 0) {
        return -12;
    }
    result = fence_config_decode(encoded, encoded_len, config);
    if (result < 0) {
        memset(config, 0, sizeof(*config));
        return -13;
    }
    return 0;
}

static int valid_temp_file_replace_ops(const TempFileReplaceOps *ops)
{
    return ops && ops->remove && ops->open && ops->write && ops->sync &&
           ops->close && ops->stat && ops->rename;
}

int temp_file_replace(const TempFileReplaceOps *ops,
                      const char *temp_path,
                      const char *target_path,
                      const char *content,
                      size_t content_len)
{
    int result;
    int close_result;
    int type;
    size_t size;

    if (!valid_temp_file_replace_ops(ops) || !temp_path || !target_path ||
        !content || content_len == 0U) {
        return -1;
    }

    result = ops->remove(ops->context, temp_path);
    if (result != 0 && result != ops->not_found_error) {
        return -2;
    }

    result = ops->open(ops->context, temp_path);
    if (result != 0) {
        ops->remove(ops->context, temp_path);
        return -2;
    }

    result = ops->write(ops->context, content, content_len);
    if (result < 0 || (size_t)result != content_len) {
        ops->close(ops->context);
        ops->remove(ops->context, temp_path);
        return -3;
    }

    result = ops->sync(ops->context);
    close_result = ops->close(ops->context);
    if (result != 0 || close_result != 0) {
        ops->remove(ops->context, temp_path);
        return -4;
    }

    result = ops->stat(ops->context, temp_path, &type, &size);
    if (result != 0 || type != ops->regular_file_type ||
        size != content_len) {
        ops->remove(ops->context, temp_path);
        return -5;
    }

    result = ops->rename(ops->context, temp_path, target_path);
    if (result != 0) {
        ops->remove(ops->context, temp_path);
        return -6;
    }
    return 0;
}

static int valid_config_table_ops(const ConfigTableStorageOps *ops)
{
    return ops && ops->lock && ops->unlock && ops->ready_locked &&
           ops->replace_locked && ops->read_locked && ops->has_data_locked;
}

int config_table_storage_set(const ConfigTableStorageOps *ops,
                             const ConfigTable_t *config)
{
    char encoded[CONFIG_TABLE_TEXT_MAX_SIZE];
    int encoded_len;
    int result;

    if (!valid_config_table_ops(ops)) {
        return -1;
    }
    encoded_len = config_table_encode(config, encoded, sizeof(encoded));
    if (encoded_len < 0) {
        return encoded_len;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->replace_locked(ops->context,
                                 encoded,
                                 (size_t)encoded_len);
    ops->unlock(ops->context);
    return result < 0 ? -12 : 0;
}

int config_table_storage_get(const ConfigTableStorageOps *ops,
                             ConfigTable_t *config)
{
    char encoded[CONFIG_TABLE_TEXT_MAX_SIZE];
    size_t encoded_len = 0;
    int needs_upgrade = 0;
    int result;

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    if (!valid_config_table_ops(ops)) {
        return -2;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->read_locked(ops->context,
                              encoded,
                              sizeof(encoded),
                              &encoded_len);
    if (result < 0) {
        result = -12;
    } else {
        result = config_table_decode_ex(encoded,
                                        encoded_len,
                                        config,
                                        &needs_upgrade);
        if (result < 0) {
            result = -13;
        } else if (needs_upgrade) {
            result = config_table_encode(config, encoded, sizeof(encoded));
            if (result < 0) {
                result = -14;
            } else {
                result = ops->replace_locked(ops->context,
                                             encoded,
                                             (size_t)result);
                if (result < 0) {
                    result = -15;
                }
            }
        }
    }
    ops->unlock(ops->context);
    if (result < 0) {
        memset(config, 0, sizeof(*config));
    }
    return result;
}

int config_table_storage_init_if_empty(
    const ConfigTableStorageOps *ops,
    const ConfigTable_t *default_config)
{
    char encoded[CONFIG_TABLE_TEXT_MAX_SIZE];
    int encoded_len;
    int has_data;
    int result;

    if (!valid_config_table_ops(ops)) {
        return -1;
    }
    encoded_len = config_table_encode(default_config, encoded, sizeof(encoded));
    if (encoded_len < 0) {
        return encoded_len;
    }
    if (ops->lock(ops->context) != 0) {
        return -10;
    }
    if (ops->ready_locked(ops->context) != 0) {
        ops->unlock(ops->context);
        return -11;
    }
    result = ops->has_data_locked(ops->context, &has_data);
    if (result < 0) {
        ops->unlock(ops->context);
        return -12;
    }
    if (has_data) {
        ops->unlock(ops->context);
        return 0;
    }
    result = ops->replace_locked(ops->context,
                                 encoded,
                                 (size_t)encoded_len);
    ops->unlock(ops->context);
    return result < 0 ? -13 : 1;
}
