#ifndef Z64_BIGENDIAN_H_INCLUDED
#define Z64_BIGENDIAN_H_INCLUDED

#include <stdint.h>

static uint32_t u32r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static uint16_t u16r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 8) | b[1];
}

static uint8_t u8r(void* d) {
	uint8_t* b = d;
	
	return *b;
}

static int32_t s32r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static int16_t s16r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 8) | b[1];
}

static int8_t s8r(void* d) {
	uint8_t* b = d;
	
	return *b;
}

#endif
