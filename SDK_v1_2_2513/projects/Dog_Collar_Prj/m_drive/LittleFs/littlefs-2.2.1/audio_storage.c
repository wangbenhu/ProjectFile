#include "audio_storage.h"

#include <limits.h>

#define AUDIO_FLASH_COPY_CHUNK 1024U

static int audio_storage_ops_valid(const AudioStorageOps *ops)
{
    return ops && ops->lock && ops->unlock && ops->ready_locked &&
           ops->stat_locked && ops->mkdir_locked && ops->open_locked &&
           ops->write_locked && ops->sync_locked && ops->close_locked &&
           ops->remove_locked;
}

static int audio_storage_lock_ready(const AudioStorageOps *ops)
{
    if (ops->lock(ops->context) < 0) {
        return -1;
    }
    if (ops->ready_locked(ops->context) < 0) {
        ops->unlock(ops->context);
        return -1;
    }
    return 0;
}

bool audio_storage_exist(const AudioStorageOps *ops, const char *filename)
{
    char path[AUDIO_USER_PATH_MAX_LEN];
    uint32_t size = 0U;
    int is_regular = 0;
    int result;

    if (!audio_storage_ops_valid(ops) ||
        audio_build_user_path(filename, path, sizeof(path)) < 0 ||
        audio_storage_lock_ready(ops) < 0) {
        return false;
    }

    result = ops->stat_locked(ops->context, path, &size, &is_regular);
    ops->unlock(ops->context);
    return result == 0 && is_regular != 0;
}

int audio_storage_get_size(const AudioStorageOps *ops,
                           const char *filename,
                           uint32_t *size)
{
    char path[AUDIO_USER_PATH_MAX_LEN];
    uint32_t found_size = 0U;
    int is_regular = 0;
    int result;

    if (size) {
        *size = 0U;
    }
    if (!size || !audio_storage_ops_valid(ops) ||
        audio_build_user_path(filename, path, sizeof(path)) < 0) {
        return -1;
    }
    if (audio_storage_lock_ready(ops) < 0) {
        return -2;
    }

    result = ops->stat_locked(ops->context,
                              path,
                              &found_size,
                              &is_regular);
    ops->unlock(ops->context);
    if (result < 0) {
        return -3;
    }
    if (!is_regular) {
        return -4;
    }

    *size = found_size;
    return 0;
}

int audio_storage_write(const AudioStorageOps *ops,
                        const char *filename,
                        const uint8_t *data,
                        size_t len,
                        AudioStorageWriteMode mode)
{
    char path[AUDIO_USER_PATH_MAX_LEN];
    int written;
    int sync_result;
    int close_result;

    if (!data || len == 0U) {
        return -1;
    }
    if (!audio_storage_ops_valid(ops) ||
        audio_build_user_path(filename, path, sizeof(path)) < 0) {
        return -2;
    }
    if (mode != AUDIO_STORAGE_WRITE_OVERWRITE &&
        mode != AUDIO_STORAGE_WRITE_APPEND) {
        return -3;
    }
    if (audio_storage_lock_ready(ops) < 0) {
        return -4;
    }
    if (ops->mkdir_locked(ops->context) < 0) {
        ops->unlock(ops->context);
        return -5;
    }
    if (ops->open_locked(ops->context, path, mode) < 0) {
        ops->unlock(ops->context);
        return -6;
    }

    written = ops->write_locked(ops->context, data, len);
    sync_result = written >= 0 ? ops->sync_locked(ops->context) : 0;
    close_result = ops->close_locked(ops->context);
    ops->unlock(ops->context);

    if (written < 0) {
        return -7;
    }
    if ((size_t)written != len) {
        return -8;
    }
    if (sync_result < 0) {
        return -9;
    }
    if (close_result < 0) {
        return -10;
    }
    return 0;
}

int audio_storage_delete(const AudioStorageOps *ops, const char *filename)
{
    char path[AUDIO_USER_PATH_MAX_LEN];
    uint32_t size = 0U;
    int is_regular = 0;
    int result;

    if (!audio_storage_ops_valid(ops) ||
        audio_build_user_path(filename, path, sizeof(path)) < 0) {
        return -1;
    }
    if (audio_storage_lock_ready(ops) < 0) {
        return -2;
    }

    result = ops->stat_locked(ops->context, path, &size, &is_regular);
    if (result < 0) {
        ops->unlock(ops->context);
        return -3;
    }
    if (!is_regular) {
        ops->unlock(ops->context);
        return -4;
    }

    result = ops->remove_locked(ops->context, path);
    ops->unlock(ops->context);
    return result < 0 ? -5 : 0;
}

int audio_storage_copy_from_flash(const AudioFlashCopyOps *ops,
                                  uint32_t flash_addr,
                                  const char *filename)
{
    uint8_t buffer[AUDIO_FLASH_COPY_CHUNK];
    char path[AUDIO_USER_PATH_MAX_LEN];
    uint32_t file_size;
    uint32_t offset = 0U;
    size_t chunk;
    AudioStorageWriteMode mode;

    if (!ops || !ops->get_size || !ops->read || !ops->write ||
        audio_build_user_path(filename, path, sizeof(path)) < 0) {
        return -1;
    }
    if (ops->get_size(ops->context, flash_addr, &file_size) < 0 ||
        file_size == 0U ||
        file_size > UINT32_MAX - flash_addr) {
        return -2;
    }

    while (offset < file_size) {
        chunk = (size_t)(file_size - offset);
        if (chunk > AUDIO_FLASH_COPY_CHUNK) {
            chunk = AUDIO_FLASH_COPY_CHUNK;
        }
        if (ops->read(ops->context,
                      flash_addr + offset,
                      buffer,
                      chunk) < 0) {
            return -3;
        }

        mode = offset == 0U ? AUDIO_STORAGE_WRITE_OVERWRITE
                            : AUDIO_STORAGE_WRITE_APPEND;
        if (ops->write(ops->context,
                       filename,
                       buffer,
                       chunk,
                       mode) < 0) {
            return -4;
        }
        offset += (uint32_t)chunk;
    }
    return 0;
}
