#ifndef __Z64MATRIX_H__
#define __Z64MATRIX_H__

#include <ExtLib.h>
#include <Vector.h>

typedef enum {
	MTXMODE_NEW,
	MTXMODE_APPLY
} MtxMode;

typedef s32 Mtx_t[4][4];
typedef union {
	Mtx_t m;
	struct {
		u16 intPart[4][4];
		u16 fracPart[4][4];
	};
} Mtx;

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

extern MtxF* gMatrixStack;
extern MtxF* gCurrentMatrix;
extern const MtxF gMtxFClear;

void Matrix_Init();
void Matrix_Clear(MtxF* mf);
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Get(MtxF* dest);
void Matrix_Put(MtxF* src);
void Matrix_MultVec3f(Vec3f* src, Vec3f* dest);
void Matrix_Transpose(MtxF* mf);
void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_RotateX(f32 x, MtxMode mode);
void Matrix_RotateY(f32 y, MtxMode mode);
void Matrix_RotateZ(f32 z, MtxMode mode);

void Matrix_MultVec3fExt(Vec3f* src, Vec3f* dest, MtxF* mf);
void Matrix_MultVec4fExt(Vec4f* src, Vec4f* dest, MtxF* mf);
void Matrix_OrientVec3f(Vec3f* src, Vec3f* dest, MtxF* mf);
void Matrix_Mult(MtxF* mf, MtxMode mode);
void Matrix_MtxFCopy(MtxF* dest, MtxF* src);
void Matrix_ToMtxF(MtxF* mtx);
Mtx* Matrix_ToMtx(Mtx* dest);
Mtx* Matrix_NewMtx();
void Matrix_MtxToMtxF(Mtx* src, MtxF* dest);
Mtx* Matrix_MtxFToMtx(MtxF* src, Mtx* dest);
void Matrix_MtxFMtxFMult(MtxF* mfA, MtxF* mfB, MtxF* dest);
void Matrix_Projection(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale);
void Matrix_LookAt(MtxF* mf, Vec3f eye, Vec3f at, s16 roll);

void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation);

#define Matrix_RotateX_s(x, mode) Matrix_RotateX(BinToRad(x), mode)
#define Matrix_RotateY_s(x, mode) Matrix_RotateY(BinToRad(x), mode)
#define Matrix_RotateZ_s(x, mode) Matrix_RotateZ(BinToRad(x), mode)

#endif
