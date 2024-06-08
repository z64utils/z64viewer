/*
 * n64.c <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 * derived from the information available here:
 * https://wiki.cloudmodding.com/oot/F3DZEX2
 * https://wiki.cloudmodding.com/oot/F3DZEX2/Opcode_Details
 *
 * optimization opportunities:
 * - I pulled n64texconv from zztexview, but that uses per-pixel
 *   callbacks to convert textures; a dedicated converter for each
 *   format tuned for speed would be better
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#include <n64.h>
#include <n64texconv.h>
#include <bigendian.h>
#include <glad/glad.h>

#include "n64types.h"

#define SHADER_SOURCE(...) "#version 330 core\n" # __VA_ARGS__

// TODO is it possible to combine both calculations?
#define N64_RSP_TEXTURE_GEN        0b01000000000000000000
#define N64_RSP_TEXTURE_GEN_LINEAR 0b10000000000000000000

#define UNFOLD_VEC3(v)               (v).x, (v).y, (v).z
#define UNFOLD_VEC3_EXT(v, action)   (v).x action, (v).y action, (v).z action

// new rendering based on UoT (see DataBlobSegmentsPopulateFromMeshNew() in z64scene)
// TODO is super messy and could use some cleanup
// (you can comment out #define RENDERHOOK_UOT to go back to the original method)
#define RENDERHOOK_UOT

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

GbiGfx n64_poly_opa_head[N64_OPA_STACK_SIZE];
GbiGfx* n64_poly_opa_disp;
GbiGfx n64_poly_xlu_head[N64_XLU_STACK_SIZE];
GbiGfx* n64_poly_xlu_disp;
void* n64_segment[N64_SEGMENT_MAX];
bool n64_tick_20fps;

static GLuint gVAO;
static GLuint gVBO;
static GLuint gEBO;
static GLuint gTexel[N64_TEXTURE_CACHE_SIZE];
static GLubyte gIndices[4096];
static N64Vtx sVbuf[N64_VBUF_MAX];
static uint32_t gIndicesUsed = 0;
static int gTexelCacheCount = 0;
static void* gTexelDict[N64_TEXTURE_CACHE_SIZE];
static GLint gFilterMode = GL_LINEAR;

static void* s_tri_callback_data;
static void* s_cull_callback_data;
static N64TriCallback s_tri_callback;
static N64CullCallback s_cull_callback;

static uint32_t gRdpHalf1;
static uint32_t gRdpHalf2;
static uintptr_t gPtrHi = 0;
static bool gPtrHiSet = false;
static Shader* gShader = 0;
static Shader* sOutlineShader = 0;

static bool gHideGeometry = false;
static bool gVertexColors = false;
static bool gFogEnabled = true;
static bool gForceBl = false;
static bool gCvgXalpha = false;
static bool gGxOutline = false;

static bool s_cull_enabled = true;
static int gPolygonOffset = 0;
static enum N64GeoLayer gOnlyThisGeoLayer;
static enum N64ZMode gOnlyThisZmode;
static enum N64ZMode gCurrentZmode;
static ShaderList* sShaderList = 0;
static int sLightNum;
static GbiLightsN sLights;

static struct {
	//float model[16];
	Mtx  view;
	Mtx  normal;
	Mtx  projection;
	Mtx  modelStack[N64_MTX_STACK_SIZE];
	Mtx* modelNow;
} gMatrix;

static struct {
	float fog[2];
	float color[3];
} gFog;

static struct {
	struct {
		void*    data;
		int      level;
		int      on;
		float    scaleS;
		float    scaleT;
		uint16_t uls;
		uint16_t ult;
		uint16_t lrs;
		uint16_t lrt;
		
		int   fmt;
		int   siz;
		int   line;
		int   tmem;
		int   tile;
		int   palette;
		int   cmT;
		int   maskT;
		int   shiftT;
		int   cmS;
		int   maskS;
		int   shiftS;
		float shiftS_m;
		float shiftT_m;
		bool  doUpdate;
	} tile[2];
	struct {
		void* imgaddr;
		int   fmt;
		int   siz;
		int   width;
	} timg;
	uint16_t pal[256]; /* palette */
	int      mtlReady;
	int      texWidth;
	int      texHeight;
	uint32_t othermode_hi;
	uint32_t othermode_lo;
	struct {
		uint32_t hi;
		uint32_t lo;
	} setcombine;
	struct {
		uint32_t hi;
		uint32_t lo;
		float    r;
		float    g;
		float    b;
		float    alpha;
		float    lodfrac;      // TODO not yet populated
	} prim;
	struct {
		uint32_t hi;
		uint32_t lo;
		float    r;
		float    g;
		float    b;
		float    alpha;
	} env;
	struct {
		float   r, g, b, factor;
		uint8_t mode;
	} xhighlight;
	float    lodfrac;          // TODO not yet populated
	float    k4;               // TODO not yet populated
	float    k5;               // TODO not yet populated
	uint32_t geometrymode;
	bool     mixFog;           // Condition to mix fog into the material
	unsigned texgen:2;

#ifdef RENDERHOOK_UOT
	struct {
		uint32_t Dram;
		int Width;
		int Height;
		int RealWidth;
		int RealHeight;
		double ShiftS;
		double ShiftT;
		double S_Scale;
		double T_Scale;
		double TextureHRatio;
		double TextureWRatio;
		// SetTile
		int TexFormat;
		int TexFormatUOT;
		int TexelSize;
		int LineSize;
		int CMT;
		int CMS;
		int MaskS;
		int MaskT;
		int TShiftS;
		int TShiftT;
		// SetTileSize
		qu102_t ULS;
		qu102_t ULT;
		qu102_t LRS;
		qu102_t LRT;
	} textures[2];
	#define Textures(X) gMatState.textures[X]
	int CurrentTex;
	bool MultiTexCoord;
	bool MultiTexture;
	uint8_t history[8];
	int historyI;
	#define CurrentTex gMatState.CurrentTex
	#define MultiTexCoord gMatState.MultiTexCoord
	#define MultiTexture gMatState.MultiTexture
	#define HISTORY_GET(n) \
	gMatState.history[(gMatState.historyI - 1 - (n) < 0) ? (gMatState.historyI - 1 - (n) + ARRLEN(gMatState.history)) : (gMatState.historyI - 1 - (n))]
#endif // RENDERHOOK_UOT

} gMatState; /* material state magic */

const Mtx sClearMtx = {
	.mf[0] = { 1.0f, 0.0f, 0.0f, 0.0f },
	.mf[1] = { 0.0f, 1.0f, 0.0f, 0.0f },
	.mf[2] = { 0.0f, 0.0f, 1.0f, 0.0f },
	.mf[3] = { 0.0f, 0.0f, 0.0f, 1.0f },
};

#ifdef RENDERHOOK_UOT // functions, direct copy-paste from datablobs.c

#define Min(A, B) ((A) < (B) ? (A) : (B))

// gbi extras
/* dl push flag */
#define G_DL_PUSH   0
#define G_DL_NOPUSH 1
/* 10.2 fixed point */
typedef uint16_t qu102_t;
#define qu102_I(x) \
    ((signed int)((x) >> 2))
#define G_SIZ_BYTES(siz) (G_SIZ_BITS(siz) / 8)
#define ALIGN8(x) (((x) + 7) & ~7)

#define READ_32_BE(BYTES, OFFSET) ( \
	(((const uint8_t*)(BYTES))[OFFSET + 0] << 24) | \
	(((const uint8_t*)(BYTES))[OFFSET + 1] << 16) | \
	(((const uint8_t*)(BYTES))[OFFSET + 2] <<  8) | \
	(((const uint8_t*)(BYTES))[OFFSET + 3] <<  0) \
)

#define ARRLEN(X) (sizeof(X) / sizeof(*(X)))
#define FALLTHROUGH __attribute__((fallthrough))
#define SIZEOF_GFX 8
#define SIZEOF_MTX 0x40
#define SIZEOF_VTX 0x10
#define SHIFTL(v, s, w) \
	((uint32_t)(((uint32_t)(v) & ((0x01 << (w)) - 1)) << (s)))
#define SHIFTR(v, s, w) \
	((uint32_t)(((uint32_t)(v) >> (s)) & ((0x01 << (w)) - 1)))
#define ShiftR SHIFTR

