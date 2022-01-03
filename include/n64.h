/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#ifndef Z64_N64_H_INCLUDED
#define Z64_N64_H_INCLUDED

#define MATRIX_STACK_MAX 16
#define SEGMENT_MAX      16
#define VBUF_MAX         32
#define GRAPH_INIT       0
#define TRI_INIT         0

#define POLY_OPA_DISP gPolyOpaDisp
#define POLY_XLU_DISP gPolyOpaDisp

#include <Light.h>

#ifndef F3DEX_GBI_2
	#define F3DEX_GBI_2
#endif
#include "gbi.h"

typedef struct Vtx {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t pad;
	int16_t u;
	int16_t v;
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		} color;
		struct {
			int8_t  x;
			int8_t  y;
			int8_t  z;
			uint8_t alpha;
		} normal;
	} ext;
} Vtx;

typedef struct VtxF {
	Vec4f pos;
	struct {
		f32 u;
		f32 v;
	} texcoord0, texcoord1;
	Vec4f color;
	struct {
		f32 x;
		f32 y;
		f32 z;
	} norm;
} VtxF;

typedef struct {
	Vec3f p[3];
	Vec3f n[3];
} Tri;

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

typedef struct {
	_Alignas(8)
	uint32_t hi;
	uint32_t lo;
} Gfx;

extern void* gSegment[SEGMENT_MAX];

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
void n64_swap(Gfx* g);

void* n64_graph_alloc(u32 sz);
void n64_assign_triangle(s32 flag);

Gfx n64_gbi_gfxhi_ptr(void* ptr);
Gfx n64_gbi_gfxhi_seg(u32 seg);

Gfx* Gfx_TwoTexScroll(s32 t1, u16 x1, u16 y1, s16 w1, s16 h1, s32 t2, u16 x2, u16 y2, s16 w2, s16 h2);
Gfx* Gfx_TexScroll(u32 x, u32 y, s32 width, s32 height);
Gfx* Gfx_TwoTexScrollEnvColor(s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2, u32 y2, s32 width2, s32 height2, s32 r, s32 g, s32 b, s32 a);
Gfx* Gfx_TwoTexScrollPrimColor(s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2, u32 y2, s32 width2, s32 height2, s32 r, s32 g, s32 b, s32 a);

extern uintptr_t gStorePointer;
extern Gfx gPolyOpaHead[4096];
extern Gfx* gPolyOpaDisp;
extern Tri gTriHead[1024 * 256];
extern u32 gTriCur;

static inline
void* Graph_Alloc(u32 sz) {
	return n64_graph_alloc(sz);
}

#define gxSPMatrix(mtx)           { OsAssert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); gSPMatrix(gPolyOpaDisp++, mtx, G_MTX_LOAD); }
#define gxSPDisplayListSeg(dl)    gxSPDisplayList((dl))
#define SEGMENTED_TO_VIRTUAL(seg) n64_virt2phys(seg)
#define gxSPDisplayList(dl) \
	{ \
		OsAssert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
		gDisplayList(gPolyOpaDisp++, dl, 0); \
	}
#define gxSPSegment(disp, sed, data) \
	{ \
		OsAssert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
		gSPSegment(disp, sed, data); \
		gSegment[sed] = data; \
	}

#define n64_ClearSegments() for (int i = 0x8; i < 0x10; ++i) \
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
