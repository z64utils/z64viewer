#ifndef __Z64LIGHT_H__
#define __Z64LIGHT_H__
#include <HermosauhuLib.h>

typedef struct {
	u8 hue;
	u8 saturation;
	u8 luminance;
} HLS8;

typedef struct {
	u8 hue;
	u8 saturation;
	u8 luminance;
	u8 alpha;
} HLSA8;

typedef struct {
	u8 r;
	u8 g;
	u8 b;
} RGB8;

typedef struct {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} RGBA8;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
} RGB32;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
	f32 a;
} RGBA32;

typedef struct {
	RGB32 ambient;
} LightContext;

void Light_Scene_SetLights(MemFile* zScene, LightContext* lightCtx);

#endif