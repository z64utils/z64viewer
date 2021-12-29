/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#ifndef Z64_N64_H_INCLUDED
#define Z64_N64_H_INCLUDED

#include <Light.h>

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

#define G_TX_MIRROR 1
#define G_TX_CLAMP  2

/* color combiner */
#define G_CCMUX_COMBINED        0
#define G_CCMUX_TEXEL0          1
#define G_CCMUX_TEXEL1          2
#define G_CCMUX_PRIMITIVE       3
#define G_CCMUX_SHADE           4
#define G_CCMUX_ENVIRONMENT     5
#define G_CCMUX_1               6
#define G_CCMUX_NOISE           7
#define G_CCMUX_0               31
#define G_CCMUX_CENTER          6
#define G_CCMUX_K4              7
#define G_CCMUX_SCALE           6
#define G_CCMUX_COMBINED_ALPHA  7
#define G_CCMUX_TEXEL0_ALPHA    8
#define G_CCMUX_TEXEL1_ALPHA    9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA     11
#define G_CCMUX_ENV_ALPHA       12
#define G_CCMUX_LOD_FRACTION    13
#define G_CCMUX_PRIM_LOD_FRAC   14
#define G_CCMUX_K5              15
#define G_ACMUX_COMBINED        0
#define G_ACMUX_TEXEL0          1
#define G_ACMUX_TEXEL1          2
#define G_ACMUX_PRIMITIVE       3
#define G_ACMUX_SHADE           4
#define G_ACMUX_ENVIRONMENT     5
#define G_ACMUX_1               6
#define G_ACMUX_0               7
#define G_ACMUX_LOD_FRACTION    0
#define G_ACMUX_PRIM_LOD_FRAC   6

#define G_MTX_NOPUSH     0x00
#define G_MTX_PUSH       0x01
#define G_MTX_MUL        0x00
#define G_MTX_LOAD       0x02
#define G_MTX_MODELVIEW  0x00
#define G_MTX_PROJECTION 0x04

#define G_ZBUFFER            0b00000000000000000000000000000001
#define G_SHADE              0b00000000000000000000000000000100
#define G_CULL_FRONT         0b00000000000000000000001000000000
#define G_CULL_BACK          0b00000000000000000000010000000000
#define G_FOG                0b00000000000000010000000000000000
#define G_LIGHTING           0b00000000000000100000000000000000
#define G_TEXTURE_GEN        0b00000000000001000000000000000000
#define G_TEXTURE_GEN_LINEAR 0b00000000000010000000000000000000
#define G_SHADING_SMOOTH     0b00000000001000000000000000000000
#define G_CLIPPING           0b00000000100000000000000000000000

/* commands for f3dex2 */
#define F3DEX_GBI_2
#if defined(F3DEX_GBI_2)
# define G_NOOP           0x00
# define G_VTX            0x01
# define G_MODIFYVTX      0x02
# define G_CULLDL         0x03
# define G_BRANCH_Z       0x04
# define G_TRI1           0x05
# define G_TRI2           0x06
# define G_QUAD           0x07
# define G_LINE3D         0x08
# define G_SPECIAL_3      0xD3
# define G_SPECIAL_2      0xD4
# define G_SPECIAL_1      0xD5
# define G_DMA_IO         0xD6
# define G_TEXTURE        0xD7
# define G_POPMTX         0xD8
# define G_GEOMETRYMODE   0xD9
# define G_MTX            0xDA
# define G_MOVEWORD       0xDB
# define G_MOVEMEM        0xDC
# define G_LOAD_UCODE     0xDD
# define G_DL             0xDE
# define G_ENDDL          0xDF
# define G_SPNOOP         0xE0
# define G_RDPHALF_1      0xE1
# define G_SETOTHERMODE_L 0xE2
# define G_SETOTHERMODE_H 0xE3
# define G_RDPHALF_2      0xF1
#endif

/* rdp commands */
#define G_TEXRECT         0xE4
#define G_TEXRECTFLIP     0xE5
#define G_RDPLOADSYNC     0xE6
#define G_RDPPIPESYNC     0xE7
#define G_RDPTILESYNC     0xE8
#define G_RDPFULLSYNC     0xE9
#define G_SETKEYGB        0xEA
#define G_SETKEYR         0xEB
#define G_SETCONVERT      0xEC
#define G_SETSCISSOR      0xED
#define G_SETPRIMDEPTH    0xEE
#define G_RDPSETOTHERMODE 0xEF
#define G_LOADTLUT        0xF0
#define G_SETTILESIZE     0xF2
#define G_LOADBLOCK       0xF3
#define G_LOADTILE        0xF4
#define G_SETTILE         0xF5
#define G_FILLRECT        0xF6
#define G_SETFILLCOLOR    0xF7
#define G_SETFOGCOLOR     0xF8
#define G_SETBLENDCOLOR   0xF9
#define G_SETPRIMCOLOR    0xFA
#define G_SETENVCOLOR     0xFB
#define G_SETCOMBINE      0xFC
#define G_SETTIMG         0xFD
#define G_SETZIMG         0xFE
#define G_SETCIMG         0xFF

