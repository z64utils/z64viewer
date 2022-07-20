#ifndef Z64_BIGENDIAN_H_INCLUDED
#define Z64_BIGENDIAN_H_INCLUDED

#include <stdint.h>

uint32_t u32r(void* d);
uint16_t u16r(void* d);
uint8_t u8r(void* d);
int32_t s32r(void* d);
int16_t s16r(void* d);
int8_t s8r(void* d);

#endif
