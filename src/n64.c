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
 * - tri1/tri2 caching to reduce glDrawElements invocations
 * - I pulled n64texconv from zztexview, but that uses per-pixel
 *   callbacks to convert textures; a dedicated converter for each
 *   format tuned for speed would be better
 *
 */

#include <n64.h>
#include <n64texconv.h>
#include <bigendian.h>
#include <shader.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Matrix.h>
#include <Vector.h>
#include <HermosauhuLib.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define SEGMENT_MAX 16
#define VBUF_MAX    32

static GLuint gVAO;
static GLuint gVBO;
static GLuint gEBO;
static GLuint gTexel[2];
static GLuint gIndices[6];

static uint32_t gRdpHalf1;
static uint32_t gRdpHalf2;

static Shader *gShader = 0;

static bool gHideGeometry = false;
static bool gVertexColors = false;
static bool gFogEnabled = true;
static bool gForceBl = false;
static bool gCvgXalpha = false;

static enum n64_geoLayer gOnlyThisGeoLayer;
static enum n64_zmode gOnlyThisZmode;
static enum n64_zmode gCurrentZmode;

static struct {
	float model[16];
	float view[16];
	float projection[16];
} gMatrix;

static float gLights[16];

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
	uint32_t othermode_low;
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
		float    lodfrac;
	} prim;
	struct {
		uint32_t hi;
		uint32_t lo;
		float    r;
		float    g;
		float    b;
		float    alpha;
	} env;
	float    lodfrac;
	float    k4;
	float    k5;
	uint32_t geometrymode;
} gMatState; /* material state magic */

/*
 *
 * private
 *
 */
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
	/*struct {
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} pos;*/
	Vec3f pos;
	struct {
		GLfloat u;
		GLfloat v;
	} texcoord0, texcoord1;
	struct {
		GLfloat r;
		GLfloat g;
		GLfloat b;
		GLfloat a;
	} color;
	struct {
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} norm;
} VtxF;

static void* gSegment[SEGMENT_MAX] = { 0 };
typedef void (* gbiFunc)(void* cmd);
static VtxF gVbuf[VBUF_MAX];

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

static void othermode(void) {
	uint32_t lo = gMatState.othermode_low;
	uint32_t indep = (lo & 0b1111111111111000) >> 3;
	
	gCurrentZmode = (indep & 0b0000110000000) >> 7;
	gForceBl = (indep & 0b0100000000000) >> 11;
	gCvgXalpha = (indep & 0b0001000000000) >> 9;
	
	gHideGeometry = false;
	if (gOnlyThisZmode != ZMODE_ALL && gCurrentZmode != gOnlyThisZmode)
		gHideGeometry = true;
	
	if (!gHideGeometry && gOnlyThisGeoLayer != GEOLAYER_ALL)
	{
		int isOverlay = gForceBl || gCvgXalpha;
		switch (gOnlyThisGeoLayer)
		{
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
		    glPolygonOffset(-1, -1);
		    break;
	    default:
		    glDisable(GL_POLYGON_OFFSET_FILL);
		    glPolygonOffset(0, 0);
		    break;
	}
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

static const char* quickstr(const char* fmt, ...) {
	static char buf[256];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	
	return buf;
}

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
	    case 0x03: return quickstr("vec3(%f,%f,%f)", gMatState.prim.r, gMatState.prim.g, gMatState.prim.b);
	    case 0x04: return "shading.rgb";
	    case 0x05: return quickstr("vec3(%f,%f,%f)", gMatState.env.r, gMatState.env.g, gMatState.env.b);
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
			case 0x01: return quickstr("vec3(%f)", gMatState.k4);
			case 0x02: return "vec3(FragColor.a)";
			case 0x03: return "vec3(0.0)";
		    }
	    case 0x08: return "vec3(texture(texture0, TexCoord0).a)";
	    case 0x09: return "vec3(texture(texture1, TexCoord1).a)";
	    case 0x0A: return quickstr("vec3(%f)", gMatState.prim.alpha);
	    case 0x0B: return "vec3(shading.a)";
	    case 0x0C: return quickstr("vec3(%f)", gMatState.env.alpha);
	    case 0x0D: return quickstr("vec3(%f)", gMatState.lodfrac);
	    case 0x0E: return quickstr("vec3(%f)", gMatState.prim.lodfrac);
	    case 0x0F: return quickstr("vec3(%f)", gMatState.k5);
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
			case 0x02: return quickstr("%f", gMatState.lodfrac);
			default: return "FragColor.a";
		    }
	    case 0x01: return "texture(texture0, TexCoord0).a";
	    case 0x02: return "texture(texture1, TexCoord1).a";
	    case 0x03: return quickstr("%f", gMatState.prim.alpha);
	    case 0x04: return "shading.a";
	    case 0x05: return quickstr("%f", gMatState.env.alpha);
	    case 0x06:
		    switch (idx) {
			case 0x02: return quickstr("%f", gMatState.prim.lodfrac);
			default: return "1.0";
		    }
	}
	
	return "0.0";
}

