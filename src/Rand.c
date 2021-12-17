#include <Rand.h>

// The latest generated random number, used to generate the next number in the sequence.
static u32 sRandInt = 1;

// Space to store a value to be re-interpreted as a float.
static u32 sRandFloat;

void Rand_Seed(u32 seed) {
	sRandInt = seed;
}

f32 Rand_ZeroOne(void) {
	sRandInt = (sRandInt * 1664525) + 1013904223;
	sRandFloat = ((sRandInt >> 9) | 0x3F800000);
	
	return *((f32*)&sRandFloat) - 1.0f;
}

s16 Rand_S16Offset(s16 base, s16 range) {
	return (s16)(Rand_ZeroOne() * range) + base;
}