/* data types and structures */
typedef uint8_t qu08_t;
typedef uint16_t qu016_t;
typedef int16_t qs48_t;
typedef int16_t qs510_t;
typedef uint16_t qu510_t;
typedef int16_t qs102_t;
typedef uint16_t qu102_t;
typedef int16_t qs105_t;
typedef uint16_t qu105_t;
typedef int16_t qs132_t;
typedef int16_t qs142_t;
typedef int32_t qs1516_t;
typedef int32_t qs1616_t;
typedef int32_t qs205_t;

typedef uint16_t g_bglt_t;
typedef uint8_t g_ifmt_t;
typedef uint8_t g_isiz_t;
typedef uint16_t g_bgf_t;
typedef uint8_t g_objf_t;
typedef uint32_t g_objlt_t;

typedef struct {
	_Alignas(8)
	uint32_t hi;
	uint32_t lo;
} Gfx;

/* fixed-point conversion macros */
#define qu08(n)   ((qu08_t)((n) * 0x100))
#define qu016(n)  ((qu016_t)((n) * 0x10000))
#define qs48(n)   ((qs48_t)((n) * 0x0100))
#define qs510(n)  ((qs510_t)((n) * 0x0400))
#define qu510(n)  ((qu510_t)((n) * 0x0400))
#define qs102(n)  ((qs102_t)((n) * 0x0004))
#define qu102(n)  ((qu102_t)((n) * 0x0004))
#define qs105(n)  ((qs105_t)((n) * 0x0020))
#define qu105(n)  ((qu105_t)((n) * 0x0020))
#define qs132(n)  ((qs132_t)((n) * 0x0004))
#define qs142(n)  ((qs142_t)((n) * 0x0004))
#define qs1516(n) ((qs1516_t)((n) * 0x00010000))
#define qs1616(n) ((qs1616_t)((n) * 0x00010000))
#define qs205(n)  ((qs205_t)((n) * 0x00000020))

#define gI_(i)         ((uint32_t)(i))
#define gL_(l)         ((uint64_t)(l))
#define gF_(i,n,s)     ((gI_(i) & ((gI_(1) << (n)) - 1)) << (s))
#define gFL_(l,n,s)    ((gL_(l) & ((gL_(1) << (n)) - 1)) << (s))
#define gO_(opc,hi,lo) ((Gfx) { gF_(opc,8,24) | gI_(hi),gI_(lo) })
#define gD_(gdl,m,...) gDisplayListPut(gdl,m(__VA_ARGS__))

#define gsDPSetTileSize(tile,uls,   \
	    ult,lrs,lrt) gO_( \
		G_SETTILESIZE,                      \
		gF_(uls,12,12) | gF_(ult,12,0),       \
		gF_(tile,3,24) | gF_(lrs,12,12) |      \
		gF_(lrt,12,0) \
)
#define gsDPSetEnvColor(r, g, b, a) \
	gO_( \
		G_SETENVCOLOR, \
		0, \
		gF_(r, 8, 24) | \
		gF_(g, 8, 16) | \
		gF_(b, 8, 8) | \
		gF_(a, 8, 0) \
	)
#define gsDPSetPrimColor(m, l, r, g, b, a) \
	gO_( \
		G_SETPRIMCOLOR, \
		gF_(m, 8, 8) | \
		gF_(l, 8, 0), \
		gF_(r, 8, 24) | \
		gF_(g, 8, 16) | \
		gF_(b, 8, 8) | \
		gF_(a, 8, 0) \
	)
#define gsSPEndDisplayList() gO_(G_ENDDL,0,0)
#define gsDPTileSync()       gO_(G_RDPTILESYNC,0,0)
#define gsDPPipeSync()       gO_(G_RDPPIPESYNC,0,0)

#define gDPSetTileSize(gdl,...)   gD_(gdl,gsDPSetTileSize,__VA_ARGS__)
#define gDPSetEnvColor(gdl, ...)  gD_(gdl,gsDPSetEnvColor,__VA_ARGS__)
#define gDPSetPrimColor(gdl, ...) gD_(gdl,gsDPSetPrimColor,__VA_ARGS__)
#define gDPTileSync(gdl)          gDisplayListPut(gdl,gsDPTileSync())
#define gDPPipeSync(gdl)          gDisplayListPut(gdl,gsDPPipeSync())
#define gSPEndDisplayList(gdl)    gDisplayListPut(gdl,gsSPEndDisplayList())

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
