#ifndef __Z64VECTOR_H__
#define __Z64VECTOR_H__

#include <ExtLib.h>

typedef struct {
	f32 x;
	f32 y;
	f32 z;
	f32 w;
} Vec4f;

typedef struct {
	union {
		struct {
			f32 x;
			f32 y;
			f32 z;
		};
		f32 s[3];
	};
} Vec3f;

typedef struct {
	f32 x;
	f32 y;
} Vec2f;

typedef struct {
	union {
		struct {
			f64 x;
			f64 y;
		};
		f64 s[2];
	};
} Vec2d;

typedef struct {
	s16 x;
	s16 y;
	s16 z;
} Vec3s;

typedef struct {
	s16 x;
	s16 y;
} Vec2s;

typedef struct {
	s8 x;
	s8 y;
	s8 z;
} Vec3c;

typedef struct {
	s8 x;
	s8 y;
} Vec2c;

typedef struct {
	s32 x;
	s32 y;
	s32 z;
} Vec3i;

typedef struct {
	s32 x;
	s32 y;
} Vec2i;

typedef struct {
	f32 r;
	s16 pitch;
	s16 yaw;
} VecSph;

typedef struct {
	f64 x;
	f64 y;
	f64 w;
	f64 h;
} Rectf32;

typedef struct {
	s32 x;
	s32 y;
	s32 w;
	s32 h;
} Rect;

// CoordinatesRect
typedef struct {
	s32 x1;
	s32 y1;
	s32 x2;
	s32 y2;
} CRect;

s16 Math_Atan2S(f32 x, f32 y);
f32 Vec_Vec3f_DistXZ(Vec3f* a, Vec3f* b);
f32 Vec_Vec3f_DistXYZ(Vec3f* a, Vec3f* b);
f32 Vec_Vec3s_DistXZ(Vec3s* a, Vec3s* b);
f32 Vec_Vec3s_DistXYZ(Vec3s* a, Vec3s* b);
f32 Vec_Vec2f_DistXZ(Vec2f* a, Vec2f* b);
f32 Vec_Vec2s_DistXZ(Vec2s* a, Vec2s* b);
s16 Vec_Yaw(Vec3f* a, Vec3f* b);
s16 Vec_Pitch(Vec3f* a, Vec3f* b);
void Vec_VecSphToVec3f(Vec3f* dest, VecSph* sph);
void Vec_AddVecSphToVec3f(Vec3f* dest, VecSph* sph);
VecSph* Vec_Vec3fToVecSph(VecSph* dest, Vec3f* vec);
VecSph* Vec_Vec3fToVecSphGeo(VecSph* dest, Vec3f* vec);
VecSph* Vec_Vec3fDiffToVecSphGeo(VecSph* dest, Vec3f* a, Vec3f* b);
Vec3f* Vec_CalcUpFromPitchYawRoll(Vec3f* dest, s16 pitch, s16 yaw, s16 roll);
f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep);

// void Vec_Substract(Vec3f* dest, Vec3f* a, Vec3f* b);
void Vec_Add(Vec3f* dest, Vec3f* a, Vec3f* b);
void Vec_Multiply(Vec3f* dest, Vec3f* a, Vec3f* b);
void Vec_Divide(Vec3f* dest, Vec3f* a, Vec3f* b);
void Vec_Vec2f_Substract(Vec2f* dest, Vec2f* a, Vec2f* b);
void Vec_Vec2f_Add(Vec2f* dest, Vec2f* a, Vec2f* b);
void Vec_Vec2f_Multiply(Vec2f* dest, Vec2f* a, Vec2f* b);
void Vec_Vec2f_Divide(Vec2f* dest, Vec2f* a, Vec2f* b);
void Vec_Vec2s_Substract(Vec2s* dest, Vec2s* a, Vec2s* b);
void Vec_Vec2s_Add(Vec2s* dest, Vec2s* a, Vec2s* b);
void Vec_Vec2s_Multiply(Vec2s* dest, Vec2s* a, Vec2s* b);
void Vec_Vec2s_Divide(Vec2s* dest, Vec2s* a, Vec2s* b);

void Rect_ToCRect(CRect* dst, Rect* src);
void Rect_ToRect(Rect* dst, CRect* src);
bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos);
void Rect_Translate(Rect* rect, s32 x, s32 y);
void Rect_Verify(Rect* rect);
void Rect_Set(Rect* dest, s32 x, s32 w, s32 y, s32 h);