static void doMaterial(void) {
	
	int tile = 0; /* G_TX_RENDERTILE */
	
	/* update texture image associated with each tile */
	for (tile = 0; tile < 2; ++tile) {
		if (!gMatState.tile[tile].doUpdate)
			continue;
		
		glActiveTexture(GL_TEXTURE0 + tile);
		glBindTexture(GL_TEXTURE_2D, gTexel[tile]);
		
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
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
		uint8_t wow[4096 * 4];
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
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	if (gHideGeometry)
		return;
	
#define SHADER_SOURCE(...) "#version 330 core\n" # __VA_ARGS__
	/* TODO track state changes; if no states changed, don't compile new shader */
	if (1) {
		gMatState.prim.r = ((gMatState.prim.lo >> 24) & 0xff) / 255.0f;
		gMatState.prim.g = ((gMatState.prim.lo >> 16) & 0xff) / 255.0f;
		gMatState.prim.b = ((gMatState.prim.lo >> 8) & 0xff) / 255.0f;
		gMatState.prim.alpha = (gMatState.prim.lo & 0xff) / 255.0f;
		
		gMatState.env.r = ((gMatState.env.lo >> 24) & 0xff) / 255.0f;
		gMatState.env.g = ((gMatState.env.lo >> 16) & 0xff) / 255.0f;
		gMatState.env.b = ((gMatState.env.lo >> 8) & 0xff) / 255.0f;
		gMatState.env.alpha = (gMatState.env.lo & 0xff) / 255.0f;
		static Shader* shader = 0;
		const char* vtx = SHADER_SOURCE(
			layout (location = 0) in vec3 aPos;
			layout (location = 1) in vec4 aColor;
			layout (location = 2) in vec2 aTexCoord0;
			layout (location = 3) in vec2 aTexCoord1;
			layout (location = 4) in vec3 aNorm;
			
			out vec4 vColor;
			out vec2 TexCoord0;
			out vec2 TexCoord1;
			out float vFog;
			out vec3 vLightColor;
			
			//uniform mat4 model;
			uniform mat4 view;
			uniform mat4 projection;
			uniform vec2 uFog;
			uniform mat4 uLights;
			
			float fog_linear(const float dist, const float start, const float end) {
			return 1.0 - clamp((end - dist) / (end - start), 0.0, 1.0);
		}
			
			vec3 mul(mat4 a, vec3 b) {
			return (a * vec4(b, 1.0)).xyz;
		}
			
			void main() {
			float fogStart = uFog.x;
			float fogEnd = uFog.y;
			//gl_Position = projection * view * model * vec4(aPos, 1.0);
			gl_Position = projection * view * vec4(aPos, 1.0);
			vColor = aColor;
			TexCoord0 = vec2(aTexCoord0.x, aTexCoord0.y);
			TexCoord1 = vec2(aTexCoord1.x, aTexCoord1.y);
			vFog = fog_linear(length(gl_Position.xyz), fogStart, fogEnd);
			
			/* when lighting is disabled for a vertex, its normal == 0 */
			if (aNorm == vec3(0.0)) {
				vLightColor = vec3(1.0);
			} else {
				vec3 amb = uLights[0].xyz;
				vec3 dif0 = uLights[1].xyz;
				vec3 dif1 = uLights[2].xyz;
				vec3 lightVector0 = normalize(uLights[3].xyz);
				vec3 lightVector1 = normalize(vec3(uLights[0][3], uLights[1][3], uLights[2][3]));
				//vec3 mvNormal = normalize(vec3(mul(model, aNorm)));
				vec3 mvNormal = normalize(aNorm);
				float dif0mod = clamp(dot(mvNormal, lightVector0), 0.0, 1.0);
				float dif1mod = clamp(dot(mvNormal, lightVector1), 0.0, 1.0);
				vLightColor = amb + dif0 * dif0mod + dif1 * dif1mod;
			}
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
			
			if (gCvgXalpha || gForceBl)
			{
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
			}
			else
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
			
			if (gFogEnabled)
				ADD("FragColor.rgb = mix(FragColor.rgb, uFogColor, vFog);");
			
			ADD("}");
			
#undef ADD
#undef ADDF
		}
		
		if (!shader)
			shader = Shader_new();
		
		gShader = shader;
		
		Shader_update(shader, vtx, frag);
		
		// render container
		Shader_use(shader);
		
		// mvp matrix
		//Shader_setMat4(shader, "model", gMatrix.model);
		Shader_setMat4(shader, "view", gMatrix.view);
		Shader_setMat4(shader, "projection", gMatrix.projection);
		Shader_setMat4(shader, "uLights", gLights);
		Shader_setVec3(shader, "uFogColor", gFog.color[0], gFog.color[1], gFog.color[2]);
		Shader_setVec2(shader, "uFog", gFog.fog[0], gFog.fog[1] - gFog.fog[0]);
		Shader_setInt(shader, "texture0", 0);
		Shader_setInt(shader, "texture1", 1);
	}
}

static float shift_to_multiplier(const int shift) {
	/* how many bits to shift texture coordinates       *
	* if in range  1 <= n <= 10, texcoord >>= n        *
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

static inline Vec3f vec3_mul_mat44f(void *v_, void *mat_)
{
	Vec3f *v = v_;
	struct {
		Vec4f x;
		Vec4f y;
		Vec4f z;
		Vec4f w;
	} *mat = mat_;
	return (Vec3f) {
		  .x = v->x * mat->x.x + v->y * mat->y.x + v->z * mat->z.x + 1 * mat->w.x
		, .y = v->x * mat->x.y + v->y * mat->y.y + v->z * mat->z.y + 1 * mat->w.y
		, .z = v->x * mat->x.z + v->y * mat->y.z + v->z * mat->z.z + 1 * mat->w.z
	};
}

static void gbiFunc_vtx(void* cmd) {
	
	uint8_t* b = cmd;
	
	int numv = (b[1] << 4) | (b[2] >> 4);
	int vbidx = (b[3] >> 1) - numv;
	uint8_t* vaddr = n64_virt2phys(u32r(b + 4));
	
	VtxF* v = gVbuf + vbidx;
	
	if (!gMatState.mtlReady) {
		doMaterial();
		gMatState.mtlReady = 1;
	}
	
	if (gHideGeometry)
		return;
	
	while (numv--) {
		float scale = 0.001f;
		const float div_1_255 = (1.0f / 255.0f);
		const float div_1_127 = (1.0f / 127.0f);
		v->pos.x = s16r(vaddr + 0) * scale;
		v->pos.y = s16r(vaddr + 2) * scale;
		v->pos.z = s16r(vaddr + 4) * scale;
		v->pos = vec3_mul_mat44f(&v->pos, gMatrix.model);
		v->texcoord0.u = s16r(vaddr + 8) * (1.0 / 1024) * (32.0 / gMatState.texWidth);
		v->texcoord0.v = s16r(vaddr + 10) * (1.0 / 1024) * (32.0 / gMatState.texHeight);
		v->texcoord1.u = v->texcoord0.u;
		v->texcoord1.v = v->texcoord0.v;
		
		v->texcoord0.u *= gMatState.tile[0].shiftS_m;
		v->texcoord0.v *= gMatState.tile[0].shiftT_m;
		
		v->texcoord1.u *= gMatState.tile[1].shiftS_m;
		v->texcoord1.v *= gMatState.tile[1].shiftT_m;
		
		if (gVertexColors) {
			v->color.r = u8r(vaddr + 12) * div_1_255;
			v->color.g = u8r(vaddr + 13) * div_1_255;
			v->color.b = u8r(vaddr + 14) * div_1_255;
			v->norm.x = 0;
			v->norm.y = 0;
			v->norm.z = 0;
		} else {
			v->color.r = 1;
			v->color.g = 1;
			v->color.b = 1;
			v->norm.x = s8r(vaddr + 12) * div_1_127;
			v->norm.y = s8r(vaddr + 13) * div_1_127;
			v->norm.z = s8r(vaddr + 14) * div_1_127;
		}
		v->color.a = u8r(vaddr + 15) * div_1_255;
		//memcpy(&v->ext, vaddr + 12, 4);
		
		++v;
		vaddr += 16; /* byte stride */
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVbuf), gVbuf, GL_DYNAMIC_DRAW);
}

