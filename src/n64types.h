#ifndef N64_TYPES_H
#define N64_TYPES_H

#include "n64.h"
#include <stdint.h>
#include "math.h"
#include "assert.h"
#include "stdlib.h"
#include <stddef.h>

#include <shader.h>

extern bool n64_tick_20fps;

typedef bool (*GbiFunc)(void*);

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

typedef struct {
	float x;
	float y;
	float z;
	float w;
} N64Quat;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void mtx_mul_vec4(N64Vector3* src, N64Vector4* vec, Mtx* mf) {
	vec->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	vec->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	vec->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
	vec->w = mf->ww + (mf->wx * src->x + mf->wy * src->y + mf->wz * src->z);
}

static N64Vector3 mtx_mul_vec3(N64Vector3 vec, Mtx* mf) {
	N64Vector3 src = vec;
	vec.x = mf->xw + (mf->xx * src.x + mf->xy * src.y + mf->xz * src.z);
	vec.y = mf->yw + (mf->yx * src.x + mf->yy * src.y + mf->yz * src.z);
	vec.z = mf->zw + (mf->zx * src.x + mf->zy * src.y + mf->zz * src.z);
	return vec;
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

static float lerp(float v, float min, float max) {
	return min + v * (max - min);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static N64Quat vec3_to_quat(N64Vector3 vec) {
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

static N64Vector3 quat_to_vec3(N64Quat q) {
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

static N64Quat quat_lerp(float t, N64Quat a, N64Quat b) {
	N64Quat result;
	
	if (t <= 0)
		return a;
	else if (t >= 1)
		return b;
	
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma GCC diagnostic pop

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
	N64Quat*    frame_tbl; // N64Quat[limb_num * frame_num]
	N64Vector3* limb_rot; // N64Vector3[limb_num]
	N64Vector3  root_pos;
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

typedef struct N64Object {
	uint16_t obj_id;
	uint8_t  uniq_id;
	uint8_t  mtl_setup_dl_id;
	Anim*    anim;
	Mtx      mtx;
} N64Object;

#endif