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

typedef struct {
	struct {
		float x, y, z;
	} v[3];
	struct {
		float x, y, z;
	} n[3];
	bool cullBackface;
	bool cullFrontface;
} n64_triangleCallbackData;

typedef void (*n64_triangleCallbackFunc)(
	void* userData,
	const n64_triangleCallbackData* tri
);

typedef struct {
	float x, y, z, w;
	struct {
		float u, v;
	} texcoord0, texcoord1;
	float r, g, b, a;
	struct {
		float x, y, z;
	} n;
} n64_cullingCallbackData;

typedef bool (*n64_cullingCallbackFunc)(
	void* userData,
	const n64_cullingCallbackData* vtx,
	uint32_t numVtx
);

void n64_set_segment(int seg, void* data);
void* n64_virt2phys(unsigned int segaddr);
unsigned int n64_phys2virt(void* cmd);
void n64_draw(void* dlist);
void n64_draw_buffers(void);
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
void n64_set_triangleCallbackFunc(void* userData, n64_triangleCallbackFunc callback);
void n64_set_cullingCallbackFunc(void* userData, n64_cullingCallbackFunc callback);
Gfx n64_gbi_gfxhi_ptr(void* ptr);
Gfx n64_gbi_gfxhi_seg(uint32_t seg);
bool n64_set_culling(bool state);
void n64_reset_buffers(void);
void n64_graph_init();
void* n64_gbi(size_t gbiNum, ...);
#define n64_gbi(...) n64_gbi(N64_VA_ARG_NUM(__VA_ARGS__), __VA_ARGS__)

#define gxSPMatrix(mtx)           { Assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); gSPMatrix(gPolyOpaDisp++, mtx, G_MTX_LOAD); }
#define gxSPDisplayListSeg(dl)    gxSPDisplayList((dl))
#define SEGMENTED_TO_VIRTUAL(seg) n64_virt2phys(seg)
#define gxSPDisplayList(dl)													  \
		{																	  \
			_assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
			gDisplayList(gPolyOpaDisp++, dl, 0);							  \
		}
#define gxSPSegment(disp, sed, data)										  \
		{																	  \
			_assert(gPolyOpaDisp - gPolyOpaHead < ARRAY_COUNT(gPolyOpaHead)); \
			gSPSegment(disp, sed, data);									  \
			gSegment[sed] = data;											  \
		}

#define n64_clear_segments() for (int i = 0x8; i < 0x10; ++i) \
		gSegment[i] = NULL

#if 0 // Bloated Assembly

#define gDisplayListPut(gdl, ...)					\
		({											\
		Gfx Gdl__[] = { __VA_ARGS__ };				\
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) /	\
		sizeof(*Gdl__); ++Gi__) {					\
			Gdl__[Gi__].hi = u32r(&Gdl__[Gi__].hi);	\
			Gdl__[Gi__].lo = u32r(&Gdl__[Gi__].lo);	\
			*(Gfx*)(gdl) = Gdl__[Gi__];				\
		}											\
		(void)0;									\
	})
#else // Bloated Assembly

#define gDisplayListPut(gdl, ...)					\
		({											\
		Gfx Gdl__[] = { __VA_ARGS__ }, * wow = gdl;	\
		for (size_t Gi__ = 0; Gi__<sizeof(Gdl__) /	\
		sizeof(*Gdl__); ++Gi__, ++wow)				\
		(*(Gfx*)(wow)).hi = u32r(&Gdl__[Gi__].hi),	\
		(*(Gfx*)(wow)).lo = u32r(&Gdl__[Gi__].lo);	\
		for (size_t Gi__ = 1; Gi__<sizeof(Gdl__) /	\
		sizeof(*Gdl__); ++Gi__)						\
		(void)(gdl);								\
		(void)0;									\
	})
#endif

#define N64_VA_ARG_SEQ(										\
			_1, _2, _3, _4, _5, _6, _7, _8,					\
			_9, _10, _11, _12, _13, _14, _15, _16,			\
			_17, _18, _19, _20, _21, _22, _23, _24,			\
			_25, _26, _27, _28, _29, _30, _31, _32,			\
			_33, _34, _35, _36, _37, _38, _39, _40,			\
			_41, _42, _43, _44, _45, _46, _47, _48,			\
			_49, _50, _51, _52, _53, _54, _55, _56,			\
			_57, _58, _59, _60, _61, _62, _63, _64,			\
			_65, _66, _67, _68, _69, _70, _71, _72,			\
			_73, _74, _75, _76, _77, _78, _79, _80,			\
			_81, _82, _83, _84, _85, _86, _87, _88,			\
			_89, _90, _91, _92, _93, _94, _95, _96,			\
			_97, _98, _99, _100, _101, _102, _103, _104,	\
			_105, _106, _107, _108, _109, _110, _111, _112,	\
			_113, _114, _115, _116, _117, _118, _119, _120,	\
			_121, _122, _123, _124, _125, _126, _127, _128, N, ...) N
#define N64_VA_ARG_NUM(...)							 \
		N64_VA_ARG_SEQ(								 \
			__VA_ARGS__								 \
			, 128, 127, 126, 125, 124, 123, 122, 121 \
			, 120, 119, 118, 117, 116, 115, 114, 113 \
			, 112, 111, 110, 109, 108, 107, 106, 105 \
			, 104, 103, 102, 101, 100, 99, 98, 97	 \
			, 96, 95, 94, 93, 92, 91, 90, 89		 \
			, 88, 87, 86, 85, 84, 83, 82, 81		 \
			, 80, 79, 78, 77, 76, 75, 74, 73		 \
			, 72, 71, 70, 69, 68, 67, 66, 65		 \
			, 64, 63, 62, 61, 60, 59, 58, 57		 \
			, 56, 55, 54, 53, 52, 51, 50, 49		 \
			, 48, 47, 46, 45, 44, 43, 42, 41		 \
			, 40, 39, 38, 37, 36, 35, 34, 33		 \
			, 32, 31, 30, 29, 28, 27, 26, 25		 \
			, 24, 23, 22, 21, 20, 19, 18, 17		 \
			, 16, 15, 14, 13, 12, 11, 10, 9			 \
			, 8, 7, 6, 5, 4, 3, 2, 1				 \
		)

#endif
