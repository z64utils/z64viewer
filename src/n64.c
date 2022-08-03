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

typedef float MtxF_t[4][4];
typedef union {
	MtxF_t mf;
	struct {
		float xx, yx, zx, wx,
			xy, yy, zy, wy,
			xz, yz, zz, wz,
			xw, yw, zw, ww;
	};
} MtxF;

typedef union {
	int32_t m[4][4];
	struct {
		uint16_t intPart[4][4];
		uint16_t fracPart[4][4];
	};
} Mtx;

typedef struct {
	uint8_t r, g, b, a;
} RGBA8;

typedef struct {
	uint8_t r, g, b;
} RGB8;

typedef struct {
	float x, y, z, w;
} Vec4f;

typedef struct {
	float x, y, z;
} Vec3f;

typedef struct VtxS {
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
} VtxS;

typedef struct VtxF {
	Vec4f pos;
	struct {
		float u;
		float v;
	} texcoord0, texcoord1;
	Vec4f color;
	Vec3f norm;
} VtxF;

#include <n64.h>
#include <n64texconv.h>
#include <bigendian.h>
#include <shader.h>
#include <glad/glad.h>

#define TEXTURE_CACHE_SIZE 256

static GLuint gVAO;
static GLuint gVBO;
static GLuint gEBO;
static GLuint gTexel[TEXTURE_CACHE_SIZE];
static GLubyte gIndices[4096];
static int gIndicesUsed = 0;
static int gTexelCacheCount = 0; // number of textures cached thus far
static struct {
	void* data;
} gTexelDict[TEXTURE_CACHE_SIZE];
static GLint gFilterMode = GL_LINEAR;
static void* sTriangleCallbackUserData;
static n64_triangleCallbackFunc sTriangleCallback;

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
static bool gCullDLenabled = true;
static bool gGxOutline = false;

static int gPolygonOffset = 0;

Gfx gPolyOpaHead[4096];
Gfx* gPolyOpaDisp;
Gfx gPolyXluHead[4096];
Gfx* gPolyXluDisp;
uint8_t gSegCheckBuf[64];

static enum n64_geoLayer gOnlyThisGeoLayer;
static enum n64_zmode gOnlyThisZmode;
static enum n64_zmode gCurrentZmode;

typedef struct ShaderList {
	struct ShaderList* next;
	Shader*  shader;
	uint64_t uuid;
} ShaderList;

static ShaderList* sShaderList = 0;

static struct {
	//float model[16];
	MtxF  view;
	MtxF  projection;
	MtxF  modelStack[MATRIX_STACK_MAX];
	MtxF* modelNow;
} gMatrix;

static int sLightNum;
static Lights7 sLights;

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
		
		int      fmt;
		int      siz;
		int      line;
		int      tmem;
		int      tile;
		int      palette;
		int      cmT;
		int      maskT;
		int      shiftT;
		int      cmS;
		int      maskS;
		int      shiftS;
		float    shiftS_m;
		float    shiftT_m;
		bool     doUpdate;
	} tile[2];
	struct {
		void* imgaddr;
		int   fmt;
		int   siz;
		int   width;
	} timg;
	uint16_t pal[256]; /* palette */
	int mtlReady;
	int texWidth;
	int texHeight;
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
		float    lodfrac; // TODO not yet populated
	} prim;
	struct {
		uint32_t hi;
		uint32_t lo;
		float    r;
		float    g;
		float    b;
		float    alpha;
	} env;
	float    lodfrac; // TODO not yet populated
	float    k4; // TODO not yet populated
	float    k5; // TODO not yet populated
	uint32_t geometrymode;
	bool mixFog; // Condition to mix fog into the material
} gMatState; /* material state magic */

void* gSegment[SEGMENT_MAX] = { 0 };
static VtxF gVbuf[VBUF_MAX];
const MtxF sClearMtx = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

