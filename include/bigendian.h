#ifndef Z64_BIGENDIAN_H_INCLUDED
#define Z64_BIGENDIAN_H_INCLUDED

#include <stdint.h>

uint32_t u32r(const void* d);
uint16_t u16r(const void* d);
uint8_t u8r(const void* d);
int32_t s32r(const void* d);
int16_t s16r(const void* d);
int8_t s8r(const void* d);

#endif