#define Vec2_Substract(dest, a, b) \
	dest.x = a.x - b.x; \
	dest.y = a.y - b.y

#define Vec3_Substract(dest, a, b) \
	dest.x = a.x - b.x; \
	dest.y = a.y - b.y; \
	dest.z = a.z - b.z

#define Vec2_Add(dest, a, b) \
	dest.x = a.x + b.x; \
	dest.y = a.y + b.y

#define Vec3_Add(dest, a, b) \
	dest.x = a.x + b.x; \
	dest.y = a.y + b.y; \
	dest.z = a.z + b.z

#define Vec2_Equal(a, b) ( \
		a.x == b.x && \
		a.y == b.y \
)

#define Vec3_Equal(a, b) ( \
		a.x == b.x && \
		a.y == b.y && \
		a.z == b.z \
)

#define Vec2_Copy(dest, src) \
	dest.x = src.x; \
	dest.y = src.y; \
	dest.z = src.z

#define Vec3_Copy(dest, src) \
	dest.x = src.x; \
	dest.y = src.y; \
	dest.z = src.z

#define Vec3_CopyBE(dest, src) \
	dest.x = ReadBE(src.x); \
	dest.y = ReadBE(src.y); \
	dest.z = ReadBE(src.z)

#define Vec2_MultVec(dest, src) \
	dest.x *= src.x; \
	dest.y *= src.y; \
	dest.z *= src.z

#define Vec3_MultVec(dest, src) \
	dest.x *= src.x; \
	dest.y *= src.y; \
	dest.z *= src.z

#define Vec2_Mult(dest, src) \
	dest.x *= src; \
	dest.y *= src;

#define Vec3_Mult(dest, src) \
	dest.x *= src; \
	dest.y *= src; \
	dest.z *= src

#define Vec4_Mult(dest, src) \
	dest.x *= src; \
	dest.y *= src; \
	dest.z *= src; \
	dest.w *= src

#define Vec2_Dot(a, b) ({ \
		(a.x * b.x) + \
		(a.y * b.y); \
	})

#define Vec3_Dot(a, b) ({ \
		(a.x * b.x) + \
		(a.y * b.y) + \
		(a.z * b.z); \
	})

#define Vec3_Cross(a, b) ({ \
		(typeof(*(a))) { \
			a.y* b.z - b.y* a.z, \
			a.z* b.x - b.z* a.x, \
			a.x* b.y - b.x* a.y \
		}; \
	})

#define Vec2_Magnitude(a) ({ \
		sqrtf( \
			(a.x * a.x) + \
			(a.y * a.y) \
		); \
	})

#define Vec3_Magnitude(a) ({ \
		sqrtf( \
			(a.x * a.x) + \
			(a.y * a.y) + \
			(a.z * a.z) \
		); \
	})

#define Vec2_Normalize(a) ({ \
		typeof(a) ret; \
		f32 mgn = Vec2_Magnitude(a); \
		if (mgn == 0) { \
			ret.x = ret.y = 0; \
		} else { \
			ret.x = a.x / mgn; \
			ret.y = a.y / mgn; \
		} \
		ret; \
	})

#define Vec3_Normalize(a) ({ \
		typeof(a) ret; \
		f32 mgn = Vec3_Magnitude(a); \
		if (mgn == 0) { \
			ret.x = ret.y = ret.z = 0; \
		} else { \
			ret.x = a.x / mgn; \
			ret.y = a.y / mgn; \
			ret.z = a.z / mgn; \
		} \
		ret; \
	})

#define SQ(x)        ((x) * (x))
#define Math_SinS(x) sinf(BinToRad((s16)(x)))
#define Math_CosS(x) cosf(BinToRad((s16)(x)))
#define Math_SinF(x) sinf(DegToRad(x))
#define Math_CosF(x) cosf(DegToRad(x))
#define Math_SinR(x) sinf(x)
#define Math_CosR(x) cosf(x)

// Trig macros
#define DegToBin(degreesf) (s16)(degreesf * 182.04167f + .5f)
#define RadToBin(radf)     (s16)(radf * (32768.0f / M_PI))
#define RadToDeg(radf)     (radf * (180.0f / M_PI))
#define DegToRad(degf)     (degf * (M_PI / 180.0f))
#define BinFlip(angle)     ((s16)(angle - 0x7FFF))
#define BinSub(a, b)       ((s16)(a - b))
#define BinToDeg(binang)   ((f32)binang * (360.0001525f / 65535.0f))
#define BinToRad(binang)   (((f32)binang / 32768.0f) * M_PI)

#endif