typedef bool (* gbiFunc)(void* cmd);

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
	} else { /* false */
		glDisable(GL_BLEND);
	}
	
	gHideGeometry = false;
	if (gOnlyThisZmode != ZMODE_ALL && gCurrentZmode != gOnlyThisZmode)
		gHideGeometry = true;
	
	if (!gHideGeometry && gOnlyThisGeoLayer != GEOLAYER_ALL) {
		int isOverlay = gForceBl || gCvgXalpha;
		switch (gOnlyThisGeoLayer) {
			case GEOLAYER_OPAQUE:
				if (isOverlay)
					gHideGeometry = true;
				break;
				
			case GEOLAYER_OVERLAY:
				if (!isOverlay)
					gHideGeometry = true;
				break;
				
			case GEOLAYER_ALL:
				break;
		}
	}
	
	/* hack for eliminating z-fighting on decals */
	switch (gCurrentZmode) {
		case ZMODE_DEC: /* ZMODE_DEC */
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

/* like strcat, but returns pointer to tail of appended string */
static char* strcatt(char* dst, const char* src) {
	size_t n = strlen(src);
	
	memcpy(dst, src, n + 1);
	
	return dst + n;
}

/* like strcatt, but allows fancy string formatting */
static char* strcattf(char* dst, const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	vsprintf(dst, fmt, args);
	va_end(args);
	
	return dst + strlen(dst);
}

/*static const char* quickstr(const char* fmt, ...) {
        static char buf[256];
        va_list args;

        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        return buf;
   }*/

static const char* colorValueString(int idx, int v) {
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

static const char* alphaValueString(int idx, int v) {
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

static void doMaterial(void* addr) {
	int tile = 0; /* G_TX_RENDERTILE */
	
	/* update texture image associated with each tile */
	for (tile = 0; tile < 2; ++tile) {
		int i;
		bool isNew = false;
		
		if (!gMatState.tile[tile].doUpdate)
			continue;
		
		for (i = 0; i < TEXTURE_CACHE_SIZE; ++i) {
			if (gMatState.tile[tile].data == gTexelDict[i].data)
				break;
			
			if (gTexelDict[i].data == 0) {
				isNew = true;
				gTexelDict[i].data = gMatState.tile[tile].data;
				break;
			}
		}
		// no match found
		if (i == TEXTURE_CACHE_SIZE) {
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
		if (isNew && gTexelCacheCount < TEXTURE_CACHE_SIZE) {
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
	
	#define SHADER_SOURCE(...) "#version 330 core\n" # __VA_ARGS__
	/* uuid tracks state changes; if no states changed, don't compile new shader */
	{
		uint64_t uuid =
			(gFogEnabled << 63)
			| ((gCvgXalpha || gForceBl) << 62)
			| ((gMatState.setcombine.hi & 0x00ffffff) << 32)
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
			const char* vtx = SHADER_SOURCE(
				layout (location = 0) in vec4 aPos;
				layout (location = 1) in vec4 aColor;
				layout (location = 2) in vec2 aTexCoord0;
				layout (location = 3) in vec2 aTexCoord1;
				layout (location = 4) in vec3 aNorm;
				
				out vec4 vColor;
				out vec2 TexCoord0;
				out vec2 TexCoord1;
				out float vFog;
				out vec3 vLightColor;
				
				// uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;
				uniform vec2 uFog;
				
				float fog_linear(const float dist, const float start, const float end) {
				float s = clamp((end - dist) / (end - start), 0.0, 1.0);
				
				return 1.0 - s;
			}
				
				vec3 mul(mat4 a, vec3 b) {
				return (a * vec4(b, 1.0)).xyz;
			}
				
				void main() {
				float fogM = uFog.x;
				float fogO = uFog.y;
				vec4 wow = projection * view * vec4(aPos.xyz, 1.0);
				
				gl_Position = projection * view * aPos;
				vColor = aColor;
				TexCoord0 = vec2(aTexCoord0.x, aTexCoord0.y);
				TexCoord1 = vec2(aTexCoord1.x, aTexCoord1.y);
				if (wow.w < 0) {
					vFog = -fogM + fogO;
				} else {
					vFog = wow.z / wow.w * fogM + fogO;
				}
				vFog = clamp(vFog, 0.0, 255.0) / 255;
				
				/* when lighting is disabled for a vertex, its normal == 0 */
				if (aNorm == vec3(0.0))
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
				uniform vec4 uEnvColor;
				uniform float uK4;
				uniform float uK5;
				uniform float uLodFrac;
				uniform float uPrimLodFrac;
				
				/*void main() // simple default for testing...
				                                {
				                                                 vec4 s = texture(texture0, TexCoord0);
				                                                 if (s.a < 0.5)
				                                                                                 discard;
				                                                 FragColor = s * vColor;

				                                                 FragColor.rgb = mix(FragColor.rgb * vLightColor, uFogColor, vFog);
				                                }*/
			);
			
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
				
				if (gCvgXalpha || gForceBl) {
					/* alpha cycle 0 */
					ADDF("alpha = %s;", alphaValueString(0, (hi >> 12) & 0x7));
					ADDF("alpha -= %s;", alphaValueString(1, (lo >> 12) & 0x7));
					ADDF("alpha *= %s;", alphaValueString(2, (hi >>  9) & 0x7));
					ADDF("alpha += %s;", alphaValueString(3, (lo >>  9) & 0x7));
					ADD("FragColor.a = alpha;");
					
					/* alpha cycle 1 */
					ADDF("alpha = %s;", alphaValueString(0, (lo >> 21) & 0x7));
					ADDF("alpha -= %s;", alphaValueString(1, (lo >> 3) & 0x7));
					ADDF("alpha *= %s;", alphaValueString(2, (lo >> 18) & 0x7));
					ADDF("alpha += %s;", alphaValueString(3, (lo >>  0) & 0x7));
					ADD("FragColor.a = alpha;");
					
					/* TODO optimization: only include this bit if texture is sampled */
					ADD("if (alpha == 0.0) discard;");
				} else
					ADD("FragColor.a = 1.0;");
				
				/* TODO optimization: detect when unnecessary and omit */
				/* color cycle 0 */
				ADDF("final = %s;", colorValueString(0, (hi >> 20) & 0xf));
				ADDF("final -= %s;", colorValueString(1, (lo >> 28) & 0xf));
				ADDF("final *= %s;", colorValueString(2, (hi >> 15) & 0x1f));
				ADDF("final += %s;", colorValueString(3, (lo >> 15) & 0x7));
				ADD("FragColor.rgb = final;");
				
				/* color cycle 1 */
				ADDF("final = %s;", colorValueString(0, (hi >> 5) & 0xf));
				ADDF("final -= %s;", colorValueString(1, (lo >> 24) & 0xf));
				ADDF("final *= %s;", colorValueString(2, (hi >> 0) & 0x1f));
				ADDF("final += %s;", colorValueString(3, (lo >> 6) & 0x7));
				ADD("FragColor.rgb = final;");
				
				if (gFogEnabled && gMatState.mixFog)
					ADD("FragColor.rgb = mix(FragColor.rgb, uFogColor, vFog);");
				
				ADD("}");
				
#undef ADD
#undef ADDF
			}
			
			Shader_update(shader, vtx, frag);
		}
		
		// using new shader, so update view-projection matrices
		gShader = shader;
		if (Shader_use(shader)) {
			Shader_setMat4(shader, "view", &gMatrix.view);
			Shader_setMat4(shader, "projection", &gMatrix.projection);
		}
		
		// populate other misc variables
		Shader_setVec4(
			shader,
			"uPrimColor",
			gMatState.prim.r,
			gMatState.prim.g,
			gMatState.prim.b,
			gMatState.prim.alpha
		);
		Shader_setVec4(
			shader,
			"uEnvColor",
			gMatState.env.r,
			gMatState.env.g,
			gMatState.env.b,
			gMatState.env.alpha
		);
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

static float shift_to_multiplier(const int shift) {
	/* how many bits to shift texture coordinates	   *
	 * if in range  1 <= n <= 10, texcoord >>= n		*
	 * if in range 11 <= n <= 15, texcoord <<= (16 - n) */
	if (!shift)
		return 1;
	
	/* right shift; division by 2 per bit */
	if (shift < 11) {
		return 1.0f / pow(2, shift);
	}
	
	/* left shift; multiplication by 2 per bit */
	return pow(2, 16 - shift);
}

#if 0
static inline Vec3f vec3_mul_mat44f(void* v_, void* mat_) {
	Vec3f* v = v_;
	
	struct {
		Vec4f x;
		Vec4f y;
		Vec4f z;
		Vec4f w;
	}* mat = mat_;
	
	return (Vec3f) {
		       .x = v->x * mat->x.x + v->y * mat->y.x + v->z * mat->z.x + 1 * mat->w.x,
		       .y = v->x * mat->x.y + v->y * mat->y.y + v->z * mat->z.y + 1 * mat->w.y,
		       .z = v->x * mat->x.z + v->y * mat->y.z + v->z * mat->z.z + 1 * mat->w.z
	};
}
#endif

static void mtx_multVec3fToVec4f(Vec3f* src, Vec4f* vec, MtxF* mf) {
	vec->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	vec->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	vec->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
	vec->w = mf->ww + (mf->wx * src->x + mf->wy * src->y + mf->wz * src->z);
}

static void mtx_normalReoriantation(Vec3f* src, Vec3f* vec, MtxF* mf) {
	vec->x = (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	vec->y = (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	vec->z = (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
}

static Vec4f vec4f_normalize3(Vec4f vec) {
	Vec4f ret;
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
	
	ret.w = vec.w;
	
	return ret;
}

static Vec3f vec3f_normalize(Vec3f vec) {
	Vec3f ret;
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

static float vec3f_dot(Vec3f a, Vec3f b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static Vec4f vec4f_color(uint8_t color[3]) {
	const float scale = 1 / 255.0f;
	
	return (Vec4f) { color[0] * scale, color[1] * scale, color[2] * scale, 0.0f };
}

static Vec4f light_bind(Vec3f vtxPos, Vec3f vtxNor, int i) {
	Light* light = &sLights.l[i];
	
	if (light->dir.pad1 == 0) {
		LightDir_t* dir = &light->dir;
		Vec3f onrm;
		onrm.x = (float)dir->dir[0] / __INT8_MAX__;
		onrm.y = (float)dir->dir[1] / __INT8_MAX__;
		onrm.z = (float)dir->dir[2] / __INT8_MAX__;
		
		// Directional light
		Vec4f col = vec4f_color(dir->col);
		Vec3f norm = vec3f_normalize(onrm);
		float mod = CLAMP(vec3f_dot(vtxNor, norm), 0.0, 1.0);
		
		col.x *= mod;
		col.y *= mod;
		col.z *= mod;
		
		return col;
	} else {
		// LightPoint_t* point = &light->point;
	}
	
	return (Vec4f) { 0 };
}

static Vec4f light_bind_all(Vec3f vtxPos, Vec3f vtxNor) {
	Vec4f final = {
		sLights.a.l.col[0] / 255.0f,
		sLights.a.l.col[1] / 255.0f,
		sLights.a.l.col[2] / 255.0f,
		1.0f
	};
	
	for (int i = 0; i < sLightNum; i++) {
		Vec4f color = light_bind(vtxPos, vtxNor, i);
		final.x += color.x;
		final.y += color.y;
		final.z += color.z;
	}
	
	final.x = CLAMP(final.x, 0.0f, 1.0f);
	final.y = CLAMP(final.y, 0.0f, 1.0f);
	final.z = CLAMP(final.z, 0.0f, 1.0f);
	
	return final;
}

static bool gbiFunc_vtx(void* cmd) {
	uint8_t* b = cmd;
	
	int numv = (b[1] << 4) | (b[2] >> 4);
	int vbidx = (b[3] >> 1) - numv;
	uint8_t* vaddr = n64_virt2phys(u32r(b + 4));
	
	VtxF* v = gVbuf + vbidx;
	
	if (!gMatState.mtlReady) {
		doMaterial(cmd);
		gMatState.mtlReady = 1;
	}
	
	if (gHideGeometry)
		return false;
	
	while (numv--) {
		const float div_1_255 = (1.0f / 255.0f);
		const float div_1_127 = (1.0f / 127.0f);
		
		v->pos.x = s16r(vaddr + 0);
		v->pos.y = s16r(vaddr + 2);
		v->pos.z = s16r(vaddr + 4);
		
		Vec3f modelPos = { v->pos.x, v->pos.y, v->pos.z };
		
		mtx_multVec3fToVec4f(&modelPos, &v->pos, gMatrix.modelNow);
		
		v->texcoord0.u = s16r(vaddr + 8) * (1.0 / 1024) * (32.0 / gMatState.texWidth);
		v->texcoord0.v = s16r(vaddr + 10) * (1.0 / 1024) * (32.0 / gMatState.texHeight);
		v->texcoord1.u = v->texcoord0.u;
		v->texcoord1.v = v->texcoord0.v;
		
		v->texcoord0.u *= gMatState.tile[0].shiftS_m;
		v->texcoord0.v *= gMatState.tile[0].shiftT_m;
		v->texcoord0.u -= gMatState.tile[0].uls / 128.0f; /* TODO hard-coded why? */
		v->texcoord0.v -= gMatState.tile[0].ult / 128.0f; /* TODO hard-coded why? */
		
		v->texcoord1.u *= gMatState.tile[1].shiftS_m;
		v->texcoord1.v *= gMatState.tile[1].shiftT_m;
		v->texcoord1.u -= gMatState.tile[1].uls / 128.0f; /* TODO hard-coded why? */
		v->texcoord1.v -= gMatState.tile[1].ult / 128.0f; /* TODO hard-coded why? */
		
		if (gVertexColors) {
			v->color.x = u8r(vaddr + 12) * div_1_255;
			v->color.y = u8r(vaddr + 13) * div_1_255;
			v->color.z = u8r(vaddr + 14) * div_1_255;
			v->norm.x = 0;
			v->norm.y = 0;
			v->norm.z = 0;
		} else {
			Vec3f n = {
				s8r(vaddr + 12) * div_1_127,
				s8r(vaddr + 13) * div_1_127,
				s8r(vaddr + 14) * div_1_127,
			};
			Vec3f mn;
			
			mtx_normalReoriantation(&n, &mn, gMatrix.modelNow);
			n = mn;
			
			n = vec3f_normalize(n);
			v->color = light_bind_all(modelPos, n);
			
			v->norm.x = 0;
			v->norm.y = 0;
			v->norm.z = 0;
		}
		v->color.w = u8r(vaddr + 15) * div_1_255;
		
		++v;
		vaddr += 16; /* byte stride */
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVbuf), gVbuf, GL_DYNAMIC_DRAW);
	gIndicesUsed = 0;
	
	return false;
}

static bool gbiFunc_culldl(void* cmd) {
	if (gCullDLenabled == false)
		return false;
	
	uint8_t* b = cmd;
	MtxF* view = &gMatrix.view;
	MtxF* proj = &gMatrix.projection;
	int vfirst = u16r(b + 2) / 2;
	int vlast = u16r(b + 6) / 2;
	int i;
	VtxF* v = gVbuf + vfirst;
	float Top = FLT_MIN;
	float Bottom = FLT_MAX;
	float Left = FLT_MAX;
	float Right = FLT_MIN;
	float Near = FLT_MIN;
	float Far = FLT_MAX;
	Vec4f Eye = { view->xw, view->yw, view->zw, view->ww };
	
	for (i = vfirst; i < vlast; ++i, ++v) {
		// multiply by view/projection matrices
		Vec4f pos = v->pos;
		Vec4f temp;
		Vec4f clipspace;
		Vec4f ndc; // normalized device coordinate space
		
		Top = fmax(Top, pos.y);
		Bottom = fmin(Bottom, pos.y);
		Left = fmin(Left, pos.x);
		Right = fmax(Right, pos.x);
		Near = fmax(Near, pos.z);
		Far = fmin(Far, pos.z);
		
		mtx_multVec3fToVec4f((void*)&pos, &temp, view);
		mtx_multVec3fToVec4f((void*)&temp, &clipspace, proj);
		
		if (clipspace.w == 0)
			continue;
		
		ndc.x = clipspace.x / clipspace.w;
		ndc.y = clipspace.y / clipspace.w;
		ndc.z = clipspace.z / clipspace.w;
		ndc.w = clipspace.w;
		
		// TODO if (depth > fogStart) continue;
		
		// skip any vertex that falls outside NDC space
		// fprintf(stderr, "%f %f %f\n", ndc.x, ndc.y, ndc.z);
		if (ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1)
			continue;
		
		return false;
	}
	
	// eye of camera is contained within bounding box
	if (Left <= Eye.x && Eye.x <= Right
	   && Bottom <= Eye.y && Eye.y <= Top
	   && Near <= Eye.z && Eye.z <= Far
	)
		return false;
	
	return true;
}

static inline void TryDrawTriangleBatch(const uint8_t* b) {
	if ((b[8] != G_TRI1 && b[8] != G_TRI2)
	   || gIndicesUsed + 6 >= ARRAY_COUNT(gIndices)
	) {
		if (sTriangleCallback) {
			for (int i = 0; i < gIndicesUsed; i += 3) {
				VtxF A = gVbuf[gIndices[i + 0]];
				VtxF B = gVbuf[gIndices[i + 1]];
				VtxF C = gVbuf[gIndices[i + 2]];
				n64_triangleCallbackData triData = {
					{
						{ A.pos.x, A.pos.y, A.pos.z },
						{ B.pos.x, B.pos.y, B.pos.z },
						{ C.pos.x, C.pos.y, C.pos.z },
					},
					{
						{ A.norm.x, A.norm.y, A.norm.z },
						{ B.norm.x, B.norm.y, B.norm.z },
						{ C.norm.x, C.norm.y, C.norm.z },
					},
					!!(gMatState.geometrymode & G_CULL_BACK),
					!!(gMatState.geometrymode & G_CULL_FRONT)
				};
				
				sTriangleCallback(sTriangleCallbackUserData, &triData);
			}
		}
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*gIndices) * gIndicesUsed, gIndices, GL_DYNAMIC_DRAW);
		
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
		
		glDrawElements(GL_TRIANGLES, gIndicesUsed, GL_UNSIGNED_BYTE, 0);
		gIndicesUsed = 0;
	}
}

static bool gbiFunc_tri1(void* cmd) {
	uint8_t* b = cmd;
	
	if (gHideGeometry)
		return false;
	
	gIndices[gIndicesUsed++] = b[1] / 2;
	gIndices[gIndicesUsed++] = b[2] / 2;
	gIndices[gIndicesUsed++] = b[3] / 2;
	
	TryDrawTriangleBatch(b);
	
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
	
	TryDrawTriangleBatch(b);
	
	return false;
}

static bool gbiFunc_settimg(void* cmd) {
	uint8_t* b = cmd;
	uint8_t bits = b[1];
	uint16_t hi = u16r(b + 2);
	uint32_t lo = u32r(b + 4);
	void* imgaddr = n64_virt2phys(lo);
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

static bool gbiFunc_extras(void* cmd) {
	uint8_t* b = cmd;
	
	uint32_t clear = u32r(b);
	uint32_t set = u32r(b + 4);
	
	if (clear & GX_OUTLINE)
		gGxOutline = false;
	
	if (set & GX_OUTLINE)
		gGxOutline = true;
	
	if (clear & GX_POLYGONOFFSET) {
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0, 0);
	}
	
	if (clear & GX_WIREFRAME) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
	
	if (set & GX_POLYGONOFFSET) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-2, -2);
	}
	
	if (set & GX_WIREFRAME) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
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

static void MtxToMtxF(Mtx* src, MtxF* dest) {
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

static void MtxFMtxFMult(MtxF* mfA, MtxF* mfB, MtxF* dest) {
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

static bool gbiFunc_mtx(void* cmd) {
	uint8_t* b = cmd;
	uint8_t params = b[3] ^ G_MTX_PUSH;
	uint32_t mtxaddr = u32r(b + 4);
	Mtx* mtx;
	MtxF mtxF;
	
	if (mtxaddr == 0x8012DB20) /* XXX hard-coded gMtxClear */
		memcpy(&mtxF, &sClearMtx, sizeof(sClearMtx));
	else {
		bool wasDirectAddress = gPtrHiSet;
		mtx = n64_virt2phys(mtxaddr);
		
		if (!mtx)
			return false;
		
		Mtx swap = *mtx;
		
		if (wasDirectAddress == false && (mtxaddr & 0xFF000000) != 0x01000000 && (mtxaddr & 0xFF000000) != 0x0D000000) {
			for (int32_t i = 0; i < 0x40 / 2; i++) {
				uint16_t* ss = (uint16_t*)&swap;
				ss[i] = u32r(&ss[i]);
			}
		}
		
		MtxToMtxF(&swap, &mtxF);
	}
	
	/* push matrix on stack */
	if (params & G_MTX_PUSH) {
		gMatrix.modelNow += 1;
		
		assert(gMatrix.modelNow - gMatrix.modelStack < MATRIX_STACK_MAX && "matrix stack overflow");
		
		*gMatrix.modelNow = *(gMatrix.modelNow - 1);
	}
	
	if (params & G_MTX_LOAD) {
		*gMatrix.modelNow = mtxF;
	} else {
		MtxF copy = *gMatrix.modelNow;
		MtxFMtxFMult(&copy, &mtxF, gMatrix.modelNow);
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
	
	n64_draw(n64_virt2phys(lo));
	
	return b[1] != 0;
}

static bool gbiFunc_setptrhi(void* cmd) {
	if (sizeof(uintptr_t) == 8) {
		uint8_t* b = cmd;
		gPtrHi = u32r(b + 4);
		gPtrHi <<= 32;
	} else
		gPtrHi = 0;
	gPtrHiSet = true;
	
	return false;
}

static bool gbiFunc_moveword(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	uint8_t index = b[1];
	uint16_t offset = hi & 0xffff;
	void* data = n64_virt2phys(lo);
	
	switch (index) {
		case G_MW_MATRIX: break; // TODO
		case G_MW_NUMLIGHT: break; // TODO
		case G_MW_CLIP: break; // TODO
		case G_MW_SEGMENT:
			gSegment[offset / 4] = data;
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
	n64_draw(n64_virt2phys(gRdpHalf1));
	
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

/* this LUT emulates the N64's graphics binary interface */
static gbiFunc gGbi[256] = {
	[G_VTX] = gbiFunc_vtx,
	[G_CULLDL] = gbiFunc_culldl,
	[G_TRI1] = gbiFunc_tri1,
	[G_TRI2] = gbiFunc_tri2,
	[G_SETTIMG] = gbiFunc_settimg,
	[G_TEXTURE] = gbiFunc_texture,
	[G_LOADTLUT] = gbiFunc_loadtlut,
	[G_SETTILE] = gbiFunc_settile,
	[G_SETTILESIZE] = gbiFunc_settilesize,
	[G_LOADBLOCK] = gbiFunc_loadblock,
	[G_LOADTILE] = gbiFunc_loadtile,
	[G_RDPPIPESYNC] = gbiFunc_rdppipesync,
	[G_RDPSETOTHERMODE] = gbiFunc_rdpsetothermode,
	[G_SETOTHERMODE_L] = gbiFunc_setothermode_l,
	[G_SETOTHERMODE_H] = gbiFunc_setothermode_h,
	[G_SETPRIMCOLOR] = gbiFunc_setprimcolor,
	[G_SETENVCOLOR] = gbiFunc_setenvcolor,
	[G_SETCOMBINE] = gbiFunc_setcombine,
	[G_GEOMETRYMODE] = gbiFunc_geometrymode,
	[G_MTX] = gbiFunc_mtx,
	[G_POPMTX] = gbiFunc_popmtx,
	[G_DL] = gbiFunc_dl,
	[G_MOVEWORD] = gbiFunc_moveword,
	[G_SETPTRHI] = gbiFunc_setptrhi,
	[G_BRANCH_Z] = gbiFunc_branch_z,
	[G_RDPHALF_1] = gbiFunc_rdphalf_1,
	[G_RDPHALF_2] = gbiFunc_rdphalf_2,
	[G_ENDDL] = gbiFunc_enddl,
	[GX_EXTRAS] = gbiFunc_extras
};

/*
 *
 * public
 *
 */

void n64_set_segment(int seg, void* data) {
	assert(seg < SEGMENT_MAX);
	
	gSegment[seg] = data;
}

void* n64_virt2phys(unsigned int segaddr) {
	uint8_t* b;
	
	if (gPtrHiSet) {
		gPtrHiSet = false;
		gPtrHi |= segaddr;
		
		return (void*)gPtrHi;
	}
	
	if (!segaddr)
		return 0;
	
	assert((segaddr >> 24) < SEGMENT_MAX);
	
	b = gSegment[segaddr >> 24];
	
	if (!b)
		return 0;
	
	return b + (segaddr & 0xffffff);
}

unsigned int n64_phys2virt(void* cmd) {
	uint8_t* b = cmd;
	uint32_t dist = -1;
	int smallest = -1;
	int i;
	
	if (!b)
		return 0;
	
	for (i = 0; i < SEGMENT_MAX; ++i) {
		uint8_t* this = gSegment[i];
		
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

void n64_draw(void* dlist) {
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
		glGenTextures(TEXTURE_CACHE_SIZE, gTexel);
	
	/* set up geometry stuff */
	glBindVertexArray(gVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVbuf), gVbuf, GL_DYNAMIC_DRAW);
	
	/* pos */
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, pos));
	glEnableVertexAttribArray(0);
	
	/* color */
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, color));
	glEnableVertexAttribArray(1);
	
	/* texcoord0 */
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, texcoord0));
	glEnableVertexAttribArray(2);
	
	/* texcoord1 */
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, texcoord1));
	glEnableVertexAttribArray(3);
	
	/* normal */
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, norm));
	glEnableVertexAttribArray(4);
	
	for (cmd = dlist; ; cmd += 8) {
		//fprintf(stderr, "%08x %08x\n", u32r(cmd), u32r(cmd + 4));
		if (gGbi[*cmd] && gGbi[*cmd](cmd))
			break;
	}
}

void n64_draw_buffers(void) {
	gSPEndDisplayList(POLY_OPA_DISP++);
	gSPEndDisplayList(POLY_XLU_DISP++);
	n64_draw(gPolyOpaHead);
	n64_draw(gPolyXluHead);
}

void n64_setMatrix_model(void* data) {
	//memcpy(gMatrix.model, data, sizeof(gMatrix.model));
	gMatrix.modelNow = gMatrix.modelStack;
	memcpy(gMatrix.modelStack, data, sizeof(*gMatrix.modelStack));
}

void n64_setMatrix_view(void* data) {
	memcpy(&gMatrix.view, data, sizeof(gMatrix.view));
}

void n64_setMatrix_projection(void* data) {
	memcpy(&gMatrix.projection, data, sizeof(gMatrix.projection));
}

void n64_set_fog(float fog[2], float color[3]) {
	memcpy(gFog.fog, fog, sizeof(gFog.fog));
	memcpy(gFog.color, color, sizeof(gFog.color));
}

static void n64_clear_lights(void) {
	sLightNum = 0;
}

bool n64_bind_light(Light* lightInfo, Ambient* ambient) {
	bool ret = EXIT_FAILURE;
	
	if (lightInfo && sLightNum < 7) {
		sLights.l[sLightNum++] = *lightInfo;
		ret = EXIT_SUCCESS;
	}
	
	if (ambient) {
		sLights.a = *ambient;
		ret = EXIT_SUCCESS;
	}
	
	return ret;
}

void n64_set_onlyZmode(enum n64_zmode zmode) {
	gOnlyThisZmode = zmode;
}

void n64_set_onlyGeoLayer(enum n64_geoLayer geoLayer) {
	gOnlyThisGeoLayer = geoLayer;
}

void n64_swap(Gfx* g) {
	uint8_t* cmd = (void*)g;
	
	for (;;) {
		*(uint32_t*)(cmd) = u32r(cmd);
		*(uint32_t*)(cmd + 4) = u32r(cmd + 4);
		if (*cmd == G_ENDDL || (*cmd == G_DL && cmd[1]))
			break;
		cmd += 8;
	}
}

/* you'll generally want to run this during scene change; any time
 * heap memory gets freed and more heap memory gets allocated which
 * might potentially occupy the same sets of addresses
 */
void n64_clearCache() {
	ShaderList_cleanup();
	for (int32_t i = 0; i < gTexelCacheCount; i++) {
		gTexelDict[i].data = 0;
		gTexel[i] = 0;
	}
	gTexelCacheCount = 0;
}

void* n64_graph_alloc(uint32_t sz) {
	static uint8_t buf[1024 * 1024 * 8];
	static uint8_t* ptr;
	uint8_t* ret;
	
	if (!ptr || !sz)
		ptr = buf;
	
	ret = ptr;
	ptr += sz;
	
	return ret;
}

void n64_set_triangleCallbackFunc(void* userData, n64_triangleCallbackFunc callback) {
	sTriangleCallbackUserData = userData;
	sTriangleCallback = callback;
}

uintptr_t gStorePointer;

Gfx n64_gbi_gfxhi_ptr(void* ptr) {
	gStorePointer = (uintptr_t)ptr;
	
	return gO_(G_SETPTRHI, 0, (uint64_t)gStorePointer >> 32);
}

Gfx n64_gbi_gfxhi_seg(uint32_t seg) {
	gStorePointer = seg;
	
	return gO_(G_NOOP, 0, 0);
}

bool n64_set_culling(bool state) {
	return gCullDLenabled = state;
}

void n64_reset_buffers(void) {
	gPolyOpaDisp = gPolyOpaHead;
	gPolyXluDisp = gPolyXluHead;
}

void n64_graph_init() {
	n64_clear_lights();
	n64_graph_alloc(GRAPH_INIT);
	for (int i = 0; i <= 0xF; i++)
		gSegment[i] = NULL;
	n64_reset_buffers();
	Shader_use(0);
	n64_set_onlyZmode(ZMODE_ALL);
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
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
				vec4 wow = projection * view * vec4(aPos.xyz, 1.0);
				
				gl_Position = projection * view * aPos;
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
