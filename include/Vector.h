#ifndef __Z64VECTOR_H__
#define __Z64VECTOR_H__

#include <HermosauhuLib.h>

typedef struct {
	f32 x;
	f32 y;
	f32 z;
} Vec3f;

typedef struct {
	f32 x;
	f32 y;
} Vec2f;

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
	f32 x;
	f32 y;
	f32 w;
	f32 h;
} Rectf;

typedef struct {
	s32 x;
	s32 y;
	s32 w;
	s32 h;
} Recti;

s16 Math_Atan2S(f32 x, f32 y);
f32 Vec_DistXZ(Vec3f* a, Vec3f* b);
f32 Vec_DistXYZ(Vec3f* a, Vec3f* b);
s16 Vec_Yaw(Vec3f* a, Vec3f* b);
s16 Vec_Pitch(Vec3f* a, Vec3f* b);
void Vec_VecSphToVec3f(Vec3f* dest, VecSph* sph);
void Vec_AddVecSphToVec3f(Vec3f* dest, VecSph* sph);
f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep);

#define SQ(x)        (x * x)
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