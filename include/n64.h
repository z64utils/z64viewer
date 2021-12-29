/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#ifndef Z64_N64_H_INCLUDED
#define Z64_N64_H_INCLUDED

#include <Light.h>

#define F3DEX_GBI_2
#include "gbi.h"

enum n64_zmode {
	ZMODE_OPA,
	ZMODE_INTER,
	ZMODE_XLU,
	ZMODE_DEC,
	ZMODE_ALL
};

enum n64_geoLayer {
	GEOLAYER_ALL,
	GEOLAYER_OPAQUE,
	GEOLAYER_OVERLAY
};

void n64_set_segment(int seg, void* data);
void* n64_virt2phys(unsigned int segaddr);
unsigned int n64_phys2virt(void* cmd);
void n64_draw(void* dlist);
void n64_set_onlyZmode(enum n64_zmode zmode);
void n64_set_onlyGeoLayer(enum n64_geoLayer geoLayer);

void n64_setMatrix_model(void* data);
void n64_setMatrix_view(void* data);
void n64_setMatrix_projection(void* data);

void n64_set_fog(float fog[2], float color[3]);
void n64_set_lights(float lights[16]);
void n64_clear_lights(void);
bool n64_add_light(LightInfo* lightInfo);

void n64_clearShaderCache(void);

#define gSPMatrix(mtx)            n64_setMatrix_model(mtx)
#define gSPDisplayListSeg(dl)     n64_draw(n64_virt2phys(dl))
#define gSPDisplayList(dl)        n64_draw(dl)
#define gSPSegment(sed, data)     n64_set_segment(sed, data)
#define SEGMENTED_TO_VIRTUAL(seg) n64_virt2phys(seg)

#define n64_ClearSegments() for (int i = 0x8; i < 0x10; ++i) \
	gSPSegment(i, 0)

typedef struct {
	_Alignas(8)
	uint32_t hi;
	uint32_t lo;
} Gfx;

#if 0 // Bloated Assembly

#define gDisplayListPut(gdl,...)                    \
	({                                              \
		Gfx Gdl__[] = { __VA_ARGS__ };              \
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) /  \
		sizeof(*Gdl__); ++Gi__) {                   \
			Gdl__[Gi__].hi = u32r(&Gdl__[Gi__].hi); \
			Gdl__[Gi__].lo = u32r(&Gdl__[Gi__].lo); \
			*(Gfx*)(gdl) = Gdl__[Gi__];             \
		}                                           \
		(void)0;                                    \
	})
#else // Bloated Assembly

#define gDisplayListPut(gdl,...)                    \
	({                                              \
		Gfx Gdl__[] = { __VA_ARGS__ }, * wow = gdl; \
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) /  \
		sizeof(*Gdl__); ++Gi__,++wow)               \
		(*(Gfx*)(wow)).hi = u32r(&Gdl__[Gi__].hi),  \
		(*(Gfx*)(wow)).lo = u32r(&Gdl__[Gi__].lo);  \
		for (size_t Gi__ = 1; Gi__<sizeof(Gdl__) /  \
		sizeof(*Gdl__); ++Gi__)                     \
		(void)(gdl);                                \
		(void)0;                                    \
	})
#endif

#endif
