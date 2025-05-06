#ifndef __CRC8_H
#define __CRC8_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define CHECKSUM_FACTOR			0x5A
//校验值计算
uint8_t CheckSum_Count_Get(uint8_t *data,uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

