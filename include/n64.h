/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#include <stdbool.h>

#ifndef Z64_N64_H_INCLUDED
#define Z64_N64_H_INCLUDED

#define MATRIX_STACK_MAX 16
#define SEGMENT_MAX      16
#define VBUF_MAX         32
#define GRAPH_INIT       0
#define TRI_INIT         0

#define CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define ARRAY_COUNT(arr)     (uint32_t)(sizeof(arr) / sizeof(arr[0]))

#define POLY_OPA_DISP gPolyOpaDisp
#define POLY_XLU_DISP gPolyXluDisp

#ifndef F3DEX_GBI_2
#define F3DEX_GBI_2
#endif
#include "gbi.h"

extern Gfx gPolyOpaHead[4096];
extern Gfx* gPolyOpaDisp;
extern Gfx gPolyXluHead[4096];
extern Gfx* gPolyXluDisp;
extern uint8_t gSegCheckBuf[64];

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

extern void* gSegment[SEGMENT_MAX];

typedef void (* TriCallback)(
	/* 0 == init */ int32_t flag,
	/* vertex    */ const void* v0, const void* v1, const void* v2,
	/* normal    */ const void* n0, const void* n1, const void* n2
);

void n64_set_segment(int seg, void* data);
void* n64_virt2phys(unsigned int segaddr);
unsigned int n64_phys2virt(void* cmd);
void n64_draw(void* dlist);
void n64_setMatrix_model(void* data);
void n64_setMatrix_view(void* data);
void n64_setMatrix_projection(void* data);
void n64_set_fog(float fog[2], float color[3]);
bool n64_bind_light(Light* lightInfo, Ambient* ambient);
void n64_set_onlyZmode(enum n64_zmode zmode);
void n64_set_onlyGeoLayer(enum n64_geoLayer geoLayer);
void n64_swap(Gfx* g);
void n64_clearCache();
void* n64_graph_alloc(uint32_t sz);
void n64_set_triangle_buffer_callback(TriCallback callback);
Gfx n64_gbi_gfxhi_ptr(void* ptr);
Gfx n64_gbi_gfxhi_seg(uint32_t seg);
void n64_graph_init();

#define gxSPMatrix(mtx)           { Assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); gSPMatrix(gPolyOpaDisp++, mtx, G_MTX_LOAD); }
#define gxSPDisplayListSeg(dl)    gxSPDisplayList((dl))
#define SEGMENTED_TO_VIRTUAL(seg) n64_virt2phys(seg)
#define gxSPDisplayList(dl) \
	{ \
		Assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
		gDisplayList(gPolyOpaDisp++, dl, 0); \
	}
#define gxSPSegment(disp, sed, data) \
	{ \
		Assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
		gSPSegment(disp, sed, data); \
		gSegment[sed] = data; \
	}

#define n64_clear_segments() for (int i = 0x8; i < 0x10; ++i) \
	gSegment[i] = NULL

#if 0 // Bloated Assembly

#define gDisplayListPut(gdl, ...) \
	({ \
		Gfx Gdl__[] = { __VA_ARGS__ }; \
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) / \
		sizeof(*Gdl__); ++Gi__) { \
			Gdl__[Gi__].hi = u32r(&Gdl__[Gi__].hi); \
			Gdl__[Gi__].lo = u32r(&Gdl__[Gi__].lo); \
			*(Gfx*)(gdl) = Gdl__[Gi__]; \
		} \
		(void)0; \
	})
#else // Bloated Assembly

#define gDisplayListPut(gdl, ...) \
	({ \
		Gfx Gdl__[] = { __VA_ARGS__ }, * wow = gdl; \
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) / \
		sizeof(*Gdl__); ++Gi__, ++wow) \
		(*(Gfx*)(wow)).hi = u32r(&Gdl__[Gi__].hi), \
		(*(Gfx*)(wow)).lo = u32r(&Gdl__[Gi__].lo); \
		for (size_t Gi__ = 1; Gi__<sizeof(Gdl__) / \
		sizeof(*Gdl__); ++Gi__) \
		(void)(gdl); \
		(void)0; \
	})
#endif

#endif
