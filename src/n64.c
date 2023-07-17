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
#include <shader.h>
#include <glad/glad.h>

typedef bool (*GbiFunc)(void*);

typedef union {
	float mf[4][4];
	struct {
		float xx, yx, zx, wx,
			xy, yy, zy, wy,
			xz, yz, zz, wz,
			xw, yw, zw, ww;
	};
} Mtx;

typedef union {
	int32_t m[4][4];
	struct {
		uint16_t intPart[4][4];
		uint16_t fracPart[4][4];
	};
} GbiMtx;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void mtx_mul_vec4(N64Vector3* src, N64Vector4* vec, Mtx* mf) {
	vec->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	vec->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	vec->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
	vec->w = mf->ww + (mf->wx * src->x + mf->wy * src->y + mf->wz * src->z);
}

static void mtx_mul_vec3_rot(N64Vector3* src, N64Vector3* vec, Mtx* mf) {
	vec->x = (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	vec->y = (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	vec->z = (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
}

static void gbimtx_to_mtx(GbiMtx* src, Mtx* dest) {
	uint16_t* m1 = (void*)((uint8_t*)src);
	uint16_t* m2 = (void*)((uint8_t*)src + 0x20);
	
	dest->xx = ((m1[0] << 0x10) | m2[0]) * (1 / 65536.0f);
	dest->yx = ((m1[1] << 0x10) | m2[1]) * (1 / 65536.0f);
	dest->zx = ((m1[2] << 0x10) | m2[2]) * (1 / 65536.0f);
	dest->wx = ((m1[3] << 0x10) | m2[3]) * (1 / 65536.0f);
	dest->xy = ((m1[4] << 0x10) | m2[4]) * (1 / 65536.0f);
	dest->yy = ((m1[5] << 0x10) | m2[5]) * (1 / 65536.0f);
	dest->zy = ((m1[6] << 0x10) | m2[6]) * (1 / 65536.0f);
	dest->wy = ((m1[7] << 0x10) | m2[7]) * (1 / 65536.0f);
	dest->xz = ((m1[8] << 0x10) | m2[8]) * (1 / 65536.0f);
	dest->yz = ((m1[9] << 0x10) | m2[9]) * (1 / 65536.0f);
	dest->zz = ((m1[10] << 0x10) | m2[10]) * (1 / 65536.0f);
	dest->wz = ((m1[11] << 0x10) | m2[11]) * (1 / 65536.0f);
	dest->xw = ((m1[12] << 0x10) | m2[12]) * (1 / 65536.0f);
	dest->yw = ((m1[13] << 0x10) | m2[13]) * (1 / 65536.0f);
	dest->zw = ((m1[14] << 0x10) | m2[14]) * (1 / 65536.0f);
	dest->ww = ((m1[15] << 0x10) | m2[15]) * (1 / 65536.0f);
}

static void mtx_mtx_mul(Mtx* mfA, Mtx* mfB, Mtx* dest) {
	float cx;
	float cy;
	float cz;
	float cw;
	float rx = mfA->xx;
	float ry = mfA->xy;
	float rz = mfA->xz;
	float rw = mfA->xw;
	
	//--------
	
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->xx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->xy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->xz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->xw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW2---
	rx = mfA->yx;
	ry = mfA->yy;
	rz = mfA->yz;
	rw = mfA->yw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->yx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->yy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->yz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->yw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW3---
	rx = mfA->zx;
	ry = mfA->zy;
	rz = mfA->zz;
	rw = mfA->zw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->zx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->zy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->zz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->zw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW4---
	rx = mfA->wx;
	ry = mfA->wy;
	rz = mfA->wz;
	rw = mfA->ww;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->wx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->wy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->wz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->ww = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
}

static void mtx_translate_rot(Mtx* m, N64Vector3* translation, N64Vector3* rotation) {
	float sin = sinf(rotation->z);
	float cos = cosf(rotation->z);
	float temp1;
	float temp2;
	
	temp1 = m->xx;
	temp2 = m->xy;
	m->xw += temp1 * translation->x + temp2 * translation->y + m->xz * translation->z;
	m->xx = temp1 * cos + temp2 * sin;
	m->xy = temp2 * cos - temp1 * sin;
	
	temp1 = m->yx;
	temp2 = m->yy;
	m->yw += temp1 * translation->x + temp2 * translation->y + m->yz * translation->z;
	m->yx = temp1 * cos + temp2 * sin;
	m->yy = temp2 * cos - temp1 * sin;
	
	temp1 = m->zx;
	temp2 = m->zy;
	m->zw += temp1 * translation->x + temp2 * translation->y + m->zz * translation->z;
	m->zx = temp1 * cos + temp2 * sin;
	m->zy = temp2 * cos - temp1 * sin;
	
	temp1 = m->wx;
	temp2 = m->wy;
	m->ww += temp1 * translation->x + temp2 * translation->y + m->wz * translation->z;
	m->wx = temp1 * cos + temp2 * sin;
	m->wy = temp2 * cos - temp1 * sin;
	
	if (rotation->y != 0.0f) {
		sin = sinf(rotation->y);
		cos = cosf(rotation->y);
		
		temp1 = m->xx;
		temp2 = m->xz;
		m->xx = temp1 * cos - temp2 * sin;
		m->xz = temp1 * sin + temp2 * cos;
		
		temp1 = m->yx;
		temp2 = m->yz;
		m->yx = temp1 * cos - temp2 * sin;
		m->yz = temp1 * sin + temp2 * cos;
		
		temp1 = m->zx;
		temp2 = m->zz;
		m->zx = temp1 * cos - temp2 * sin;
		m->zz = temp1 * sin + temp2 * cos;
		
		temp1 = m->wx;
		temp2 = m->wz;
		m->wx = temp1 * cos - temp2 * sin;
		m->wz = temp1 * sin + temp2 * cos;
	}
	
	if (rotation->x != 0.0f) {
		sin = sinf(rotation->x);
		cos = cosf(rotation->x);
		
		temp1 = m->xy;
		temp2 = m->xz;
		m->xy = temp1 * cos + temp2 * sin;
		m->xz = temp2 * cos - temp1 * sin;
		
		temp1 = m->yy;
		temp2 = m->yz;
		m->yy = temp1 * cos + temp2 * sin;
		m->yz = temp2 * cos - temp1 * sin;
		
		temp1 = m->zy;
		temp2 = m->zz;
		m->zy = temp1 * cos + temp2 * sin;
		m->zz = temp2 * cos - temp1 * sin;
		
		temp1 = m->wy;
		temp2 = m->wz;
		m->wy = temp1 * cos + temp2 * sin;
		m->wz = temp2 * cos - temp1 * sin;
	}
}

static void mtx_to_gbimtx(Mtx* src, GbiMtx* dest) {
	int temp;
	uint16_t* m1 = (void*)(((char*)dest));
	uint16_t* m2 = (void*)(((char*)dest) + 0x20);
	
	temp = src->xx * 0x10000;
	m1[0] = (temp >> 0x10);
	m1[16 + 0] = temp & 0xFFFF;
	
	temp = src->yx * 0x10000;
	m1[1] = (temp >> 0x10);
	m1[16 + 1] = temp & 0xFFFF;
	
	temp = src->zx * 0x10000;
	m1[2] = (temp >> 0x10);
	m1[16 + 2] = temp & 0xFFFF;
	
	temp = src->wx * 0x10000;
	m1[3] = (temp >> 0x10);
	m1[16 + 3] = temp & 0xFFFF;
	
	temp = src->xy * 0x10000;
	m1[4] = (temp >> 0x10);
	m1[16 + 4] = temp & 0xFFFF;
	
	temp = src->yy * 0x10000;
	m1[5] = (temp >> 0x10);
	m1[16 + 5] = temp & 0xFFFF;
	
	temp = src->zy * 0x10000;
	m1[6] = (temp >> 0x10);
	m1[16 + 6] = temp & 0xFFFF;
	
	temp = src->wy * 0x10000;
	m1[7] = (temp >> 0x10);
	m1[16 + 7] = temp & 0xFFFF;
	
	temp = src->xz * 0x10000;
	m1[8] = (temp >> 0x10);
	m1[16 + 8] = temp & 0xFFFF;
	
	temp = src->yz * 0x10000;
	m1[9] = (temp >> 0x10);
	m2[9] = temp & 0xFFFF;
	
	temp = src->zz * 0x10000;
	m1[10] = (temp >> 0x10);
	m2[10] = temp & 0xFFFF;
	
	temp = src->wz * 0x10000;
	m1[11] = (temp >> 0x10);
	m2[11] = temp & 0xFFFF;
	
	temp = src->xw * 0x10000;
	m1[12] = (temp >> 0x10);
	m2[12] = temp & 0xFFFF;
	
	temp = src->yw * 0x10000;
	m1[13] = (temp >> 0x10);
	m2[13] = temp & 0xFFFF;
	
	temp = src->zw * 0x10000;
	m1[14] = (temp >> 0x10);
	m2[14] = temp & 0xFFFF;
	
	temp = src->ww * 0x10000;
	m1[15] = (temp >> 0x10);
	m2[15] = temp & 0xFFFF;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static N64Vector3 vec3_normalize(N64Vector3 vec) {
	N64Vector3 ret;
	float mgn = sqrtf(
		(vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z)
	);
	
	if (mgn == 0) {
		ret.x = ret.y = ret.z = 0;
	} else {
		ret.x = vec.x / mgn;
		ret.y = vec.y / mgn;
		ret.z = vec.z / mgn;
	}
	
	return ret;
}

static float vec3_dot(N64Vector3 a, N64Vector3 b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static N64Vector4 vec4_color(uint8_t color[3]) {
	const float scale = 1 / 255.0f;
	
	return (N64Vector4) { color[0] * scale, color[1] * scale, color[2] * scale, 0.0f };
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
	uint8_t r, g, b, a;
} RGBA8;

typedef struct {
	uint8_t r, g, b;
} RGB8;

typedef struct ShaderList {
	struct ShaderList* next;
	Shader*  shader;
	uint64_t uuid;
} ShaderList;

GbiGfx n64_poly_opa_head[N64_OPA_STACK_SIZE];
GbiGfx* n64_poly_opa_disp;
GbiGfx n64_poly_xlu_head[N64_XLU_STACK_SIZE];
GbiGfx* n64_poly_xlu_disp;
void* n64_segment[N64_SEGMENT_MAX];

static GLuint gVAO;
static GLuint gVBO;
static GLuint gEBO;
static GLuint gTexel[N64_TEXTURE_CACHE_SIZE];
static GLubyte gIndices[4096];
static N64Vtx sVbuf[N64_VBUF_MAX];
static uint32_t gIndicesUsed = 0;
static int gTexelCacheCount = 0; // number of textures cached thus far
static struct {
	void* data;
} gTexelDict[N64_TEXTURE_CACHE_SIZE];
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
static bool gHideGeometry = false;
static bool gVertexColors = false;
static bool gFogEnabled = true;
static bool gForceBl = false;
static bool gCvgXalpha = false;
static bool s_cull_enabled = true;
static int gPolygonOffset = 0;
static enum N64GeoLayer gOnlyThisGeoLayer;
static enum N64ZMode gOnlyThisZmode;
static enum N64ZMode gCurrentZmode;
static ShaderList* sShaderList = 0;
static int sLightNum;
static GbiLightsN sLights;

static bool s_tick_20fps;

static struct {
	//float model[16];
	Mtx  view;
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
} gMatState; /* material state magic */

const Mtx sClearMtx = {
	.mf[0] = { 1.0f, 0.0f, 0.0f, 0.0f },
	.mf[1] = { 0.0f, 1.0f, 0.0f, 0.0f },
	.mf[2] = { 0.0f, 0.0f, 1.0f, 0.0f },
	.mf[3] = { 0.0f, 0.0f, 0.0f, 1.0f },
};

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
			if (gMatState.tile[tile].data == gTexelDict[i].data)
				break;
			
			if (gTexelDict[i].data == 0) {
				isNew = true;
				gTexelDict[i].data = gMatState.tile[tile].data;
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
		
		int fmt = gMatState.tile[tile].fmt;
		int siz = gMatState.tile[tile].siz;
		
		unsigned wrapT = GL_REPEAT;
		unsigned wrapS = GL_REPEAT;
		
		gMatState.texWidth = width;
		gMatState.texHeight = height;
		
		switch (gMatState.tile[tile].cmT) {
			case G_TX_MIRROR:
				wrapT = GL_MIRRORED_REPEAT;
				break;
			case G_TX_CLAMP:
				wrapT = GL_CLAMP_TO_EDGE;
				break;
		}
		
		switch (gMatState.tile[tile].cmS) {
			case G_TX_MIRROR:
				wrapS = GL_MIRRORED_REPEAT;
				break;
			case G_TX_CLAMP:
				wrapS = GL_CLAMP_TO_EDGE;
				break;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		
		if (gHideGeometry)
			continue;
		
		//uls >>= 2; /* discard precision; sourcing pixels directly */
		//ult >>= 2;
		
		/* TODO emulate dxt */
		//dxt = 0;
		
		//src += ult * width;
		//src += uls;
		//fprintf(stderr, "%d %d\n", fmt, siz);
		//memcpy(tmem, src, bytes); /* TODO dxt emulation requires line-by-line */
		if (isNew && gTexelCacheCount < N64_TEXTURE_CACHE_SIZE)	{
			uint8_t wow[4096 * 8];
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
		
		// TODO also populate lodfrac, prim.lodfrac, k4, and k5 here
		
		if (isNew) {
			
			#define SHADER_SOURCE(...) "#version 330 core\n" # __VA_ARGS__
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
				
#undef SHADER_SOURCE
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
	if (!( (b[8] != G_TRI1 && b[8] != G_TRI2) || gIndicesUsed + 6 >= N64_ARRAY_COUNT(gIndices) ))
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
	
	glDrawElements(GL_TRIANGLES, gIndicesUsed, GL_UNSIGNED_BYTE, 0);
	gIndicesUsed = 0;
}

static void othermode(void) {
	#define G_RM_CYCLE1_MASK 0xCCCC0000
	#define G_RM_CYCLE2_MASK 0x33330000
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
	if ((lo & G_RM_CYCLE1_MASK) == GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1))
		gMatState.mixFog = false;
	
	gCurrentZmode = (indep & 0b0000110000000) >> 7;
	gForceBl = (indep & 0b0100000000000) >> 11;
	gCvgXalpha = (indep & 0b0001000000000) >> 9;
	
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
	if (gOnlyThisZmode != N64_ZMODE_ALL && gCurrentZmode != gOnlyThisZmode)
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
		
		if (gVertexColors) {
			dst->color.x = vtx->color.r * (1.0 / 255.0);
			dst->color.y = vtx->color.g * (1.0 / 255.0);
			dst->color.z = vtx->color.b * (1.0 / 255.0);
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
	
	return false;
}

static bool gbiFunc_setenvcolor(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.env.hi = u32r(b);
	gMatState.env.lo = u32r(b + 4);
	
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
		bool wasDirectAddress = gPtrHiSet;
		mtx = n64_segment_get(mtxaddr);
		
		if (!mtx)
			return false;
		
		GbiMtx swap = *mtx;
		
		if (wasDirectAddress == false && (mtxaddr & 0xFF000000) != 0x01000000 && (mtxaddr & 0xFF000000) != 0x0D000000) {
			for (int32_t i = 0; i < 0x40 / 2; i++) {
				uint16_t* ss = (uint16_t*)&swap;
				ss[i] = u32r(&ss[i]);
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
	// uint8_t* b = cmd;
	// uint32_t hi = u32r(b);
	// uint32_t lo = u32r(b + 4);
	// int vbidx0 = ((hi >> 12) & 0xfff) / 5;
	// int vbidx1 = (hi & 0xfff) / 2;
	
	/* TODO simulate branching; for now, just draw everything */
	n64_drawImpl(n64_segment_get(gRdpHalf1));
	
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

static GbiFunc gGbi[0xFF] = {
	[G_VTX] =             gbiFunc_vtx,
	[G_CULLDL] =          gbiFunc_culldl,
	[G_TRI1] =            gbiFunc_tri1,
	[G_TRI2] =            gbiFunc_tri2,
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
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void* s_prev_seg;
static int s_prev_seg_id;

static void segment_swap_to(int seg, const void* data) {
	s_prev_seg = n64_segment[seg];
	s_prev_seg_id = seg;
	n64_segment[seg] = (void*)data;
}

static void segment_swap_back(void) {
	n64_segment[s_prev_seg_id] = s_prev_seg;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define OBJ_TYPE_NONE     0
#define OBJ_TYPE_DLIST    1
#define OBJ_TYPE_SKELETON 2
#define OBJ_REG_A_SIZE    0x200
#define OBJ_REG_B_SIZE    0x3F

typedef struct {
	float       speed;
	float       cur;
	uint32_t    start;
	uint32_t    end;
	uint32_t    frame_num;
	N64Vector3* frame_tbl /* [limb_num * frame_num] */;
	N64Vector3* anim_rot /* [limb_num] */;
	N64Vector3* anim_morph /* [limb_num] */;
	N64Vector3  pos;
} Anim;

typedef struct {
	N64Vector3 pos;
	uint32_t   dlist;
	uint8_t    child;
	uint8_t    sibling;
} Limb;

typedef struct {
	const void* data;
	struct {
		uint8_t seg_id : 6;
		uint8_t type   : 2;
	};
	union {
		struct {
			Limb*    limb;
			uint32_t limb_num;
		};
		struct {
			GbiGfx*  dlist;
			uint32_t dlist_num;
		};
	};
} Object;

static_assert(offsetof(Object, dlist) == offsetof(Object, limb));
static_assert(offsetof(Object, limb_num) == offsetof(Object, dlist_num));

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static Object s_object_list[OBJ_REG_A_SIZE][OBJ_REG_B_SIZE];

static Object* get_obj(uint16_t obj_id, uint8_t uniq_id) {
	assert(obj_id < OBJ_REG_A_SIZE);
	assert(uniq_id < OBJ_REG_B_SIZE);
	return &s_object_list[obj_id][uniq_id];
}

void n64_register_skeleton(uint16_t obj_id, uint8_t uniq_id, const void* data, uint8_t seg_id, uint32_t skel_offset) {
	Object* obj = get_obj(obj_id, uniq_id);
	
	assert(obj->type == OBJ_TYPE_NONE);
	obj->type = OBJ_TYPE_SKELETON;
	obj->data = data;
	obj->seg_id = seg_id;
	
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint32_t list_segment;
		uint8_t  limb_num;
	} SkeletonHeader;
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint32_t segment;
	} SegmentList;
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t  pos_x, pos_y, pos_z;
		uint8_t  child;
		uint8_t  sibling;
		uint32_t dlist;
	} ZLimb;
	
	segment_swap_to(seg_id, obj->data);
	SkeletonHeader* header = n64_segment_get(skel_offset);
	SegmentList* list = n64_segment_get(header->list_segment);
	
	obj->limb_num = header->limb_num;
	assert((obj->limb = calloc(obj->limb_num, sizeof(Limb))) != NULL);
	
	for (uint32_t i = 0; i < obj->limb_num; i++) {
		ZLimb* limb = n64_segment_get(list[i].segment);
		
		if (!limb) continue;
		
		obj->limb[i] = (Limb) {
			.pos = { limb->pos_x, limb->pos_y, limb->pos_z },
			.child = limb->child,
			.sibling = limb->sibling,
			.dlist = limb->dlist,
		};
	}
	
	segment_swap_back();
}

void n64_register_dlist(uint16_t obj_id, uint8_t uniq_id, const void* data, uint8_t seg_id, GbiGfx* dlist, int dlist_num) {
	Object* obj = get_obj(obj_id, uniq_id);
	
	assert(obj->type == OBJ_TYPE_NONE);
	
	obj->type = OBJ_TYPE_DLIST;
	obj->data = data;
	obj->seg_id = seg_id;
	assert((obj->dlist = calloc(dlist_num, sizeof(GbiGfx))) != NULL);
	obj->dlist_num = dlist_num;
	
	memcpy(obj->dlist, dlist, sizeof(GbiGfx[dlist_num]));
}

void n64_unregister(uint16_t obj_id, uint8_t uniq_id) {
	Object* obj = get_obj(obj_id, uniq_id);
	
	if (obj->type == OBJ_TYPE_NONE)
		return;
	
	// free limb/dlist
	if (obj->limb)
		free(obj->limb);
	*obj = (Object) {};
}

typedef struct N64Object {
	uint16_t obj_id;
	uint8_t  uniq_id;
	uint8_t  mtl_setup_dl_id;
	Anim*    anim;
	Mtx      mtx;
} N64Object;

N64Object* n64_object_new(uint16_t obj_id, uint8_t uniq_id, uint8_t mtl_setup_dl_id) {
	N64Object* obj = calloc(1, sizeof(N64Object));
	
	obj->obj_id = obj_id;
	obj->uniq_id = uniq_id;
	obj->mtl_setup_dl_id = mtl_setup_dl_id;
	
	return obj;
}

static void n64anim_destroy(Anim** self) {
	if (*self) {
		free((*self)->frame_tbl);
		free(*self);
	}
	*self = NULL;
}

void n64_object_destroy(N64Object* self) {
	n64anim_destroy(&self->anim);
}

void n64_object_set_anim(N64Object* self, uint32_t seg_anim, float speed) {
	#define BIN_TO_RAD(binang) ((float)((int32_t)binang) * (3.14159265359 / 0x8000))
	Object* obj = get_obj(self->obj_id, self->uniq_id);
	
	assert(obj->type == OBJ_TYPE_SKELETON);
	
	segment_swap_to(obj->seg_id, obj->data);
	n64anim_destroy(&self->anim);
	
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t binang;
	} Frame;
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint16_t x, y, z;
	} JointIndex;
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t  frame_num;
		uint32_t seg_tbl;
		uint32_t seg_jnt_id_tbl;
		uint16_t max;
	} AnimHeader;
	
	AnimHeader* header = n64_segment_get(seg_anim);
	Frame* frame_tbl = n64_segment_get(header->seg_tbl);
	Anim* anim;
	N64Vector3* dst;
	
	assert((anim = self->anim = calloc(1, sizeof(Anim))) != NULL);
	
	anim->speed = speed;
	anim->frame_num = anim->end = header->frame_num;
	
	assert((dst = anim->frame_tbl = calloc(anim->frame_num * obj->limb_num, sizeof(N64Vector3))) != NULL);
	assert((anim->anim_rot = calloc(obj->limb_num, sizeof(N64Vector3))) != NULL);
	assert((anim->anim_morph = calloc(obj->limb_num, sizeof(N64Vector3))) != NULL);
	
	for (uint32_t k = 0; k < anim->frame_num; k++) {
		uint16_t static_max = header->max;
		Frame* fstatic = frame_tbl;
		Frame* fdynamic = &frame_tbl[k];
		JointIndex* jnt_id = n64_segment_get(header->seg_jnt_id_tbl);
		
		for (uint32_t i = 0; i < obj->limb_num; i++, dst++, jnt_id++) {
			if (i == 0) {
				dst->x = (jnt_id->x >= static_max) ? fdynamic[jnt_id->x].binang : fstatic[jnt_id->x].binang;
				dst->y = (jnt_id->y >= static_max) ? fdynamic[jnt_id->y].binang : fstatic[jnt_id->y].binang;
				dst->z = (jnt_id->z >= static_max) ? fdynamic[jnt_id->z].binang : fstatic[jnt_id->z].binang;
			} else {
				dst->x = BIN_TO_RAD( (jnt_id->x >= static_max) ? fdynamic[jnt_id->x].binang : fstatic[jnt_id->x].binang );
				dst->y = BIN_TO_RAD( (jnt_id->y >= static_max) ? fdynamic[jnt_id->y].binang : fstatic[jnt_id->y].binang );
				dst->z = BIN_TO_RAD( (jnt_id->z >= static_max) ? fdynamic[jnt_id->z].binang : fstatic[jnt_id->z].binang );
			}
		}
	}
	
	segment_swap_back();
}

static void draw_limb(N64Object* self, Object* obj, int id, GbiMtx** gbimtx, Mtx* mtx) {
	Limb* limb = &obj->limb[id];
	
	#define MTX_PUSH() mtx++; *mtx = mtx[-1]
	#define MTX_POP()  mtx--
	
	MTX_PUSH();
	
	N64Vector3 zero = {};
	N64Vector3* pos = &limb->pos;
	N64Vector3* rot = &zero;
	
	if (self->anim) {
		Anim* anim = self->anim;
		
		if (id == 0) pos = &anim->pos;
		rot = &anim->anim_rot[id];
	}
	
	mtx_translate_rot(mtx, pos, rot);
	
	if (limb->dlist) {
		if (gbimtx && *gbimtx) {
			mtx_to_gbimtx(mtx, (*gbimtx));
			gSPMatrix(POLY_OPA_DISP++, (*gbimtx), G_MTX_LOAD);
			(*gbimtx)++;
		}
		gSPDisplayList(POLY_OPA_DISP++, limb->dlist);
	}
	
	if (limb->child != 0xFF)
		draw_limb(self, obj, limb->child, gbimtx, mtx);
	
	MTX_POP();
	
	if (limb->sibling != 0xFF)
		draw_limb(self, obj, limb->sibling, gbimtx, mtx);
	
#undef MTX_PUSH
#undef MTX_POP
}

static float lerp(float v, float min, float max) {
	return min + v * (max - min);
}

typedef struct {
	float w;
	float x;
	float y;
	float z;
} N64Quat;

N64Quat vec3_to_quat(N64Vector3 vec) {
	N64Quat q;
	float cos_z = cosf(vec.z / 2);
	float sin_z = sinf(vec.z / 2);
	float cos_y = cosf(vec.y / 2);
	float sin_y = sinf(vec.y / 2);
	float cos_x = cosf(vec.x / 2);
	float sin_x = sinf(vec.x / 2);
	
	q.w = cos_z * cos_y * cos_x + sin_z * sin_y * sin_x;
	q.x = cos_z * cos_y * sin_x - sin_z * sin_y * cos_x;
	q.y = cos_z * sin_y * cos_x + sin_z * cos_y * sin_x;
	q.z = sin_z * cos_y * cos_x - cos_z * sin_y * sin_x;
	
	return q;
}

N64Vector3 quat_to_vec3(N64Quat q) {
	N64Vector3 r;
	float sinp = 2 * (q.w * q.y - q.z * q.x);
	
	r.x = atan2f(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y));
	
	if (fabsf(sinp) >= 1)
		r.y = copysignf(M_PI / 2, sinp);
	else
		r.y = asinf(sinp);
	
	r.z = atan2f(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z));
	
	return r;
}

N64Quat quat_lerp(float t, N64Quat a, N64Quat b) {
	N64Quat result;
	
	float factor = 1.0 - t;
	
	result.w = factor * a.w + t * b.w;
	result.x = factor * a.x + t * b.x;
	result.y = factor * a.y + t * b.y;
	result.z = factor * a.z + t * b.z;
	
	float magnitude = sqrt(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
	result.w /= magnitude;
	result.x /= magnitude;
	result.y /= magnitude;
	result.z /= magnitude;
	
	return result;
}

static void update_anim(N64Object* self, Object* obj, Anim* anim) {
	int id_cur_frame = floor(anim->cur);
	float mod = anim->cur - id_cur_frame;
	int id_next_frame = (id_cur_frame + 1) % anim->end;
	
	N64Vector3* frame = &anim->frame_tbl[id_cur_frame * obj->limb_num];
	N64Vector3* next_frame = &anim->frame_tbl[id_next_frame * obj->limb_num];
	
	anim->pos.x = lerp(mod, frame->x, next_frame->x);
	anim->pos.y = lerp(mod, frame->y, next_frame->y);
	anim->pos.z = lerp(mod, frame->z, next_frame->z);
	
	for (uint32_t i = 1; i < obj->limb_num; i++) {
		if (mod) {
			N64Quat qa = vec3_to_quat(frame[i]);
			N64Quat qb = vec3_to_quat(next_frame[i]);
			anim->anim_rot[i - 1] = quat_to_vec3(quat_lerp(mod, qa, qb));
		} else
			anim->anim_rot[i - 1] = frame[i];
	}
	
	anim->cur = fmod(anim->cur + anim->speed, anim->end);
}

static void draw_skeleton(N64Object* self, Object* obj) {
	Mtx mtx[obj->limb_num + 1];
	GbiMtx* gbimtx = n64_graph_alloc(sizeof(GbiMtx[obj->limb_num + 1]));
	GbiMtx curmtx;
	
	memcpy(mtx, &self->mtx, sizeof(Mtx));
	mtx_to_gbimtx(&self->mtx, &curmtx);
	
	gSPDisplayList(POLY_OPA_DISP++, n64_material_setup_dl[self->mtl_setup_dl_id]);
	gSPSegment(POLY_OPA_DISP++, obj->seg_id, (void*)obj->data);
	gSPMatrix(POLY_OPA_DISP++, &curmtx, G_MTX_LOAD);
	gSPSegment(POLY_OPA_DISP++, 0xD, gbimtx);
	
	if (self->anim && s_tick_20fps)
		update_anim(self, obj, self->anim);
	
	draw_limb(self, obj, 0, &gbimtx, mtx);
}

void n64_object_draw(N64Object* self) {
	Object* obj = get_obj(self->obj_id, self->uniq_id);
	
	switch (obj->type) {
		case OBJ_TYPE_DLIST:
			break;
			
		case OBJ_TYPE_SKELETON:
			draw_skeleton(self, obj);
			break;
	}
}

void n64_object_set_mtx(N64Object* self, const void* mtx) {
	self->mtx = *(const Mtx*)mtx;
}

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

GbiGfx n64_gbi_gfxhi_ptr(void* ptr) {
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
}

void n64_mtx_view(void* data) {
	memcpy(&gMatrix.view, data, sizeof(gMatrix.view));
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
		gTexelDict[i].data = 0;
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
		if (gGbi[*cmd] && gGbi[*cmd](cmd))
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
	
	if ( (s_tick_20fps = diff >= (1.0 / 19.5)) )
		prev_time = cur_time;
}

void n64_buffer_init(void) {
	
	sLightNum = 0;
	n64_buffer_clear();
	Shader_use(0);
	n64_set_onlyZmode(N64_ZMODE_ALL);
	n64_set_onlyGeoLayer(N64_GEOLAYER_ALL);
}

void n64_buffer_flush(void) {
	gSPEndDisplayList(POLY_OPA_DISP++);
	gSPEndDisplayList(POLY_XLU_DISP++);
	n64_draw_dlist(n64_poly_opa_head);
	n64_draw_dlist(n64_poly_xlu_head);
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
