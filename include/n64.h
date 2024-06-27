#ifndef N64_H
#define N64_H

/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#define N64_ATTR_BIG_ENDIAN __attribute__((scalar_storage_order("big-endian")))

#include <stdbool.h>
#define F3DEX_GBI_2
#include "gbi.h"

#undef bool
typedef _Bool bool;

#ifndef N64_TEXTURE_CACHE_SIZE
	#define N64_TEXTURE_CACHE_SIZE 512
#endif

#ifndef N64_OPA_STACK_SIZE
	#define N64_OPA_STACK_SIZE 4096
#endif

#ifndef N64_XLU_STACK_SIZE
	#define N64_XLU_STACK_SIZE 4096
#endif

#ifndef N64_MTX_STACK_SIZE
	#define N64_MTX_STACK_SIZE 16
#endif

#define N64_SEGMENT_MAX 16
#define N64_VBUF_MAX    32

#define N64_CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define N64_ARRAY_COUNT(arr)     (uint32_t)(sizeof(arr) / sizeof(arr[0]))

extern GbiGfx n64_material_setup_dl[71][6];
extern GbiGfx n64_poly_opa_head[N64_OPA_STACK_SIZE];
extern GbiGfx* n64_poly_opa_disp;
extern GbiGfx n64_poly_xlu_head[N64_XLU_STACK_SIZE];
extern GbiGfx* n64_poly_xlu_disp;
extern void* n64_segment[N64_SEGMENT_MAX];

#define POLY_OPA_DISP n64_poly_opa_disp
#define POLY_XLU_DISP n64_poly_xlu_disp

enum N64ZMode {
	N64_ZMODE_OPA    = 0b0001,
	N64_ZMODE_INTER  = 0b0010,
	N64_ZMODE_XLU    = 0b0100,
	N64_ZMODE_DEC    = 0b1000,
	N64_ZMODE_ALL    = 0b1111
};

enum N64GeoLayer {
	N64_GEOLAYER_ALL,
	N64_GEOLAYER_OPAQUE,
	N64_GEOLAYER_OVERLAY
};

typedef struct {
	float x, y, z, w;
} N64Vector4;

typedef struct {
	float x, y, z;
} N64Vector3;

typedef struct {
	N64Vector4 pos;
	struct {
		float u;
		float v;
	} texcoord0, texcoord1;
	N64Vector4 color;
	N64Vector3 norm;
} N64Vtx;

typedef struct {
	const N64Vtx* vtx[3];
	struct {
		bool cullBackface  : 1;
		bool cullFrontface : 1;
	};
	uint32_t setId;
} N64Tri;

typedef bool (*N64CullCallback)(void* u_data, const N64Vtx*, uint32_t num);
typedef void (*N64TriCallback)(void* u_data, const N64Tri*);
typedef struct N64Object N64Object;

void n64_register_skeleton(uint16_t obj_id, uint8_t uniq_id, const void* obj, uint8_t seg_id, uint32_t skel_offset);
void n64_register_dlist(uint16_t obj_id, uint8_t uniq_id, const void* obj, uint8_t seg_id, GbiGfx* dlist, int dlist_num);
void n64_unregister(uint16_t obj_id, uint8_t uniq_id);

N64Object* n64_object_new(uint16_t obj_id, uint8_t uniq_id);
void n64_object_destroy(N64Object*);
void n64_object_set_anim(N64Object*, uint32_t anim, float speed);
void n64_object_set_material(N64Object* self, uint8_t id);
void n64_object_draw(N64Object*);
void n64_object_set_mtx(N64Object*, const void* mtx);

void n64_segment_set(int seg, void* data);
void* n64_segment_get(unsigned int segaddr);
unsigned int n64_segment_ptr_offset(void* cmd);

GbiGfx n64_gbi_gfxhi_ptr(const void*);
GbiGfx n64_gbi_gfxhi_seg(uint32_t);
void n64_set_onlyZmode(enum N64ZMode);
void n64_set_onlyGeoLayer(enum N64GeoLayer);

void n64_mtx_model(void*);
void n64_mtx_view(void*);
void n64_mtx_projection(void*);

void* n64_graph_alloc(uint32_t);
void n64_clear_cache(void);

void n64_draw_dlist(void* dlist);
void n64_update_tick(void);
void n64_buffer_init(void);
void n64_buffer_flush(bool drawDecalsSeparately);
void n64_buffer_clear(void);

bool n64_culling(bool state);
void n64_fog(int near, int far, uint8_t r, uint8_t g, uint8_t b);
bool n64_light_bind_dir(int8_t x, int8_t y, int8_t z, uint8_t r, uint8_t g, uint8_t b);
bool n64_light_bind_point(int16_t x, int16_t y, int16_t z, uint8_t r, uint8_t g, uint8_t b);
void n64_light_set_ambient(uint8_t r, uint8_t g, uint8_t b);

void n64_set_tri_callback(void* userData, N64TriCallback callback);
void n64_set_cull_callback(void* userData, N64CullCallback callback);

#endif