static void gbiFunc_tri1(void* cmd) {
	uint8_t* b = cmd;
	
	if (gHideGeometry)
		return;
	
	gIndices[0] = b[1] / 2;
	gIndices[1] = b[2] / 2;
	gIndices[2] = b[3] / 2;
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gIndices), gIndices, GL_DYNAMIC_DRAW);
	
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
}

static void gbiFunc_tri2(void* cmd) {
	uint8_t* b = cmd;
	
	if (gHideGeometry)
		return;
	
	gIndices[0] = b[1] / 2;
	gIndices[1] = b[2] / 2;
	gIndices[2] = b[3] / 2;
	
	gIndices[3] = b[5] / 2;
	gIndices[4] = b[6] / 2;
	gIndices[5] = b[7] / 2;
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gIndices), gIndices, GL_DYNAMIC_DRAW);
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void gbiFunc_settimg(void* cmd) {
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
}

static void gbiFunc_texture(void* cmd) {
	uint8_t* b = cmd;
	uint16_t bits = u16r(b + 2);
	int tile = (bits >> 8) & 7;
	int level = (bits >> 11) & 7;
	int on = bits & 0xfe;
	
	if (tile > 1)
		return;
	
	gMatState.tile[tile].on = on;
	
	if (!on)
		return;
	
	gMatState.tile[tile].level = level;
	gMatState.tile[tile].scaleS = u16r(b + 4) * (1.0f / UINT16_MAX);
	gMatState.tile[tile].scaleT = u16r(b + 6) * (1.0f / UINT16_MAX);
}

