#include <stdint.h>

uint32_t u32r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

uint16_t u16r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 8) | b[1];
}

uint8_t u8r(void* d) {
	uint8_t* b = d;
	
	return *b;
}

int32_t s32r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

int16_t s16r(void* d) {
	uint8_t* b = d;
	
	return (b[0] << 8) | b[1];
}

int8_t s8r(void* d) {
	uint8_t* b = d;
	
	return *b;
}
