#ifndef __M_AUDIO_FRAME_H__
#define __M_AUDIO_FRAME_H__

#include <stdint.h>
#include "sr_config.h"
#include "sr_config_codec.h"
#include "sr_config_audio.h"

/**@brief Compressed audio frame representation. */
typedef struct
{
    uint8_t     data[CONFIG_AUDIO_FRAME_SIZE_BYTES];
    uint8_t     reference_count;
    uint16_t    data_size;
} m_audio_frame_t;

#endif /* __M_AUDIO_FRAME_H__ */