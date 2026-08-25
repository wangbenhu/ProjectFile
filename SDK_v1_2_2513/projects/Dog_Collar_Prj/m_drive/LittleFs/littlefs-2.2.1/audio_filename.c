#include "audio_filename.h"

#include <string.h>

#define AUDIO_USER_PREFIX AUDIO_USER_DIR "/"
#define WAV_SUFFIX ".wav"

static size_t bounded_length(const char *text, size_t limit)
{
    size_t length;

    for (length = 0; length < limit && text[length] != '\0'; ++length) {
    }
    return length;
}

static int valid_audio_filename(const char *filename, size_t length)
{
    size_t i;

    if (length < sizeof(WAV_SUFFIX) ||
        length > AUDIO_FILENAME_MAX_LEN ||
        memcmp(filename + length - (sizeof(WAV_SUFFIX) - 1),
               WAV_SUFFIX,
               sizeof(WAV_SUFFIX) - 1) != 0) {
        return 0;
    }

    for (i = 0; i < length; ++i) {
        if (filename[i] == '/' || filename[i] == '\\' ||
            (filename[i] == '.' && i + 1 < length &&
             filename[i + 1] == '.')) {
            return 0;
        }
    }
    return 1;
}

int audio_build_user_path(const char *filename,
                          char *path,
                          size_t path_size)
{
    size_t filename_len;
    size_t prefix_len = sizeof(AUDIO_USER_PREFIX) - 1;
    size_t total_len;

    if (!path || path_size == 0) {
        return -1;
    }
    path[0] = '\0';
    if (!filename) {
        return -1;
    }

    filename_len = bounded_length(filename, AUDIO_FILENAME_MAX_LEN + 1);
    if (!valid_audio_filename(filename, filename_len)) {
        return -2;
    }

    total_len = prefix_len + filename_len;
    if (total_len + 1 > path_size) {
        return -3;
    }

    memcpy(path, AUDIO_USER_PREFIX, prefix_len);
    memcpy(path + prefix_len, filename, filename_len);
    path[total_len] = '\0';
    return 0;
}
