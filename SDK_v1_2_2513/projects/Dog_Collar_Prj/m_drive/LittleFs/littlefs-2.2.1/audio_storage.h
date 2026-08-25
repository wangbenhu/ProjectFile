#ifndef AUDIO_STORAGE_H
#define AUDIO_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "audio_filename.h"

typedef enum {
    AUDIO_STORAGE_WRITE_OVERWRITE = 0,
    AUDIO_STORAGE_WRITE_APPEND
} AudioStorageWriteMode;

typedef struct {
    void *context;
    int (*lock)(void *context);
    void (*unlock)(void *context);
    int (*ready_locked)(void *context);
    int (*stat_locked)(void *context,
                       const char *path,
                       uint32_t *size,
                       int *is_regular);
    int (*mkdir_locked)(void *context);
    int (*open_locked)(void *context,
                       const char *path,
                       AudioStorageWriteMode mode);
    int (*write_locked)(void *context, const uint8_t *data, size_t len);
    int (*sync_locked)(void *context);
    int (*close_locked)(void *context);
    int (*remove_locked)(void *context, const char *path);
} AudioStorageOps;

typedef struct {
    void *context;
    int (*get_size)(void *context, uint32_t address, uint32_t *size);
    int (*read)(void *context,
                uint32_t address,
                uint8_t *buffer,
                size_t len);
    int (*write)(void *context,
                 const char *filename,
                 const uint8_t *data,
                 size_t len,
                 AudioStorageWriteMode mode);
} AudioFlashCopyOps;

bool audio_storage_exist(const AudioStorageOps *ops, const char *filename);
int audio_storage_get_size(const AudioStorageOps *ops,
                           const char *filename,
                           uint32_t *size);
int audio_storage_write(const AudioStorageOps *ops,
                        const char *filename,
                        const uint8_t *data,
                        size_t len,
                        AudioStorageWriteMode mode);
int audio_storage_delete(const AudioStorageOps *ops, const char *filename);
int audio_storage_copy_from_flash(const AudioFlashCopyOps *ops,
                                  uint32_t flash_addr,
                                  const char *filename);

#endif