static void gbiFunc_loadtlut(void* cmd) {
	uint8_t* b = cmd;
	int t = b[4];
	int c = (b[5] << 4) | (b[6] >> 4);
	
	if (!gMatState.timg.imgaddr)
		return;
	
	memcpy(gMatState.pal, gMatState.timg.imgaddr, ((c >> 2) + 1) * sizeof(uint16_t));
}

static void gbiFunc_settilesize(void* cmd) {
	uint8_t* b = cmd;
	int i = b[4];
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	if (i > 1)
		return;
	
	gMatState.tile[i].uls = (hi >> 12) & 0xfff;
	gMatState.tile[i].ult = hi & 0xfff;
	gMatState.tile[i].lrs = (lo >> 12) & 0xfff;
	gMatState.tile[i].lrt = lo & 0xfff;
}

static void gbiFunc_settile(void* cmd) {
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
		return;
	
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
}

static void gbiFunc_loadblock(void* cmd) {
}

static void gbiFunc_loadtile(void* cmd) {
}

static void gbiFunc_rdppipesync(void* cmd) {
	gMatState.mtlReady = 0;
}

static void gbiFunc_setothermode_l(void* cmd) {
	uint8_t* b = cmd;
	
	int shift = b[2];
	int length = b[3];
	uint32_t data = u32r(b + 4);
	
	gMatState.othermode_low = gMatState.othermode_low & ~(((1 << length) - 1) << shift) | data;
	
	othermode();
}

static void gbiFunc_rdpsetothermode(void* cmd) {
	uint8_t* b = cmd;
	
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	othermode();
}

static void gbiFunc_setprimcolor(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.prim.hi = u32r(b);
	gMatState.prim.lo = u32r(b + 4);
}

static void gbiFunc_setenvcolor(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.env.hi = u32r(b);
	gMatState.env.lo = u32r(b + 4);
}

static void gbiFunc_setcombine(void* cmd) {
	uint8_t* b = cmd;
	
	gMatState.setcombine.hi = u32r(b);
	gMatState.setcombine.lo = u32r(b + 4);
}

