#ifndef PET_BEHAVIOR_AI_H
#define PET_BEHAVIOR_AI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PET_AI_API_VERSION  0x00020000u
#define PET_AI_SAMPLE_HZ    50u
#define PET_AI_WARMUP_MS    5000u
#define PET_AI_STRIDE_MS    1000u
#define PET_AI_MAX_GAP_MS   100u
#define PET_AI_CONTEXT_ALIGNMENT 4u

typedef enum {
    PET_AI_OK = 0,
    PET_AI_HAS_RESULT = 1,
    PET_AI_ERR_NULL = -1,
    PET_AI_ERR_NOT_INIT = -2,
    PET_AI_ERR_TIME = -3,
    PET_AI_ERR_RANGE = -4
} pet_ai_status_t;

typedef enum {
    PET_AI_STATIC = 1,
    PET_AI_LYING = 2,
    PET_AI_RUNNING = 3,
    PET_AI_SITTING = 4,
    PET_AI_SNIFFING = 5,
    PET_AI_WALKING = 6,
    PET_AI_OTHER = 9
} pet_ai_class_t;

typedef struct {
    uint32_t t_ms;
    int32_t ax;
    int32_t ay;
    int32_t az;
    int32_t gx;
    int32_t gy;
    int32_t gz;
} pet_ai_sample_t;

typedef struct {
    uint32_t t_ms;
    uint8_t class_id;
} pet_ai_result_t;

uint32_t pet_ai_context_size(void);
uint32_t pet_ai_version(void);
int32_t pet_ai_init(void *ctx);
int32_t pet_ai_reset(void *ctx);
int32_t pet_ai_push_sample(void *ctx,
                           const pet_ai_sample_t *sample,
                           pet_ai_result_t *result);

#ifdef __cplusplus
}
#endif

#endif
