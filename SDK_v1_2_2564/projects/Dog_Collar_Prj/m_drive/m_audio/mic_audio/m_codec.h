#ifndef __M_CODEC_H__
#define __M_CODEC_H__

void codec_init(void);
int32_t codec_encode(const uint8_t *input, uint16_t input_len, void *output, uint16_t output_len);

#endif
