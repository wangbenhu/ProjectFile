#ifndef AUDIO_FILENAME_H
#define AUDIO_FILENAME_H

#include <stddef.h>

#define AUDIO_USER_DIR "/audio/user"
#define AUDIO_FILENAME_MAX_LEN 48
#define AUDIO_USER_PATH_MAX_LEN 64

int audio_build_user_path(const char *filename,
                          char *path,
                          size_t path_size);

#endif