static void gbiFunc_geometrymode(void* cmd) {
	uint8_t* b = cmd;
	uint32_t clearbits = ~(u32r(b) & 0xffffff);
	uint32_t setbits = u32r(b + 4);
	
	gMatState.geometrymode = (gMatState.geometrymode & ~clearbits) | setbits;
	
	/* vertex colors */
	if (clearbits & G_LIGHTING)
		gVertexColors = 1;
	if (setbits & G_LIGHTING)
		gVertexColors = 0;
	
	/* backface/frontface culling */
	glEnable(GL_CULL_FACE);
	switch (gMatState.geometrymode & (G_CULL_FRONT | G_CULL_BACK))
	{
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
}

static void gbiFunc_mtx(void* cmd) {
#define G_MTX_NOPUSH      0x00
#define G_MTX_PUSH        0x01
#define G_MTX_MUL         0x00
#define G_MTX_LOAD        0x02
#define G_MTX_MODELVIEW   0x00
#define G_MTX_PROJECTION  0x04
	uint8_t* b = cmd;
	uint8_t params = b[3];
	uint32_t mtxaddr = u32r(b + 4);
	Mtx *mtx = n64_virt2phys(mtxaddr);
	MtxF mtxF;
	
	Matrix_MtxToMtxF(mtx, &mtxF);
	
	switch (params)
	{
		case G_MTX_LOAD | G_MTX_PUSH: // TODO hacky; please implement stack
			n64_setMatrix_model(&mtxF);
			break;
	}
}

static void gbiFunc_popmtx(void* cmd) {
	// TODO
}

static void gbiFunc_dl(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	
	n64_draw(n64_virt2phys(lo));
}

static void gbiFunc_branch_z(void* cmd) {
	uint8_t* b = cmd;
	uint32_t hi = u32r(b);
	uint32_t lo = u32r(b + 4);
	int vbidx0 = ((hi >> 12) & 0xfff) / 5;
	int vbidx1 = (hi & 0xfff) / 2;
	
	/* TODO simulate branching; for now, just draw everything */
	n64_draw(n64_virt2phys(gRdpHalf1));
}

static void gbiFunc_rdphalf_1(void* cmd) {
	uint8_t* b = cmd;
	
	gRdpHalf1 = u32r(b + 4);
}

static void gbiFunc_rdphalf_2(void* cmd) {
	uint8_t* b = cmd;
	
	gRdpHalf2 = u32r(b + 4);
}

/* this LUT emulates the N64's graphics binary interface */
static gbiFunc gGbi[256] = {
	[G_VTX] = gbiFunc_vtx,
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
	[G_SETPRIMCOLOR] = gbiFunc_setprimcolor,
	[G_SETENVCOLOR] = gbiFunc_setenvcolor,
	[G_SETCOMBINE] = gbiFunc_setcombine,
	[G_GEOMETRYMODE] = gbiFunc_geometrymode,
	[G_MTX] = gbiFunc_mtx,
	[G_POPMTX] = gbiFunc_popmtx,
	[G_DL] = gbiFunc_dl,
	[G_BRANCH_Z] = gbiFunc_branch_z,
	[G_RDPHALF_1] = gbiFunc_rdphalf_1,
	[G_RDPHALF_2] = gbiFunc_rdphalf_2
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
	
	if (!segaddr)
		return 0;
	
	assert((segaddr >> 24) < SEGMENT_MAX);
	
	b = gSegment[segaddr >> 24];
	
	if (!b)
		return 0;
	
	return b + (segaddr & 0xffffff);
}

unsigned int n64_phys2virt(void *cmd)
{
	uint8_t *b = cmd;
	uint32_t dist = -1;
	int smallest = -1;
	int i;
	
	if (!b)
		return 0;
	
	for (i = 0; i < SEGMENT_MAX; ++i)
	{
		uint8_t *this = gSegment[i];
		
		if (!this)
			continue;
		
		if (b < this)
			continue;
		
		if (b - this < dist)
		{
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
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (!gVAO)
		glGenVertexArrays(1, &gVAO);
	if (!gVBO)
		glGenBuffers(1, &gVBO);
	if (!gEBO)
		glGenBuffers(1, &gEBO);
	
	/* set up texture stuff */
	if (!gTexel[0])
		glGenTextures(2, gTexel);
	
	/* set up geometry stuff */
	glBindVertexArray(gVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVbuf), gVbuf, GL_DYNAMIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gIndices), gIndices, GL_DYNAMIC_DRAW);
	
	/* pos */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxF), (void*)offsetof(VtxF, pos));
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
	
	for (cmd = dlist; *cmd != G_ENDDL; cmd += 8) {
		//fprintf(stderr, "%08x %08x\n", u32r(cmd), u32r(cmd + 4));
		if (gGbi[*cmd]) {
			gGbi[*cmd](cmd);
			
			/* special early exit condition */
			if (*cmd == G_DL && cmd[1])
				break;
		}
	}
}

void n64_setMatrix_model(void* data) {
	memcpy(gMatrix.model, data, sizeof(gMatrix.model));
	//if (gShader)
	//	Shader_setMat4(gShader, "model", gMatrix.model);
}

void n64_setMatrix_view(void* data) {
	memcpy(gMatrix.view, data, sizeof(gMatrix.view));
}

void n64_setMatrix_projection(void* data) {
	memcpy(gMatrix.projection, data, sizeof(gMatrix.projection));
}

void n64_set_fog(float fog[2], float color[3]) {
	memcpy(gFog.fog, fog, sizeof(gFog.fog));
	memcpy(gFog.color, color, sizeof(gFog.color));
}

void n64_set_lights(float lights[16]) {
	memcpy(gLights, lights, sizeof(gLights));
}

void n64_set_onlyZmode(enum n64_zmode zmode) {
	gOnlyThisZmode = zmode;
}

void n64_set_onlyGeoLayer(enum n64_geoLayer geoLayer) {
	gOnlyThisGeoLayer = geoLayer;
}