// TODO consolidate it all somehow eventually so no duplicate code
static int Pow2(int val)
{
	int i = 1;
	while (i < val)
		i <<= 1;
	return i;
}
static int PowOf(int val)
{
	int num = 1;
	int i = 0;
	while (num < val)
	{
		num <<= 1;
		i += 1;
	}
	return i;
}
static float Fixed2Float(float v, int b)
{
	float FIXED2FLOATRECIP[] = { 0.5F, 0.25F, 0.125F, 0.0625F, 0.03125F
		, 0.015625F, 0.0078125F, 0.00390625F, 0.001953125F, 0.0009765625F
		, 0.00048828125F, 0.000244140625F, 0.000122070313F, 0.0000610351563F
		, 0.0000305175781F, 0.0000152587891F
	};
	return v * FIXED2FLOATRECIP[b - 1];
}
static void CalculateTexSize(int id)
{
	int MaxTexel = 0;
	int Line_Shift = 0;
	
	switch (Textures(id).TexFormatUOT)
	{
		case 0x00: case 0x40:
			MaxTexel = 4096;
			Line_Shift = 4;
			break;
		case 0x60: case 0x80:
			MaxTexel = 8192;
			Line_Shift = 4;
			break;
		case 0x8: case 0x48:
			MaxTexel = 2048;
			Line_Shift = 3;
			break;
		case 0x68: case 0x88:
			MaxTexel = 4096;
			Line_Shift = 3;
			break;
		case 0x10: case 0x70:
			MaxTexel = 2048;
			Line_Shift = 2;
			break;
		case 0x50: case 0x90:
			MaxTexel = 2048;
			Line_Shift = 0;
			break;
		case 0x18:
			MaxTexel = 1024;
			Line_Shift = 2;
			break;
	}
	
	int Line_Width = Textures(id).LineSize << Line_Shift;
	
	int Tile_Width = Textures(id).LRS - Textures(id).ULS + 1;
	int Tile_Height = Textures(id).LRT - Textures(id).ULT + 1;
	
	int Mask_Width = 1 << Textures(id).MaskS;
	int Mask_Height = 1 << Textures(id).MaskT;
	
	int Line_Height = 0;
	if (Line_Width > 0)
		Line_Height = Min(MaxTexel / Line_Width, Tile_Height);
	
	// NPOT
	// FIXME not working
	if (false)
	{
		if (Textures(id).MaskS > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
			Textures(id).RealWidth = Mask_Width;
		else if ((Tile_Width * Tile_Height) <= MaxTexel)
			Textures(id).RealWidth = Tile_Width;
		else
			Textures(id).RealWidth = (Textures(id).Width >> 2) + 1;//Line_Width;
		
		if (Textures(id).MaskT > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
			Textures(id).RealHeight = Mask_Height;
		else if ((Tile_Width * Tile_Height) <= MaxTexel)
			Textures(id).RealHeight = Tile_Height;
		else
			Textures(id).RealHeight = Min(Line_Height, (Textures(id).Height >> 2) + 1);//Line_Height;
		
		#ifndef NDEBUG
		{
			int width = Textures(id).RealWidth;
			int height = Textures(id).RealHeight;
			int siz = Textures(id).TexelSize;
			size_t size = 0; (void)size;
			
			// old method was returning 0 here
			if (siz == G_IM_SIZ_4b) size = (width * height) / 2;
			else size = G_SIZ_BYTES(siz) * width * height;
			
			//if (size > 4096) Die("this shouldn't happen");
		}
		#endif
		
		return;
	}
	
	if (Textures(id).MaskS > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
		Textures(id).Width = Mask_Width;
	else if ((Tile_Width * Tile_Height) <= MaxTexel)
		Textures(id).Width = Tile_Width;
	else
		Textures(id).Width = Line_Width;
	
	if (Textures(id).MaskT > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
		Textures(id).Height = Mask_Height;
	else if ((Tile_Width * Tile_Height) <= MaxTexel)
		Textures(id).Height = Tile_Height;
	else
		Textures(id).Height = Line_Height;
	
	int Clamp_Width = 0;
	int Clamp_Height = 0;
	if (Textures(id).CMS == 1)
		Clamp_Width = Tile_Width;
	else
		Clamp_Width = Textures(id).Width;
	if (Textures(id).CMT == 1)
		Clamp_Height = Tile_Height;
	else
		Clamp_Height = Textures(id).Height;
	
	if (Mask_Width > Textures(id).Width)
	{
		Textures(id).MaskS = PowOf(Textures(id).Width);
		Mask_Width = 1 << Textures(id).MaskS;
	}
	if (Mask_Height > Textures(id).Height)
	{
		Textures(id).MaskT = PowOf(Textures(id).Height);
		Mask_Height = 1 << Textures(id).MaskT;
	}
	
	if (Textures(id).CMS == 2 || Textures(id).CMS == 3)
		Textures(id).RealWidth = Pow2(Clamp_Width);
	else if (Textures(id).CMS == 1)
		Textures(id).RealWidth = Pow2(Mask_Width);
	else
		Textures(id).RealWidth = Pow2(Textures(id).Width);
	
	if (Textures(id).CMT == 2 || Textures(id).CMT == 3)
		Textures(id).RealHeight = Pow2(Clamp_Height);
	else if (Textures(id).CMT == 1)
		Textures(id).RealHeight = Pow2(Mask_Height);
	else
		Textures(id).RealHeight = Pow2(Textures(id).Height);
	
	Textures(id).ShiftS = 1.0f;
	Textures(id).ShiftT = 1.0f;
	
	if (Textures(id).TShiftS > 10)
		Textures(id).ShiftS = (1 << (16 - Textures(id).TShiftS));
	else if (Textures(id).TShiftS > 0)
		Textures(id).ShiftS /= (1 << Textures(id).TShiftS);
	
	if (Textures(id).TShiftT > 10)
		Textures(id).ShiftT = (1 << (16 - Textures(id).TShiftT));
	else if (Textures(id).TShiftT > 0)
		Textures(id).ShiftT /= (1 << Textures(id).TShiftT);
	
	Textures(id).TextureHRatio = ((Textures(id).T_Scale * Textures(id).ShiftT) / 32.0f / Textures(id).RealHeight);
	Textures(id).TextureWRatio = ((Textures(id).S_Scale * Textures(id).ShiftS) / 32.0f / Textures(id).RealWidth);
}
#endif // RENDERHOOK_UOT

#ifdef RENDERHOOK_UOT // implementation

#define RENDERHOOK_UOT_COMMON \
	const uint8_t *data = cmd; (void)data; \
	uint32_t w0 = u32r(data); (void)w0; \
	uint32_t w1 = u32r(data + 4); (void)w1; \

static bool UOT_gbiFunc_settimg(void* cmd)
{
	RENDERHOOK_UOT_COMMON
	
	void* imgaddr = n64_segment_get(w1);
	if (!imgaddr)
	{
		static uint8_t *blank = 0;
		
		if (!blank)
		{
			int mem = 4096;
			
			blank = malloc(mem);
			
			memset(blank, -1, mem);
		}
		
		imgaddr = blank;
		
		//fprintf(stderr, "couldn't %08x, using blank\n", Textures(CurrentTex).Dram);
	}
	gMatState.timg.imgaddr = imgaddr;
	
	/*
	bool palMode = data[8] == G_RDPTILESYNC;
	if (HISTORY_GET(0) == G_SETTILESIZE)
	{
		CurrentTex = 1;
		if (true) // GLExtensions.GLMultiTexture And GLExtensions.GLFragProg
			MultiTexCoord = true;
		else if (false)
			MultiTexCoord = false;
		MultiTexture = true;
	}
	else
	{
		CurrentTex = 0;
		MultiTexCoord = false;
		MultiTexture = false;
	}
	if (palMode)
		Textures(0).Dram = w1;
	else
		Textures(CurrentTex).Dram = w1;
	*/
	
	return false;
}

static bool UOT_gbiFunc_settile(void* cmd)
{
	RENDERHOOK_UOT_COMMON
	
	//If .CMDParams(1) > 0 Then SETTILE(.CMDLow, .CMDHigh)
	{
		//int tile = SHIFTR(w1, 24, 3);
		int tile = SHIFTR(w1, 24, 3);
		if (tile > 1)
			return false;
		CurrentTex = tile;
		
		Textures(CurrentTex).TexFormatUOT = (w0 >> 16) & 0xff; // uot
		Textures(CurrentTex).TexFormat =   SHIFTR(w0, 21, 3);
		Textures(CurrentTex).TexelSize =   SHIFTR(w0, 19, 2);
		Textures(CurrentTex).LineSize =  SHIFTR(w0,  9, 9);
		//tileDescriptors[tile].tmem =  SHIFTR(w0,  0, 9); // unused
		//tileDescriptors[tile].pal =   SHIFTR(w1, 20, 4); // unused
		Textures(CurrentTex).CMT = ShiftR(w1, 18, 2);
		Textures(CurrentTex).CMS = ShiftR(w1, 8, 2);
		Textures(CurrentTex).MaskS = ShiftR(w1, 4, 4);
		Textures(CurrentTex).MaskT = ShiftR(w1, 14, 4);
		Textures(CurrentTex).TShiftS = ShiftR(w1, 0, 4);
		Textures(CurrentTex).TShiftT = ShiftR(w1, 10, 4);
		
	
		
		// getting linesize of 8 on a 32x32 rgba truecolor texture,
		// should be 16, so maybe this math is required?
		if (Textures(CurrentTex).TexelSize == G_IM_SIZ_32b)
			Textures(CurrentTex).LineSize *= 2;
	}
	
	gMatState.tile[CurrentTex].doUpdate = true;
	gMatState.tile[CurrentTex].data = gMatState.timg.imgaddr;
	
	return false;
}

static bool UOT_gbiFunc_settilesize(void* cmd)
{
	RENDERHOOK_UOT_COMMON
	
	int i = data[4];
	if (i > 1)
		return false;
	
	CurrentTex = i;
	
	{
		Textures(CurrentTex).ULS =  SHIFTR(w0, 12, 12);
		Textures(CurrentTex).ULT =  SHIFTR(w0,  0, 12);
		Textures(CurrentTex).LRS =  SHIFTR(w1, 12, 12);
		Textures(CurrentTex).LRT =  SHIFTR(w1,  0, 12);
		Textures(CurrentTex).Width =  ((Textures(CurrentTex).LRS - Textures(CurrentTex).ULS) + 1);
		Textures(CurrentTex).Height = ((Textures(CurrentTex).LRT - Textures(CurrentTex).ULT) + 1);
	}
	
	CalculateTexSize(CurrentTex);
	
	return false;
}

static bool UOT_gbiFunc_texture(void* cmd)
{
	RENDERHOOK_UOT_COMMON
	
	uint32_t w1x = w1 & 0x00ffffff;
	for (int i = 0; i <= 1; ++i)
	{
		if (ShiftR(w1x, 16, 16) < 0xFFFF)
			Textures(i).S_Scale = Fixed2Float(ShiftR(w1x, 16, 16), 16);
		else
			Textures(i).S_Scale = 1.0F;
		
		if (ShiftR(w1x, 0, 16) < 0xFFFF)
			Textures(i).T_Scale = Fixed2Float(ShiftR(w1x, 0, 16), 16);
		else
			Textures(i).T_Scale = 1.0F;
		
		// uot algorithm (above) doesn't work, but this does:
		Textures(i).S_Scale = u16r(data + 4) * (1.0f / UINT16_MAX);
		Textures(i).T_Scale = u16r(data + 6) * (1.0f / UINT16_MAX);
	}
	
	return false;
}

static bool UOT_gbiFunc_loadtlut(void* cmd)
{
	RENDERHOOK_UOT_COMMON
	
	//uint32_t addr = Textures(0).Dram;
	uint32_t count = SHIFTR(w1, 14, 10) + 1;
	const void *realAddr;
	
	//fprintf(stderr, "loadtlut %08x\n", addr);
	
	if ((realAddr = gMatState.timg.imgaddr))
	{
		size_t size = ALIGN8(G_SIZ_BYTES(G_IM_SIZ_16b) * count);
		
		memcpy(gMatState.pal, realAddr, size);
	}
	
	return false;
}

#endif // RENDERHOOK_UOT

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void n64_drawImpl(void* dlist);

static ShaderList* ShaderList_new(uint64_t uuid, void* next) {
	ShaderList* l = calloc(1, sizeof(*l));
	
	l->uuid = uuid;
	l->next = next;
	l->shader = Shader_new();
	
	return l;
}

static Shader* ShaderList_add(uint64_t uuid, bool* isNew) {
	ShaderList* l;
	
	assert(isNew);
	
	if (!sShaderList)
		sShaderList = ShaderList_new(0, 0);
	
	*isNew = false;
	for (l = sShaderList; l; l = l->next) {
		if (l->uuid == uuid)
			return l->shader;
	}
	
	*isNew = true;
	l = ShaderList_new(uuid, sShaderList);
	sShaderList = l;
	
	return l->shader;
}

static void ShaderList_cleanup(void) {
	ShaderList* l;
	ShaderList* next = 0;
	
	if (!sShaderList)
		return;
	
	for (l = sShaderList; l; l = next) {
		if (l->shader)
			Shader_delete(l->shader);
		next = l->next;
		free(l);
	}
	
	sShaderList = 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static char* strcatt(char* dst, const char* src) {
	size_t n = strlen(src);
	
	memcpy(dst, src, n + 1);
	
	return dst + n;
}

static char* strcattf(char* dst, const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	vsprintf(dst, fmt, args);
	va_end(args);
	
	return dst + strlen(dst);
}

static void logmatrix(const char *name, const void *data)
{
	#define PRI_MATRIX_FLOAT  "%4.3f"
	
	fprintf(stderr, "%s:\n", name);
	
	for (int i = 0; i < 16; )
	{
		for (int k = 0; k < 4; ++k, ++i)
			fprintf(stderr, "  " PRI_MATRIX_FLOAT, ((const float*)data)[i]);
		
		fprintf(stderr, "\n");
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const char* mtl_color_Str(int idx, int v) {
	assert(idx >= 0 && idx < 4);
	
	/* based on this table:
	 * https://wiki.cloudmodding.com/oot/F3DZEX2#Color_Combiner_Settings
	 */
	
	if (v >= 8 && idx != 2)
		return "vec3(0.0)";
	
	switch (v) {
		case 0x00: return "FragColor.rgb";
		case 0x01: return "texture(texture0, TexCoord0).rgb";
		case 0x02: return "texture(texture1, TexCoord1).rgb";
		case 0x03: return "uPrimColor.rgb";
		case 0x04: return "shading.rgb";
		case 0x05: return "uEnvColor.rgb";
		case 0x06:
			switch (idx) {
				case 0x00: return "vec3(1.0)";
				case 0x01: return "vec3(1.0)"; // TODO CCMUX_CENTER
				case 0x02: return "vec3(1.0)"; // TODO CCMUX_SCALE
				case 0x03: return "vec3(1.0)";
			}
			
		case 0x07:
			switch (idx) {
				case 0x00: return "vec3(1.0)"; // TODO CCMUX_NOISE
				case 0x01: return "vec3(uK4)";
				case 0x02: return "vec3(FragColor.a)";
				case 0x03: return "vec3(0.0)";
			}
			
		case 0x08: return "vec3(texture(texture0, TexCoord0).a)";
		case 0x09: return "vec3(texture(texture1, TexCoord1).a)";
		case 0x0A: return "vec3(uPrimColor.a)";
		case 0x0B: return "vec3(shading.a)";
		case 0x0C: return "vec3(uEnvColor.a)";
		case 0x0D: return "vec3(uLodFrac)";
		case 0x0E: return "vec3(uPrimLodFrac)";
		case 0x0F: return "vec3(uK5)";
	}
	
	return "vec3(0.0)";
}

static const char* mtl_alpha_str(int idx, int v) {
	assert(idx >= 0 && idx < 4);
	
	/* based on this table:
	 * https://wiki.cloudmodding.com/oot/F3DZEX2#Color_Combiner_Settings
	 */
	
	switch (v) {
		case 0x00:
			switch (idx) {
				case 0x02: return "uPrimLodFrac";
				default: return "FragColor.a";
			}
			
		case 0x01: return "texture(texture0, TexCoord0).a";
		case 0x02: return "texture(texture1, TexCoord1).a";
		case 0x03: return "uPrimColor.a";
		case 0x04: return "shading.a";
		case 0x05: return "uEnvColor.a";
		case 0x06:
			switch (idx) {
				case 0x02: return "uPrimLodFrac";
				default: return "1.0";
			}
	}
	
	return "0.0";
}

static void do_mtl(void* addr) {
	int tile = 0; /* G_TX_RENDERTILE */
	
	/* update texture image associated with each tile */
	for (tile = 0; tile < 2; ++tile) {
		int i;
		bool isNew = false;
		
		if (!gMatState.tile[tile].doUpdate)
			continue;
		
		for (i = 0; i < N64_TEXTURE_CACHE_SIZE; ++i) {
			if (gMatState.tile[tile].data == gTexelDict[i])
				break;
			
			if (gTexelDict[i] == 0) {
				isNew = true;
				gTexelDict[i] = gMatState.tile[tile].data;
				break;
			}
		}
		// no match found
		if (i == N64_TEXTURE_CACHE_SIZE) {
			i = 0;
		}
		glActiveTexture(GL_TEXTURE0 + tile);
		glBindTexture(GL_TEXTURE_2D, gTexel[i]);
		
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gFilterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gFilterMode);
		
		gMatState.tile[tile].doUpdate = false;
		int width = ((gMatState.tile[tile].lrs >> 2) - (gMatState.tile[tile].uls >> 2)) + 1;
		int height = ((gMatState.tile[tile].lrt >> 2) - (gMatState.tile[tile].ult >> 2)) + 1;
		
	#ifdef RENDERHOOK_UOT
		gMatState.tile[tile].lrs = Textures(tile).LRS;
		gMatState.tile[tile].lrt = Textures(tile).LRT;
		gMatState.tile[tile].uls = Textures(tile).ULS;
		gMatState.tile[tile].ult = Textures(tile).ULT;
		gMatState.tile[tile].cmT = Textures(tile).CMT;
		gMatState.tile[tile].cmS = Textures(tile).CMS;
		gMatState.tile[tile].fmt = Textures(tile).TexFormat;
		gMatState.tile[tile].siz = Textures(tile).TexelSize;
		width = Textures(tile).RealWidth;
		height = Textures(tile).RealHeight;
	#endif
		
		int fmt = gMatState.tile[tile].fmt;
		int siz = gMatState.tile[tile].siz;
		
		unsigned wrapT = GL_REPEAT;
		unsigned wrapS = GL_REPEAT;
		
		gMatState.texWidth = width;
		gMatState.texHeight = height;
		
		// TODO mirror and clamp should be able to be combined
		//      in order to accurately emulate everything
		//      (do it at the shader level)
		switch (gMatState.tile[tile].cmT) {
			case G_TX_MIRROR:
				wrapT = GL_MIRRORED_REPEAT;
				break;
			case G_TX_CLAMP | G_TX_MIRROR:
			case G_TX_CLAMP:
				wrapT = GL_CLAMP_TO_EDGE;
				break;
		}
		switch (gMatState.tile[tile].cmS) {
			case G_TX_MIRROR:
				wrapS = GL_MIRRORED_REPEAT;
				break;
			case G_TX_CLAMP | G_TX_MIRROR:
			case G_TX_CLAMP:
				wrapS = GL_CLAMP_TO_EDGE;
				break;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		
		if (!isNew && gHideGeometry)
			continue;
		
		//uls >>= 2; /* discard precision; sourcing pixels directly */
		//ult >>= 2;
		
		/* TODO emulate dxt */
		//dxt = 0;
		
		//src += ult * width;
		//src += uls;
		//fprintf(stderr, "%d %d\n", fmt, siz);
		if (width * height > 4096) width = height = 32; // FIXME getting wrong dimensions
		//memcpy(tmem, src, bytes); /* TODO dxt emulation requires line-by-line */
		if (isNew && gTexelCacheCount < N64_TEXTURE_CACHE_SIZE)	{
			uint8_t wow[4096 * 8];
		#ifdef RENDERHOOK_UOT
			width = Textures(tile).Width;
			height = Textures(tile).Height;
			//fprintf(stderr, "append %p %08x %dx%d\n", gMatState.tile[tile].data, Textures(tile).Dram, width, height);
		#endif
			n64texconv_to_rgba8888(
				wow
				,
				gMatState.tile[tile].data
				,
				(void*)gMatState.pal
				,
				fmt
				,
				siz
				,
				width
				,
				height
			#ifdef RENDERHOOK_UOT
				, Textures(tile).LineSize
			#else
				, 0 // TODO lineSize
			#endif
			);
			//fprintf(stderr, "width height %d %d\n", width, height);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, wow);
			//glGenerateMipmap(GL_TEXTURE_2D);
			gTexelCacheCount += 1;
		}
	}
	
	if (gHideGeometry)
		return;
	
	/* uuid tracks state changes; if no states changed, don't compile new shader */
	{
		uint64_t uuid =
			((uint64_t)gFogEnabled << 63)
			| ((uint64_t)(gCvgXalpha || gForceBl) << 62)
			| ((uint64_t)(gMatState.xhighlight.mode != 0) << 61)
			| ((uint64_t)gMatState.mixFog << 60)
			| ((uint64_t)(gMatState.setcombine.hi & 0x00ffffff) << 32)
			| (gMatState.setcombine.lo)
		;
		bool isNew = false;
		gMatState.prim.r = ((gMatState.prim.lo >> 24) & 0xff) / 255.0f;
		gMatState.prim.g = ((gMatState.prim.lo >> 16) & 0xff) / 255.0f;
		gMatState.prim.b = ((gMatState.prim.lo >> 8) & 0xff) / 255.0f;
		gMatState.prim.alpha = (gMatState.prim.lo & 0xff) / 255.0f;
		
		gMatState.env.r = ((gMatState.env.lo >> 24) & 0xff) / 255.0f;
		gMatState.env.g = ((gMatState.env.lo >> 16) & 0xff) / 255.0f;
		gMatState.env.b = ((gMatState.env.lo >> 8) & 0xff) / 255.0f;
		gMatState.env.alpha = (gMatState.env.lo & 0xff) / 255.0f;
		Shader* shader = ShaderList_add(uuid, &isNew);
		
		// TODO also populate lodfrac, k4, and k5 here
		gMatState.prim.lodfrac = (gMatState.prim.hi & 0xff) / 255.0f;
		
		if (isNew) {
			
			//crustify
			const char *vtx = SHADER_SOURCE(
				layout (location = 0) in vec4 aPos;
				layout (location = 1) in vec4 aColor;
				layout (location = 2) in vec2 aTexCoord0;
				layout (location = 3) in vec2 aTexCoord1;
			
				out vec4 vColor;
				out vec2 TexCoord0;
				out vec2 TexCoord1;
				out float vFog;
				out vec3 vLightColor;
			
				// uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;
				uniform vec2 uFog;
			
				void main() {
				float fogM = uFog.x;
				float fogO = uFog.y;
				vec4 wow = projection * view * vec4(aPos.xyz, 1.0);
			
				gl_Position = projection * view * aPos;
				vColor = aColor;
				TexCoord0 = vec2(aTexCoord0.x, aTexCoord0.y);
				TexCoord1 = vec2(aTexCoord1.x, aTexCoord1.y);
				if (wow.w < 0)
				{
					vFog = -fogM + fogO;
				}
				else
				{
					vFog = wow.z / wow.w * fogM + fogO;
				}
				vFog = clamp(vFog, 0.0, 255.0) / 255;
				vLightColor = vec3(1.0);
			}
			);
			
			char frag[4096] = SHADER_SOURCE(
				out vec4 FragColor;
			
				in vec4 vColor;
				in vec2 TexCoord0;
				in vec2 TexCoord1;
				in float vFog;
				in vec3 vLightColor;
			
				// texture sampler
				uniform sampler2D texture0;
				uniform sampler2D texture1;
				uniform vec3 uFogColor;
				uniform vec4 uPrimColor;
				uniform vec4 uHighlight;
				uniform vec4 uEnvColor;
				uniform float uK4;
				uniform float uK5;
				uniform float uLodFrac;
				uniform float uPrimLodFrac;
			);
			//uncrustify
			
			/* construct fragment shader */
			{
				char* f = frag + strlen(frag);
				uint32_t hi = gMatState.setcombine.hi;
				uint32_t lo = gMatState.setcombine.lo;
				
				#define ADD(X)    f = strcatt(f, X)
				#define ADDF(...) f = strcattf(f, __VA_ARGS__)
				
				ADD("void main(){");
				
				ADD("vec3 final;");
				ADD("vec4 shading;");
				ADD("float alpha = 1.0;");
				ADD("shading = vColor;");
				ADD("shading.rgb *= vLightColor;");
				
				if (gCvgXalpha || gForceBl)	{
					/* alpha cycle 0 */
					ADDF("alpha = %s;", mtl_alpha_str(0, (hi >> 12) & 0x7));
					ADDF("alpha -= %s;", mtl_alpha_str(1, (lo >> 12) & 0x7));
					ADDF("alpha *= %s;", mtl_alpha_str(2, (hi >>  9) & 0x7));
					ADDF("alpha += %s;", mtl_alpha_str(3, (lo >>  9) & 0x7));
					ADD("FragColor.a = alpha;");
					
					/* alpha cycle 1 */
					ADDF("alpha = %s;", mtl_alpha_str(0, (lo >> 21) & 0x7));
					ADDF("alpha -= %s;", mtl_alpha_str(1, (lo >> 3) & 0x7));
					ADDF("alpha *= %s;", mtl_alpha_str(2, (lo >> 18) & 0x7));
					ADDF("alpha += %s;", mtl_alpha_str(3, (lo >>  0) & 0x7));
					ADD("FragColor.a = alpha;");
					
					/* TODO optimization: only include this bit if texture is sampled */
					ADD("if (alpha == 0.0) discard;");
				} else
					ADD("FragColor.a = 1.0;");
				
				/* TODO optimization: detect when unnecessary and omit */
				/* color cycle 0 */
				ADDF("final = %s;", mtl_color_Str(0, (hi >> 20) & 0xf));
				ADDF("final -= %s;", mtl_color_Str(1, (lo >> 28) & 0xf));
				ADDF("final *= %s;", mtl_color_Str(2, (hi >> 15) & 0x1f));
				ADDF("final += %s;", mtl_color_Str(3, (lo >> 15) & 0x7));
				ADD("FragColor.rgb = final;");
				
				/* color cycle 1 */
				ADDF("final = %s;", mtl_color_Str(0, (hi >> 5) & 0xf));
				ADDF("final -= %s;", mtl_color_Str(1, (lo >> 24) & 0xf));
				ADDF("final *= %s;", mtl_color_Str(2, (hi >> 0) & 0x1f));
				ADDF("final += %s;", mtl_color_Str(3, (lo >> 6) & 0x7));
				ADD("FragColor.rgb = final;");
				
				if (gFogEnabled && gMatState.mixFog)
					ADD("FragColor.rgb = mix(FragColor.rgb, uFogColor, vFog);");
				
				switch (gMatState.xhighlight.mode) {
					case GX_HILIGHT_ADD:
						ADD("FragColor.rgb = mix(FragColor.rgb, FragColor.rgb + uHighlight.rgb, uHighlight.a);");
						break;
						
					case GX_HILIGHT_SUB:
						ADD("FragColor.rgb = mix(FragColor.rgb, FragColor.rgb - uHighlight.rgb, uHighlight.a);");
						break;
						
					case GX_HILIGHT_MUL:
						ADD("FragColor.rgb = mix(FragColor.rgb, FragColor.rgb * uHighlight.rgb, uHighlight.a);");
						break;
						
					case GX_HILIGHT_DIV:
						ADD("FragColor.rgb = mix(FragColor.rgb, FragColor.rgb / uHighlight.rgb, uHighlight.a);");
						break;
						
					case GX_HILIGHT_MIX:
						ADD("FragColor.rgb = mix(FragColor.rgb, uHighlight.rgb, uHighlight.a);");
						break;
						
					case GX_HILIGHT_DODGE:
						ADD("FragColor.rgb = mix(FragColor.rgb, uHighlight.rgb / (vec3(1, 1, 1) - FragColor.rgb), uHighlight.a);");
						break;
				}
				
				ADD("}");
				
#undef ADD
#undef ADDF
			}
			
			Shader_update(shader, vtx, frag);
		}
		
		// using new shader, so update view-projection matrices
		gShader = shader;
		if (Shader_use(shader))	{
			Shader_setMat4(shader, "view", &gMatrix.view);
			Shader_setMat4(shader, "projection", &gMatrix.projection);
		}
		
		// populate other misc variables
		Shader_setVec4(shader, "uPrimColor", gMatState.prim.r, gMatState.prim.g, gMatState.prim.b, gMatState.prim.alpha);
		Shader_setVec4(shader, "uHighlight", gMatState.xhighlight.r, gMatState.xhighlight.g, gMatState.xhighlight.b, gMatState.xhighlight.factor);
		Shader_setVec4(shader, "uEnvColor", gMatState.env.r, gMatState.env.g, gMatState.env.b, gMatState.env.alpha);
		Shader_setVec3(shader, "uFogColor", gFog.color[0], gFog.color[1], gFog.color[2]);
		Shader_setVec2(shader, "uFog", gFog.fog[0], gFog.fog[1]);
		Shader_setFloat(shader, "uK4", gMatState.k4);
		Shader_setFloat(shader, "uK5", gMatState.k5);
		Shader_setFloat(shader, "uLodFrac", gMatState.lodfrac);
		Shader_setFloat(shader, "uPrimLodFrac", gMatState.prim.lodfrac);
		Shader_setInt(shader, "texture0", 0);
		Shader_setInt(shader, "texture1", 1);
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static float shift_to_multiplier(const int shift) {
	/* how many bits to shift texture coordinates	   *
	 * if in range  1 <= n <= 10, texcoord >>= n		*
	 * if in range 11 <= n <= 15, texcoord <<= (16 - n) */
	if (!shift)
		return 1;
	
	/* right shift; division by 2 per bit */
	if (shift < 11)
		return 1.0f / pow(2, shift);
	
	/* left shift; multiplication by 2 per bit */
	return pow(2, 16 - shift);
}

static N64Vector4 light_bind(N64Vector3 vtxPos, N64Vector3 vtxNor, int i) {
	GbiLight* light = &sLights.l[i];
	
	if (light->dir.pad1 == 0) {
		GbiLightDir* dir = &light->dir;
		N64Vector3 onrm;
		onrm.x = (float)dir->dir[0] / __INT8_MAX__;
		onrm.y = (float)dir->dir[1] / __INT8_MAX__;
		onrm.z = (float)dir->dir[2] / __INT8_MAX__;
		
		// Directional light
		N64Vector4 col = vec4_color(dir->col);
		N64Vector3 norm = vec3_normalize(onrm);
		float mod = N64_CLAMP(vec3_dot(vtxNor, norm), 0.0, 1.0);
		
		col.x *= mod;
		col.y *= mod;
		col.z *= mod;
		
		return col;
	} else {
		// GbiLightPoint* point = &light->point;
	}
	
	return (N64Vector4) { 0 };
}

static N64Vector4 light_bind_all(N64Vector3 vtxPos, N64Vector3 vtxNor) {
	N64Vector4 final = {
		sLights.a.l.col[0] / 255.0f,
		sLights.a.l.col[1] / 255.0f,
		sLights.a.l.col[2] / 255.0f,
		1.0f
	};
	
	for (int i = 0; i < sLightNum; i++)	{
		N64Vector4 color = light_bind(vtxPos, vtxNor, i);
		final.x += color.x;
		final.y += color.y;
		final.z += color.z;
	}
	
	final.x = N64_CLAMP(final.x, 0.0f, 1.0f);
	final.y = N64_CLAMP(final.y, 0.0f, 1.0f);
	final.z = N64_CLAMP(final.z, 0.0f, 1.0f);
	
	return final;
}

static void try_draw_tri_batch(const uint8_t* b) {
	if (!( (b[8] != G_TRI1 && b[8] != G_TRI2 && b[8] != G_QUAD) || gIndicesUsed + 6 >= N64_ARRAY_COUNT(gIndices) ))
		return;
	
	if (s_tri_callback) {
		for (uint32_t i = 0; i < gIndicesUsed; i += 3) {
			N64Tri triData = {
				{
					&sVbuf[gIndices[i + 0]],
					&sVbuf[gIndices[i + 1]],
					&sVbuf[gIndices[i + 2]],
				},
				{
					(gMatState.geometrymode & G_CULL_BACK),
					(gMatState.geometrymode & G_CULL_FRONT),
				}
			};
			
			s_tri_callback(s_tri_callback_data, &triData);
		}
	}
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*gIndices) * gIndicesUsed, gIndices, GL_DYNAMIC_DRAW);
	
	#if 0 // wireframe method
		if (gGxOutline) {
			Shader_use(sOutlineShader);
			Shader_setMat4(sOutlineShader, "view", &gMatrix.view);
			Shader_setMat4(sOutlineShader, "projection", &gMatrix.projection);
			
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(10, 10);
			
			Shader_setVec4(sOutlineShader, "color", 1, 0.5, 0, 1);
			glLineWidth(5);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, gIndicesUsed, GL_UNSIGNED_BYTE, 0);
			
			glPolygonOffset(gPolygonOffset, gPolygonOffset);
			glDisable(GL_POLYGON_OFFSET_LINE);
			
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			Shader_use(gShader);
		}
	#else // inverse hull method
		if (gGxOutline) {
			GLint OldCullMode;
			GLboolean OldCullBool;
			GLboolean OldDepthBool;
			GLboolean OldBlendBool;
			
			Shader_use(sOutlineShader);
			Shader_setMat4(sOutlineShader, "view", &gMatrix.view);
			Shader_setMat4(sOutlineShader, "projection", &gMatrix.projection);
			
			glGetIntegerv(GL_CULL_FACE_MODE, &OldCullMode);
			glGetBooleanv(GL_CULL_FACE, &OldCullBool);
			glGetBooleanv(GL_DEPTH_TEST, &OldDepthBool);
			glGetBooleanv(GL_BLEND, &OldBlendBool);
			
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			if (OldCullBool)
				glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glDisable(GL_DEPTH_TEST); // comment this line to disable x-ray mode
			
			//Shader_setVec4(sOutlineShader, "color", 1, 0.5, 0, 1); // opaque orange
			Shader_setVec4(sOutlineShader, "color", 1, 0.5, 0, 0.5); // translucent orange
			
			glDrawElements(GL_TRIANGLES, gIndicesUsed, GL_UNSIGNED_BYTE, 0);
			
			Shader_use(gShader);
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glCullFace(OldCullMode);
			if (!OldBlendBool)
				glDisable(GL_BLEND);
			if (!OldCullBool)
				glDisable(GL_CULL_FACE);
			if (OldDepthBool)
				glEnable(GL_DEPTH_TEST);
		}
	#endif
	
	glDrawElements(GL_TRIANGLES, gIndicesUsed, GL_UNSIGNED_BYTE, 0);
	gIndicesUsed = 0;
}

static void othermode(void) {
	///////// PARAMETER ////////////////// PPPP AAAA MMMM BBBB
	#define G_RM_CYCLE1_MASK 0xCCCC0000 // 1100 1100 1100 1100
	#define G_RM_CYCLE2_MASK 0x33330000 // 0011 0011 0011 0011
	///////// CYCLE NO. ////////////////// 1122 1122 1122 1122
	uint32_t hi = gMatState.othermode_hi;
	uint32_t lo = gMatState.othermode_lo;
	uint32_t indep = (lo & 0b1111111111111000) >> 3;
	
	gMatState.mixFog = true;
	
	/*
	 * One of the cases where FOG should be ignored. But this is mostly
	 * hardcoded solution and wont apply to all cases where fog would
	 * be disabled on material.
	 *
	 * Required further investigation.
	 */
	//if ((lo & G_RM_CYCLE1_MASK) == GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1))
	//if (!(((lo & G_RM_CYCLE1_MASK) >> (16 + 14)) & G_BL_CLR_FOG))
	if (lo >> 30 == 0)
		gMatState.mixFog = false;
	
	gCurrentZmode = (indep & 0b0000110000000) >> 7;
	gForceBl = (indep & 0b0100000000000) >> 11;
	gCvgXalpha = (indep & 0b0001000000000) >> 9;
	
	static uint8_t zmodes[] = { N64_ZMODE_OPA, N64_ZMODE_INTER, N64_ZMODE_XLU, N64_ZMODE_DEC };
	gCurrentZmode = zmodes[gCurrentZmode];
	
	switch (hi & (0b11 << G_MDSFT_TEXTFILT)) {
		case G_TF_POINT:
			gFilterMode = GL_NEAREST;
			break;
			
		case G_TF_BILERP:
		case G_TF_AVERAGE:
			gFilterMode = GL_LINEAR;
			break;
	}
	
	if (gForceBl == true) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {                   /* false */
		glDisable(GL_BLEND);
	}
	
	gHideGeometry = false;
	if (gOnlyThisZmode != N64_ZMODE_ALL && !(gOnlyThisZmode & gCurrentZmode))
		gHideGeometry = true;
	
	if (!gHideGeometry && gOnlyThisGeoLayer != N64_GEOLAYER_ALL) {
		int isOverlay = gForceBl || gCvgXalpha;
		switch (gOnlyThisGeoLayer) {
			case N64_GEOLAYER_OPAQUE:
				if (isOverlay)
					gHideGeometry = true;
				break;
				
			case N64_GEOLAYER_OVERLAY:
				if (!isOverlay)
					gHideGeometry = true;
				break;
				
			case N64_GEOLAYER_ALL:
				break;
		}
	}
	
	/* hack for eliminating z-fighting on decals */
	switch (gCurrentZmode) {
		case N64_ZMODE_DEC: /* ZMODE_DEC */
			glEnable(GL_POLYGON_OFFSET_FILL);
			gPolygonOffset = -1;
			break;
		default:
			glDisable(GL_POLYGON_OFFSET_FILL);
			gPolygonOffset = 0;
			break;
	}
	
	glPolygonOffset(gPolygonOffset, gPolygonOffset);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static bool gbiFunc_vtx(void* cmd) {
	uint8_t* b = cmd;
	
	int numv = (b[1] << 4) | (b[2] >> 4);
	int vbidx = (b[3] >> 1) - numv;
	GbiVtx* vtx = n64_segment_get(u32r(b + 4));
	N64Vtx* dst = sVbuf + vbidx;
	
	if (!gMatState.mtlReady) {
		do_mtl(cmd);
		gMatState.mtlReady = 1;
	}
	
	if (gHideGeometry)
		return false;
	
	for (; numv--; dst++, vtx++) {
		dst->pos.x = vtx->x;
		dst->pos.y = vtx->y;
		dst->pos.z = vtx->z;
		
		N64Vector3 modelPos = { dst->pos.x, dst->pos.y, dst->pos.z };
		
		mtx_mul_vec4(&modelPos, &dst->pos, gMatrix.modelNow);
		
	#ifdef RENDERHOOK_UOT
		dst->texcoord0.u = vtx->u * Textures(0).TextureWRatio;
		dst->texcoord0.v = vtx->v * Textures(0).TextureHRatio;
		dst->texcoord1.u = vtx->u * Textures(1).TextureWRatio;
		dst->texcoord1.v = vtx->v * Textures(1).TextureHRatio;
		
		// https://github.com/z64me/zzviewer-rrw
		// TODO texgen uses hard-coded texture sizes
		if (gMatState.texgen)
		{
			N64Vector3 norm;
			// GL_EYE_LINEAR
		#if 0
			// try doing some texgen stuff
			/* zzviewer-rrw code, for reference
			vec4 V = vec4_transform(ctx->modelview, vert->pos);
			struct vec4 e = vec4_normalize3(&V);
			struct vec4 n = vec4_transform(ctx->projection, vert->col);
			vec4_normalize3_inplace(&n);
			
			struct vec4 r = reflect(&e, &n);
			float m = 2 * sqrt(r.x*r.x + r.y*r.y + (r.z+1)*(r.z+1));
			vert->uv.x = r.x / m + 0.5f;
			vert->uv.y = r.y / m + 0.5f;
			vert->uv.x *= 16;
			vert->uv.y *= 16;
			*/
		#endif
			/* zzviewer-rrw
			struct vec4 n = vec4_transform(ctx->normalmatrix, vert->norm);
			vec4_normalize3_inplace(&n);
			vert->uv.x = (n.x + 1) / 2;
			vert->uv.y = (n.y + 1) / 2;
			*/
			{
				norm = (N64Vector3){ UNFOLD_VEC3_EXT(vtx->normal, * (1.0 / 127.0)) };
				norm = mtx_mul_vec3(norm, &gMatrix.normal);
				norm = vec3_normalize(norm);
			}
		#if 0 // linear?
			//vec4_normalize2_inplace(&n);
			norm.x = acos(norm.x);
			norm.y = acos(norm.y);
		#endif
			/* zzviewer-rrw
			// TODO 8 or 16? likely varies with texture dimensions...
			norm.x *= 16;
			norm.y *= 16;
			*/
			// zzviewer-rrw multiplied a value, not necessary here?
			
			// approximate spherical
			//norm.x /= 8;
			//norm.y /= 8;
			
			// apply to uv's
			dst->texcoord0.u = norm.x;
			dst->texcoord0.v = norm.y;
			dst->texcoord1.u = norm.x;
			dst->texcoord1.v = norm.y;
		}
		
		// scrolling textures
		// TODO make not hard-coded
		dst->texcoord0.u -= (Textures(0).ULS / 128.0f) / (Textures(0).RealWidth  / 32.0f);
		dst->texcoord0.v -= (Textures(0).ULT / 128.0f) / (Textures(0).RealHeight / 32.0f);
		dst->texcoord1.u -= (Textures(1).ULS / 128.0f) / (Textures(1).RealWidth  / 32.0f);
		dst->texcoord1.v -= (Textures(1).ULT / 128.0f) / (Textures(1).RealHeight / 32.0f);
	#else
		dst->texcoord0.u = vtx->u * (1.0 / 1024) * (32.0 / gMatState.texWidth);
		dst->texcoord0.v = vtx->v * (1.0 / 1024) * (32.0 / gMatState.texHeight);
		dst->texcoord1.u = dst->texcoord0.u;
		dst->texcoord1.v = dst->texcoord0.v;
		
		dst->texcoord0.u *= gMatState.tile[0].shiftS_m;
		dst->texcoord0.v *= gMatState.tile[0].shiftT_m;
		dst->texcoord0.u -= gMatState.tile[0].uls / 128.0f; /* TODO hard-coded why? */
		dst->texcoord0.v -= gMatState.tile[0].ult / 128.0f; /* TODO hard-coded why? */
		
		dst->texcoord1.u *= gMatState.tile[1].shiftS_m;
		dst->texcoord1.v *= gMatState.tile[1].shiftT_m;
		dst->texcoord1.u -= gMatState.tile[1].uls / 128.0f; /* TODO hard-coded why? */
		dst->texcoord1.v -= gMatState.tile[1].ult / 128.0f; /* TODO hard-coded why? */
	#endif
		
		if (gVertexColors) {
			dst->color.x = vtx->color.r * (1.0 / 255.0);
			dst->color.y = vtx->color.g * (1.0 / 255.0);
			dst->color.z = vtx->color.b * (1.0 / 255.0);
			
			// normals still required for inverse hull
			if (gGxOutline) {
				N64Vector3 global_normal;
				dst->norm.x = vtx->normal.x * (1.0 / 127.0);
				dst->norm.y = vtx->normal.y * (1.0 / 127.0);
				dst->norm.z = vtx->normal.z * (1.0 / 127.0);
				
				mtx_mul_vec3_rot(&dst->norm, &global_normal, gMatrix.modelNow);
				dst->norm = vec3_normalize(global_normal);
			}
		} else {
			N64Vector3 global_normal;
			dst->norm.x = vtx->normal.x * (1.0 / 127.0);
			dst->norm.y = vtx->normal.y * (1.0 / 127.0);
			dst->norm.z = vtx->normal.z * (1.0 / 127.0);
			
			mtx_mul_vec3_rot(&dst->norm, &global_normal, gMatrix.modelNow);
			dst->color = light_bind_all(modelPos, vec3_normalize(global_normal));
		}
		
		dst->color.w = vtx->color.a * (1.0 / 255.0);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sVbuf), sVbuf, GL_DYNAMIC_DRAW);
	gIndicesUsed = 0;
	
	return false;
}

static bool gbiFunc_culldl(void* cmd) {
	if (s_cull_enabled == false)
		return false;
	
	if (s_cull_callback == NULL)
		return false;
	
	uint8_t* b = cmd;
	int vfirst = u16r(b + 2) / 2;
	int vlast = u16r(b + 6) / 2;
	N64Vtx* v = sVbuf + vfirst;
	
	return s_cull_callback(s_cull_callback_data, (void*)v, vlast - vfirst + 1);
}

static bool gbiFunc_tri1(void* cmd) {
	uint8_t* b = cmd;
	
	if (gHideGeometry)
		return false;
	
	gIndices[gIndicesUsed++] = b[1] / 2;
	gIndices[gIndicesUsed++] = b[2] / 2;
	gIndices[gIndicesUsed++] = b[3] / 2;
	
	try_draw_tri_batch(b);
	
	return false;
}

static bool gbiFunc_tri2(void* cmd) {
	uint8_t* b = cmd;
	
	if (gHideGeometry)
		return false;
	
	gIndices[gIndicesUsed++] = b[1] / 2;
	gIndices[gIndicesUsed++] = b[2] / 2;
	gIndices[gIndicesUsed++] = b[3] / 2;
	
	gIndices[gIndicesUsed++] = b[5] / 2;
	gIndices[gIndicesUsed++] = b[6] / 2;
	gIndices[gIndicesUsed++] = b[7] / 2;
	
	try_draw_tri_batch(b);
	
	return false;
}

static bool gbiFunc_settimg(void* cmd) {
	uint8_t* b = cmd;
	uint8_t bits = b[1];
	uint16_t hi = u16r(b + 2);
	uint32_t lo = u32r(b + 4);
	void* imgaddr = n64_segment_get(lo);
	int fmt = bits >> 5;
	int siz = (bits >> 3) & 3;
	int width = hi + 1;
	
	gMatState.timg.fmt = fmt;
	gMatState.timg.siz = siz;
	gMatState.timg.width = width;
	gMatState.timg.imgaddr = imgaddr;
	
	//fprintf(stderr, "settimg %08x\n", lo);
	
	return false;
}

static bool gbiFunc_texture(void* cmd) {
	uint8_t* b = cmd;
	uint16_t bits = u16r(b + 2);
	int tile = (bits >> 8) & 7;
	int level = (bits >> 11) & 7;
	int on = bits & 0xfe;
	
	if (tile > 1)
		return false;
	
	gMatState.tile[tile].on = on;
	
	if (!on)
		return false;
	
	gMatState.tile[tile].level = level;
	gMatState.tile[tile].scaleS = u16r(b + 4) * (1.0f / UINT16_MAX);
	gMatState.tile[tile].scaleT = u16r(b + 6) * (1.0f / UINT16_MAX);
	
	return false;
}

static bool gbiFunc_loadtlut(void* cmd) {
	uint8_t* b = cmd;
	// int t = b[4];
	int c = (b[5] << 4) | (b[6] >> 4);
	
	if (!gMatState.timg.imgaddr)
		return false;
	
	//fprintf(stderr, "loadtlut\n");
	
	memcpy(gMatState.pal, gMatState.timg.imgaddr, ((c >> 2) + 1) * sizeof(uint16_t));
	
	return false;
}

static bool gbiFunc_settilesize(void* cmd) {
	uint8_t* b = cmd;
	int i = b[4];
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	if (i > 1)
		return false;
	
	gMatState.tile[i].uls = (hi >> 12) & 0xfff;
	gMatState.tile[i].ult = hi & 0xfff;
	gMatState.tile[i].lrs = (lo >> 12) & 0xfff;
	gMatState.tile[i].lrt = lo & 0xfff;
	
	return false;
}

static bool gbiFunc_settile(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	int fmt = (hi >> 21) & 7;
	int siz = (hi >> 19) & 3;
	int line = (hi >> 5) & 0x1ff;
	int tmem = hi & 0x1ff;
	int tile = (lo >> 24);
	int palette = (lo >> 20) & 0xf;
	int cmT = (lo >> 18) & 3;
	int maskT = (lo >> 14) & 0xf;
	int shiftT = (lo >> 10) & 0xf;
	int cmS = (lo >> 8) & 3;
	int maskS = (lo >> 4) & 0xf;
	int shiftS = lo & 0xf;
	
	if (tile > 1)
		return false;
	
	gMatState.tile[tile].fmt = fmt;
	gMatState.tile[tile].siz = siz;
	gMatState.tile[tile].line = line;
	gMatState.tile[tile].tmem = tmem;
	gMatState.tile[tile].tile = tile;
	gMatState.tile[tile].palette = palette;
	gMatState.tile[tile].cmT = cmT;
	gMatState.tile[tile].maskT = maskT;
	gMatState.tile[tile].shiftT = shiftT;
	gMatState.tile[tile].cmS = cmS;
	gMatState.tile[tile].maskS = maskS;
	gMatState.tile[tile].shiftS = shiftS;
	gMatState.tile[tile].data = gMatState.timg.imgaddr;
	gMatState.tile[tile].doUpdate = true;
	
	gMatState.tile[tile].shiftS_m = shift_to_multiplier(shiftS);
	gMatState.tile[tile].shiftT_m = shift_to_multiplier(shiftT);
	
	// this adjusts the texture coordinates for big multi-textures
	if (gMatState.tile[tile].cmS & G_TX_MIRROR)
		gMatState.tile[tile].shiftS_m *= 2;
	if (gMatState.tile[tile].cmT & G_TX_MIRROR)
		gMatState.tile[tile].shiftT_m *= 2;
	
	return false;
}

static bool gbiFunc_loadblock(void* cmd) {
	return false;
}

static bool gbiFunc_loadtile(void* cmd) {
	return false;
}

static bool gbiFunc_rdppipesync(void* cmd) {
	gMatState.mtlReady = 0;
	
	return false;
}

static bool gbiFunc_enddl(void* cmd) {
	return true;
}

static bool gbiFunc_setothermode_l(void* cmd) {
	uint8_t* b = cmd;
	int ss = b[2];
	int nn = b[3];
	uint32_t data = u32r(b + 4);
	int shift = 32 - (nn + 1) - ss;
	int length = nn + 1;
	
	gMatState.othermode_lo = (gMatState.othermode_lo & ~(((1 << length) - 1) << shift)) | data;
	
	othermode();
	
	return false;
}

static bool gbiFunc_setothermode_h(void* cmd) {
	uint8_t* b = cmd;
	int ss = b[2];
	int nn = b[3];
	uint32_t data = u32r(b + 4);
	int shift = 32 - (nn + 1) - ss;
	int length = nn + 1;
	
	gMatState.othermode_hi = (gMatState.othermode_hi & ~(((1 << length) - 1) << shift)) | data;
	
	othermode();
	
	return false;
}

static bool gbiFunc_rdpsetothermode(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.othermode_hi = u32r(b);
	gMatState.othermode_lo = u32r(b + 4);
	
	othermode();
	
	return false;
}

static bool gbiFunc_setprimcolor(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.prim.hi = u32r(b);
	gMatState.prim.lo = u32r(b + 4);
	
	// update primcolor register in already-active shader
	if (gMatState.mtlReady && gShader)
	{
		gMatState.prim.r = ((gMatState.prim.lo >> 24) & 0xff) / 255.0f;
		gMatState.prim.g = ((gMatState.prim.lo >> 16) & 0xff) / 255.0f;
		gMatState.prim.b = ((gMatState.prim.lo >> 8) & 0xff) / 255.0f;
		gMatState.prim.alpha = (gMatState.prim.lo & 0xff) / 255.0f;
		Shader_setVec4(gShader, "uPrimColor", gMatState.prim.r, gMatState.prim.g, gMatState.prim.b, gMatState.prim.alpha);
		
		gMatState.prim.lodfrac = (gMatState.prim.hi & 0xff) / 255.0f;
		Shader_setFloat(gShader, "uPrimLodFrac", gMatState.prim.lodfrac);
	}
	
	return false;
}

static bool gbiFunc_setenvcolor(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.env.hi = u32r(b);
	gMatState.env.lo = u32r(b + 4);
	
	// update envcolor register in already-active shader
	if (gMatState.mtlReady && gShader)
	{
		gMatState.env.r = ((gMatState.env.lo >> 24) & 0xff) / 255.0f;
		gMatState.env.g = ((gMatState.env.lo >> 16) & 0xff) / 255.0f;
		gMatState.env.b = ((gMatState.env.lo >> 8) & 0xff) / 255.0f;
		gMatState.env.alpha = (gMatState.env.lo & 0xff) / 255.0f;
		
		Shader_setVec4(gShader, "uEnvColor", gMatState.env.r, gMatState.env.g, gMatState.env.b, gMatState.env.alpha);
	}
	
	return false;
}

static bool gbiFunc_setcombine(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.setcombine.hi = u32r(b);
	gMatState.setcombine.lo = u32r(b + 4);
	
	return false;
}

static bool gbiFunc_geometrymode(void* cmd) {
	uint8_t* b = cmd;
	uint32_t clearbits = ~(u32r(b) & 0xffffff);
	uint32_t setbits = u32r(b + 4);
	
	gMatState.geometrymode = (gMatState.geometrymode & ~clearbits) | setbits;
	
	/* vertex colors */
	if (clearbits & G_LIGHTING)
		gVertexColors = 1;
	if (setbits & G_LIGHTING)
		gVertexColors = 0;
	if (clearbits & G_ZBUFFER)
		glDisable(GL_DEPTH_TEST);
	if (setbits & G_ZBUFFER)
		glEnable(GL_DEPTH_TEST);
	
	// texgen
	gMatState.texgen = (gMatState.geometrymode
		& (N64_RSP_TEXTURE_GEN | N64_RSP_TEXTURE_GEN_LINEAR)
	) >> 18;
	//if (gMatState.texgen) fprintf(stderr, "texgen = %d\n", gMatState.texgen);
	
	/* backface/frontface culling */
	glEnable(GL_CULL_FACE);
	switch (gMatState.geometrymode & (G_CULL_FRONT | G_CULL_BACK)) {
		case G_CULL_FRONT | G_CULL_BACK:
			glCullFace(GL_FRONT_AND_BACK);
			break;
		case G_CULL_FRONT:
			glCullFace(GL_FRONT);
			break;
		case G_CULL_BACK:
			glCullFace(GL_BACK);
			break;
		default:
			glDisable(GL_CULL_FACE);
			break;
	}
	
	return false;
}

static bool gbiFunc_mtx(void* cmd) {
	uint8_t* b = cmd;
	uint8_t params = b[3] ^ G_MTX_PUSH;
	uint32_t mtxaddr = u32r(b + 4);
	GbiMtx* mtx;
	Mtx mtxF;
	
	if (mtxaddr == 0x8012DB20) /* XXX hard-coded gMtxClear */
		memcpy(&mtxF, &sClearMtx, sizeof(sClearMtx));
	else {
		//bool wasDirectAddress = gPtrHiSet;
		mtx = n64_segment_get(mtxaddr);
		
		if (!mtx)
			return false;
		
		GbiMtx swap = *mtx;
		
		// XXX assuming all matrices are in N64 format and in need of byteswap
		//if (wasDirectAddress || (mtxaddr & 0xFF000000) != 0x0D000000) {
		if (true) {
			for (int32_t i = 0; i < 0x40 / 2; i++) {
				// byteswap
				uint16_t* ss = (uint16_t*)&swap;
				ss[i] = u16r(&ss[i]);
			}
		}
		
		gbimtx_to_mtx(&swap, &mtxF);
	}
	
	/* push matrix on stack */
	if (params & G_MTX_PUSH) {
		gMatrix.modelNow += 1;
		
		assert(gMatrix.modelNow - gMatrix.modelStack < N64_MTX_STACK_SIZE && "matrix stack overflow");
		
		*gMatrix.modelNow = *(gMatrix.modelNow - 1);
	}
	
	if (params & G_MTX_LOAD) {
		*gMatrix.modelNow = mtxF;
	} else {
		Mtx copy = *gMatrix.modelNow;
		mtx_mtx_mul(&copy, &mtxF, gMatrix.modelNow);
	}
	
	return false;
}

static bool gbiFunc_popmtx(void* cmd) {
	uint8_t* b = cmd;
	int num = u32r(b + 4) / 0x40;
	
	gMatrix.modelNow -= num;
	assert(gMatrix.modelNow >= gMatrix.modelStack && "matrix stack underflow");
	
	return false;
}

static bool gbiFunc_dl(void* cmd) {
	uint8_t* b = cmd;
	// uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	n64_drawImpl(n64_segment_get(lo));
	
	return b[1] != 0;
}

static bool gbiFunc_setptrhi(void* cmd) {
#if __SIZEOF_POINTER__ == 8
		uint8_t* b = cmd;
		gPtrHi = u32r(b + 4);
		gPtrHi <<= 32;
#else
		gPtrHi = 0;
#endif
	gPtrHiSet = true;
	
	return false;
}

static bool gbiFunc_moveword(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	uint8_t index = b[1];
	uint16_t offset = hi & 0xffff;
	void* data = n64_segment_get(lo);
	
	switch (index) {
		case G_MW_MATRIX: break; // TODO
		case G_MW_NUMLIGHT: break; // TODO
		case G_MW_CLIP: break; // TODO
		case G_MW_SEGMENT:
			n64_segment[offset / 4] = data;
			break;
		case G_MW_FOG: break; // TODO
		case G_MW_LIGHTCOL: break; // TODO
		case G_MW_FORCEMTX: break; // TODO
		case G_MW_PERSPNORM: break; // TODO
		default: assert(0 && "moveword unknown index"); break;
	}
	
	return false;
}

static bool gbiFunc_branch_z(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	int vbidx0 = ((hi >> 12) & 0xfff) / 5;
	int vbidx1 = (hi & 0xfff) / 2;
	
	assert(vbidx0 == vbidx1);
	assert(vbidx0 < N64_VBUF_MAX);
	
	N64Vector3 vtx = { UNFOLD_VEC3(sVbuf[vbidx0].pos) };
	
	vtx = mtx_mul_vec3(vtx, gMatrix.modelNow);
	vtx = mtx_mul_vec3(vtx, &gMatrix.view);
	vtx = mtx_mul_vec3(vtx, &gMatrix.projection);
	
	float z = vtx.z;
	
	//fprintf(stderr, "vtx.z = %f vs %d\n", vtx.z, lo);
	
	// simulate branching
	if (z <= lo)
	{
		n64_drawImpl(n64_segment_get(gRdpHalf1));
		return true;
	}
	
	return false;
}

static bool gbiFunc_rdphalf_1(void* cmd) {
	uint8_t* b = cmd;
	
	gRdpHalf1 = u32r(b + 4);
	
	return false;
}

static bool gbiFunc_rdphalf_2(void* cmd) {
	uint8_t* b = cmd;
	
	gRdpHalf2 = u32r(b + 4);
	
	return false;
}

static bool gbiFunc_xmode(void* cmd) {
	uint8_t* b = cmd;
	
	uint32_t clear = u32r(b);
	uint32_t set = u32r(b + 4);
	
	if (clear & GX_MODE_OUTLINE)
		gGxOutline = false;
	
	if (set & GX_MODE_OUTLINE)
		gGxOutline = true;
	
	if (clear & GX_MODE_POLYGONOFFSET) {
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0, 0);
	}
	
	if (clear & GX_MODE_WIREFRAME) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
	
	if (set & GX_MODE_POLYGONOFFSET) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-2, -2);
	}
	
	if (set & GX_MODE_WIREFRAME) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}
	
	return false;
}

static bool gbiFunc_xhlight(void* cmd) {
	struct {
		union {
			struct {
				uint8_t cmd;
				uint8_t r;
				uint8_t g;
				uint8_t b;
			};
			uint32_t upper;
		};
		union {
			struct {
				uint8_t reserved[2];
				uint8_t factor;
				uint8_t mode;
			};
			uint32_t lower;
		};
	} c = {
		.upper = *((uint32_t*)cmd),
		.lower = *((uint32_t*)cmd + 1)
		,
	};
	
	gMatState.xhighlight.mode = c.mode;
	
	if (c.mode)	{
		gMatState.xhighlight.r = c.r / 255.0f;
		gMatState.xhighlight.g = c.g / 255.0f;
		gMatState.xhighlight.b = c.b / 255.0f;
		gMatState.xhighlight.factor = c.factor / 255.0f;
	}
	
	return false;
}

// https://github.com/z64me/zzviewer-rrw
static void recompute_normal_matrix(void)
{
	float det, m[16], f[18], *normal;
	int i, j;
	
	const float *mv = (void*)&gMatrix.view;
	normal = (void*)&gMatrix.normal;
	
	/* m = inverse( mv ) */
	f[ 0] = mv[10] * mv[15] - mv[11] * mv[14];
	f[ 1] = mv[ 7] * mv[14] - mv[ 6] * mv[15];
	f[ 2] = mv[ 6] * mv[11] - mv[ 7] * mv[10];
	f[ 3] = mv[ 2] * mv[15] - mv[ 3] * mv[14];
	f[ 4] = mv[ 3] * mv[10] - mv[ 2] * mv[11];
	f[ 5] = mv[ 2] * mv[ 7] - mv[ 3] * mv[ 6];
	f[ 6] = mv[ 9] * mv[15] - mv[11] * mv[13];
	f[ 7] = mv[ 7] * mv[13] - mv[ 5] * mv[15];
	f[ 8] = mv[ 5] * mv[11] - mv[ 7] * mv[ 9];
	f[ 9] = mv[ 1] * mv[15] - mv[ 3] * mv[13];
	f[10] = mv[ 3] * mv[ 9] - mv[ 1] * mv[11];
	f[11] = mv[ 1] * mv[ 7] - mv[ 3] * mv[ 5];
	f[12] = mv[10] * mv[13] - mv[ 9] * mv[14];
	f[13] = mv[ 5] * mv[14] - mv[ 6] * mv[13];
	f[14] = mv[ 6] * mv[ 9] - mv[ 5] * mv[10];
	f[15] = mv[ 2] * mv[13] - mv[ 1] * mv[14];
	f[16] = mv[ 1] * mv[10] - mv[ 2] * mv[ 9];
	f[17] = mv[ 2] * mv[ 5] - mv[ 1] * mv[ 6];
	
	m[ 0] =  mv[5] * f[ 0] + mv[9] * f[ 1] + mv[13] * f[ 2];
	m[ 1] = -mv[1] * f[ 0] + mv[9] * f[ 3] + mv[13] * f[ 4];
	m[ 2] = -mv[1] * f[ 1] - mv[5] * f[ 3] + mv[13] * f[ 5];
	m[ 3] = -mv[1] * f[ 2] - mv[5] * f[ 4] - mv[ 9] * f[ 5];
	
	m[ 4] = -mv[4] * f[ 0] - mv[8] * f[ 1] - mv[12] * f[ 2];
	m[ 5] =  mv[0] * f[ 0] - mv[8] * f[ 3] - mv[12] * f[ 4];
	m[ 6] =  mv[0] * f[ 1] + mv[4] * f[ 3] - mv[12] * f[ 5];
	m[ 7] =  mv[0] * f[ 2] + mv[4] * f[ 4] + mv[ 8] * f[ 5];
	
	m[ 8] =  mv[4] * f[ 6] + mv[8] * f[ 7] + mv[12] * f[ 8];
	m[ 9] = -mv[0] * f[ 6] + mv[8] * f[ 9] + mv[12] * f[10];
	m[10] = -mv[0] * f[ 7] - mv[4] * f[ 9] + mv[12] * f[11];
	m[11] = -mv[0] * f[ 8] - mv[4] * f[10] - mv[ 8] * f[11];
	
	m[12] =  mv[4] * f[12] + mv[8] * f[13] + mv[12] * f[14];
	m[13] =  mv[0] * f[12] + mv[8] * f[15] + mv[12] * f[16];
	m[14] = -mv[0] * f[13] - mv[4] * f[15] + mv[12] * f[17];
	m[15] = -mv[0] * f[14] - mv[4] * f[16] - mv[ 8] * f[17];
	
	det = mv[0] * m[0] + mv[1] * m[4] + mv[2] * m[8] + mv[3] * m[12];
	
	/* normal = transpose(inverse(mv)) = transpose(m/det) */
	if ((det < -FLT_MIN) || (det > FLT_MIN)) {
		det = 1.0f / det;
		
		for (i = 0; i < 4; ++i) {
			for (j = 0; j < 4; ++j) {
				normal[i*4 + j] = m[j*4 + i] * det;
			}
		}
	} else {
		memset(normal, 0, sizeof(float) * 16);
		normal[0] = normal[5] = normal[10] = normal[15] = 1.0f;
	}
}

static GbiFunc gGbi[0xFF] = {
	[G_VTX] =             gbiFunc_vtx,
	[G_CULLDL] =          gbiFunc_culldl,
	[G_TRI1] =            gbiFunc_tri1,
	[G_TRI2] =            gbiFunc_tri2,
	[G_QUAD] =            gbiFunc_tri2, // quad and tri2 take the same args
	[G_SETTIMG] =         gbiFunc_settimg,
	[G_TEXTURE] =         gbiFunc_texture,
	[G_LOADTLUT] =        gbiFunc_loadtlut,
	[G_SETTILE] =         gbiFunc_settile,
	[G_SETTILESIZE] =     gbiFunc_settilesize,
	[G_LOADBLOCK] =       gbiFunc_loadblock,
	[G_LOADTILE] =        gbiFunc_loadtile,
	[G_RDPPIPESYNC] =     gbiFunc_rdppipesync,
	[G_RDPSETOTHERMODE] = gbiFunc_rdpsetothermode,
	[G_SETOTHERMODE_L] =  gbiFunc_setothermode_l,
	[G_SETOTHERMODE_H] =  gbiFunc_setothermode_h,
	[G_SETPRIMCOLOR] =    gbiFunc_setprimcolor,
	[G_SETENVCOLOR] =     gbiFunc_setenvcolor,
	[G_SETCOMBINE] =      gbiFunc_setcombine,
	[G_GEOMETRYMODE] =    gbiFunc_geometrymode,
	[G_MTX] =             gbiFunc_mtx,
	[G_POPMTX] =          gbiFunc_popmtx,
	[G_DL] =              gbiFunc_dl,
	[G_MOVEWORD] =        gbiFunc_moveword,
	[G_SETPTRHI] =        gbiFunc_setptrhi,
	[G_BRANCH_Z] =        gbiFunc_branch_z,
	[G_RDPHALF_1] =       gbiFunc_rdphalf_1,
	[G_RDPHALF_2] =       gbiFunc_rdphalf_2,
	[G_ENDDL] =           gbiFunc_enddl,
	
	[GX_MODE] =           gbiFunc_xmode,
	[GX_HILIGHT] =        gbiFunc_xhlight,
	
#ifdef RENDERHOOK_UOT
	[G_SETTIMG] = UOT_gbiFunc_settimg,
	[G_SETTILE] = UOT_gbiFunc_settile,
	[G_SETTILESIZE] = UOT_gbiFunc_settilesize,
	[G_TEXTURE] = UOT_gbiFunc_texture,
	[G_LOADTLUT] = UOT_gbiFunc_loadtlut,
#endif

};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void n64_segment_set(int seg, void* data) {
	assert(seg < N64_SEGMENT_MAX);
	
	n64_segment[seg] = data;
}

void* n64_segment_get(unsigned int segaddr) {
	uint8_t* b;
	
	if (gPtrHiSet) {
		gPtrHiSet = false;
		gPtrHi |= segaddr;
		
		return (void*)gPtrHi;
	}
	
	if (!segaddr)
		return 0;
	
	assert((segaddr >> 24) < N64_SEGMENT_MAX);
	
	b = n64_segment[segaddr >> 24];
	
	if (!b)
		return 0;
	
	return b + (segaddr & 0xffffff);
}

unsigned int n64_segment_ptr_offset(void* cmd) {
	uint8_t* b = cmd;
	ptrdiff_t dist = -1;
	int smallest = -1;
	int i;
	
	if (!b)
		return 0;
	
	for (i = 0; i < N64_SEGMENT_MAX; ++i) {
		uint8_t* this = n64_segment[i];
		
		if (!this)
			continue;
		
		if (b < this)
			continue;
		
		if (b - this < dist) {
			smallest = i;
			dist = b - this;
		}
	}
	
	if (smallest >= 0)
		return (smallest << 24) | dist;
	
	return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

uintptr_t __n64_pointer__;

GbiGfx n64_gbi_gfxhi_ptr(const void* ptr) {
	__n64_pointer__ = (uintptr_t)ptr;
	
	return gO_(G_SETPTRHI, 0, (uint64_t)__n64_pointer__ >> 32);
}

GbiGfx n64_gbi_gfxhi_seg(uint32_t seg) {
	__n64_pointer__ = seg;
	
	return gO_(G_NOOP, 0, 0);
}

void n64_set_onlyZmode(enum N64ZMode zmode) {
	gOnlyThisZmode = zmode;
}

void n64_set_onlyGeoLayer(enum N64GeoLayer geoLayer) {
	gOnlyThisGeoLayer = geoLayer;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void n64_mtx_model(void* data) {
	//memcpy(gMatrix.model, data, sizeof(gMatrix.model));
	gMatrix.modelNow = gMatrix.modelStack;
	memcpy(gMatrix.modelStack, data, sizeof(*gMatrix.modelStack));
	//logmatrix("n64_mtx_model()", data);
}

void n64_mtx_view(void* data) {
	memcpy(&gMatrix.view, data, sizeof(gMatrix.view));
	recompute_normal_matrix();
}

void n64_mtx_projection(void* data) {
	memcpy(&gMatrix.projection, data, sizeof(gMatrix.projection));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static uint8_t n64_graph_buffer[1024 * 1024 * 8];
static uint8_t* n64_graph_ptr;

void* n64_graph_alloc(uint32_t sz) {
	return (n64_graph_ptr += sz) - sz;
}

void n64_clear_cache(void) {
	ShaderList_cleanup();
	for (int32_t i = 0; i < gTexelCacheCount; i++) {
		gTexelDict[i] = 0;
		gTexel[i] = 0;
	}
	gTexelCacheCount = 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void n64_drawImpl(void* dlist) {
	uint8_t* cmd;
	
	if (!dlist)
		return;
	
	glDepthFunc(GL_LESS);
	
	if (!gVAO)
		glGenVertexArrays(1, &gVAO);
	if (!gVBO)
		glGenBuffers(1, &gVBO);
	if (!gEBO)
		glGenBuffers(1, &gEBO);
	
	/* set up texture stuff */
	if (!gTexel[0])
		glGenTextures(N64_TEXTURE_CACHE_SIZE, gTexel);
	
	/* set up geometry stuff */
	glBindVertexArray(gVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sVbuf), sVbuf, GL_DYNAMIC_DRAW);
	
	/* pos */
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(N64Vtx), (void*)offsetof(N64Vtx, pos));
	glEnableVertexAttribArray(0);
	
	/* color */
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(N64Vtx), (void*)offsetof(N64Vtx, color));
	glEnableVertexAttribArray(1);
	
	/* texcoord0 */
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(N64Vtx), (void*)offsetof(N64Vtx, texcoord0));
	glEnableVertexAttribArray(2);
	
	/* texcoord1 */
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(N64Vtx), (void*)offsetof(N64Vtx, texcoord1));
	glEnableVertexAttribArray(3);
	
	for (cmd = dlist; ; cmd += 8) {
		//fprintf(stderr, "%08x %08x\n", u32r(cmd), u32r(cmd + 4));
		bool shouldExit = (gGbi[*cmd] && gGbi[*cmd](cmd));
	#ifdef RENDERHOOK_UOT
		// Update history ringbuffer
		gMatState.history[gMatState.historyI] = *cmd;
		gMatState.historyI = (gMatState.historyI + 1) % ARRLEN(gMatState.history);
	#endif
		if (shouldExit)
			break;
	}
}

void n64_draw_dlist(void* dlist) {
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	n64_drawImpl(dlist);
}

#include <sys/time.h>
void n64_update_tick(void) {
	static struct timeval prev_time;
	struct timeval cur_time;
	double diff;
	
	gettimeofday(&cur_time, NULL);
	
	diff = (double)(cur_time.tv_sec - prev_time.tv_sec) + (double)(cur_time.tv_usec - prev_time.tv_usec) / 1000000.0;
	
	if ( (n64_tick_20fps = diff >= (1.0 / 19.5)) )
		prev_time = cur_time;
}

void n64_buffer_init(void) {
	
	sLightNum = 0;
	n64_buffer_clear();
	Shader_use(0);
	n64_set_onlyZmode(N64_ZMODE_ALL);
	n64_set_onlyGeoLayer(N64_GEOLAYER_ALL);
	
	if (!sOutlineShader) {
		sOutlineShader = Shader_new();
		
		const char* vtx = SHADER_SOURCE(
			layout (location = 0) in vec4 aPos;
			layout (location = 1) in vec4 aColor;
			layout (location = 2) in vec2 aTexCoord0;
			layout (location = 3) in vec2 aTexCoord1;
			layout (location = 4) in vec3 aNorm;
			
			// uniform mat4 model;
			uniform mat4 view;
			uniform mat4 projection;
			
			void main() {
				//gl_Position = projection * view * aPos; // wireframe method
				gl_Position = projection * view * vec4((aPos.xyz + aNorm * 1.0), 1.0); // inverse hull
			}
		);
		const char* frag = SHADER_SOURCE(
			out vec4 FragColor;
			uniform vec4 color;
			
			void main() {
				FragColor.rgba = color;
			}
		);
		Shader_update(sOutlineShader, vtx, frag);
	}
}

void n64_buffer_flush(bool drawDecalsSeparately)
{
	gSPEndDisplayList(POLY_OPA_DISP++);
	gSPEndDisplayList(POLY_XLU_DISP++);
	if (drawDecalsSeparately)
	{
		// supports maps that have xlu on the opa layer
		n64_set_onlyZmode(N64_ZMODE_OPA | N64_ZMODE_INTER | N64_ZMODE_XLU);
		n64_draw_dlist(n64_poly_opa_head);
		
		// decals
		n64_set_onlyZmode(N64_ZMODE_DEC);
		n64_draw_dlist(n64_poly_opa_head);
		
		// draw xlu
		n64_set_onlyZmode(N64_ZMODE_ALL);
		n64_draw_dlist(n64_poly_xlu_head);
	}
	else
	{
		n64_draw_dlist(n64_poly_opa_head);
		n64_draw_dlist(n64_poly_xlu_head);
	}
	n64_buffer_clear();
}

void n64_buffer_clear(void) {
	for (int i = 0; i <= 0xF; i++)
		n64_segment[i] = NULL;
	n64_poly_opa_disp = n64_poly_opa_head;
	n64_poly_xlu_disp = n64_poly_xlu_head;
	n64_graph_ptr = n64_graph_buffer;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool n64_culling(bool state) {
	return s_cull_enabled = state;
}

void n64_fog(int near, int far, uint8_t r, uint8_t g, uint8_t b) {
	int mult;
	int offset;
	
	near &= 0x3FF;
	
	if (near >= 1000)
		mult = offset = 0;
	else if (near >= 997)
		mult = 32767,
		offset = -32512;
	else if (near < 0)
		mult = 0,
		offset = 255;
	else
		mult = ((500 * 0x100) / (far - near)),
		offset = ((500 - near) * 0x100 / (far - near));
	
	gFog.fog[0] = mult;
	gFog.fog[1] = offset;
	
	gFog.color[0] = r * (1.0 / 255);
	gFog.color[1] = g * (1.0 / 255);
	gFog.color[2] = b * (1.0 / 255);
}

static bool n64_bind_light(GbiLight* lightInfo, GbiLightAmbient* ambient) {
	bool ret = EXIT_FAILURE;
	
	if (lightInfo && sLightNum < 7)	{
		sLights.l[sLightNum++] = *lightInfo;
		ret = EXIT_SUCCESS;
	}
	
	if (ambient) {
		sLights.a = *ambient;
		ret = EXIT_SUCCESS;
	}
	
	return ret;
}

bool n64_light_bind_dir(int8_t x, int8_t y, int8_t z, uint8_t r, uint8_t g, uint8_t b) {
	GbiLight light = {
		.dir.col = { r, g, b },
		.dir.dir = { x, y, z }
		,
	};
	
	return n64_bind_light(&light, NULL);
}

bool n64_light_bind_point(int16_t x, int16_t y, int16_t z, uint8_t r, uint8_t g, uint8_t b) {
	GbiLight light = {
		.point.c   = 1,
		.point.col = {
			r, g, b
		},
		.point.pos = {
			x, y, z
		}
		,
	};
	
	return n64_bind_light(&light, NULL);
}

void n64_light_set_ambient(uint8_t r, uint8_t g, uint8_t b) {
	GbiLightAmbient ambient = {
		.l.col = { r, g, b }
		,
	};
	
	n64_bind_light(NULL, &ambient);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void n64_set_tri_callback(void* userData, N64TriCallback callback) {
	s_tri_callback_data = userData;
	s_tri_callback = callback;
}

void n64_set_cull_callback(void* userData, N64CullCallback callback) {
	s_cull_callback_data = userData;
	s_cull_callback = callback;
}
