#include <Viewer.h>

#define    FTOFIX32(x) (long)((x) * (float)0x00010000)
#define    FIX32TOF(x) ((float)(x) * (1.0f / (float)0x00010000))

static MtxF* gMatrixStack;
static MtxF* gCurrentMatrix;
const MtxF gMtxFClear = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

void Matrix_Init() {
	gCurrentMatrix = Lib_Malloc(gCurrentMatrix, 20 * sizeof(MtxF));
	gMatrixStack = gCurrentMatrix;
}

void Matrix_Clear(MtxF* mf) {
	mf->xx = 1.0f;
	mf->yy = 1.0f;
	mf->zz = 1.0f;
	mf->ww = 1.0f;
	mf->yx = 0.0f;
	mf->zx = 0.0f;
	mf->wx = 0.0f;
	mf->xy = 0.0f;
	mf->zy = 0.0f;
	mf->wy = 0.0f;
	mf->xz = 0.0f;
	mf->yz = 0.0f;
	mf->wz = 0.0f;
	mf->xw = 0.0f;
	mf->yw = 0.0f;
	mf->zw = 0.0f;
}

void Matrix_Push(void) {
	Matrix_MtxFCopy(gCurrentMatrix + 1, gCurrentMatrix);
	gCurrentMatrix++;
}

void Matrix_Pop(void) {
	gCurrentMatrix--;
}

void Matrix_Get(MtxF* dest) {
	Matrix_MtxFCopy(dest, gCurrentMatrix);
}

void Matrix_Put(MtxF* src) {
	Matrix_MtxFCopy(gCurrentMatrix, src);
}

void Matrix_Mult(MtxF* mf, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		Matrix_MtxFMtxFMult(cmf, mf, cmf);
	} else {
		Matrix_MtxFCopy(gCurrentMatrix, mf);
	}
}

void Matrix_MultVec3fExt(Vec3f* src, Vec3f* dest, MtxF* mf) {
	dest->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	dest->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	dest->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
}

void Matrix_MultVec3f(Vec3f* src, Vec3f* dest) {
	MtxF* cmf = gCurrentMatrix;
	
	dest->x = cmf->xw + (cmf->xx * src->x + cmf->xy * src->y + cmf->xz * src->z);
	dest->y = cmf->yw + (cmf->yx * src->x + cmf->yy * src->y + cmf->yz * src->z);
	dest->z = cmf->zw + (cmf->zx * src->x + cmf->zy * src->y + cmf->zz * src->z);
}

void Matrix_Transpose(MtxF* mf) {
	f32 temp;
	
	temp = mf->yx;
	mf->yx = mf->xy;
	mf->xy = temp;
	
	temp = mf->zx;
	mf->zx = mf->xz;
	mf->xz = temp;
	
	temp = mf->zy;
	mf->zy = mf->yz;
	mf->yz = temp;
}

void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	f32 tx;
	f32 ty;
	
	if (mode == MTXMODE_APPLY) {
		tx = cmf->xx;
		ty = cmf->xy;
		cmf->xw += tx * x + ty * y + cmf->xz * z;
		tx = cmf->yx;
		ty = cmf->yy;
		cmf->yw += tx * x + ty * y + cmf->yz * z;
		tx = cmf->zx;
		ty = cmf->zy;
		cmf->zw += tx * x + ty * y + cmf->zz * z;
		tx = cmf->wx;
		ty = cmf->wy;
		cmf->ww += tx * x + ty * y + cmf->wz * z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xx = 1.0f;
		cmf->yy = 1.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xw = x;
		cmf->yw = y;
		cmf->zw = z;
	}
}

void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		cmf->xx *= x;
		cmf->yx *= x;
		cmf->zx *= x;
		cmf->xy *= y;
		cmf->yy *= y;
		cmf->zy *= y;
		cmf->xz *= z;
		cmf->yz *= z;
		cmf->zz *= z;
		cmf->wx *= x;
		cmf->wy *= y;
		cmf->wz *= z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->ww = 1.0f;
		cmf->xx = x;
		cmf->yy = y;
		cmf->zz = z;
	}
}

void Matrix_RotateX(f32 x, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (x != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(x);
			cos = cosf(x);
			
			temp1 = cmf->xy;
			temp2 = cmf->xz;
			cmf->xy = temp1 * cos + temp2 * sin;
			cmf->xz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yy;
			temp2 = cmf->yz;
			cmf->yy = temp1 * cos + temp2 * sin;
			cmf->yz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zy;
			temp2 = cmf->zz;
			cmf->zy = temp1 * cos + temp2 * sin;
			cmf->zz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wy;
			temp2 = cmf->wz;
			cmf->wy = temp1 * cos + temp2 * sin;
			cmf->wz = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (x != 0) {
			sin = sinf(x);
			cos = cosf(x);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->xx = 1.0f;
		cmf->ww = 1.0f;
		cmf->yy = cos;
		cmf->zz = cos;
		cmf->zy = sin;
		cmf->yz = -sin;
	}
}

void Matrix_RotateY(f32 y, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (y != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(y);
			cos = cosf(y);
			
			temp1 = cmf->xx;
			temp2 = cmf->xz;
			cmf->xx = temp1 * cos - temp2 * sin;
			cmf->xz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->yx;
			temp2 = cmf->yz;
			cmf->yx = temp1 * cos - temp2 * sin;
			cmf->yz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->zx;
			temp2 = cmf->zz;
			cmf->zx = temp1 * cos - temp2 * sin;
			cmf->zz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->wx;
			temp2 = cmf->wz;
			cmf->wx = temp1 * cos - temp2 * sin;
			cmf->wz = temp1 * sin + temp2 * cos;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (y != 0) {
			sin = sinf(y);
			cos = cosf(y);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->yy = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->zz = cos;
		cmf->zx = -sin;
		cmf->xz = sin;
	}
}

void Matrix_RotateZ(f32 z, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (z != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(z);
			cos = cosf(z);
			
			temp1 = cmf->xx;
			temp2 = cmf->xy;
			cmf->xx = temp1 * cos + temp2 * sin;
			cmf->xy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yx;
			temp2 = cmf->yy;
			cmf->yx = temp1 * cos + temp2 * sin;
			cmf->yy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zx;
			temp2 = cmf->zy;
			cmf->zx = temp1 * cos + temp2 * sin;
			cmf->zy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wx;
			temp2 = cmf->wy;
			cmf->wx = temp1 * cos + temp2 * sin;
			cmf->wy = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (z != 0) {
			sin = sinf(z);
			cos = cosf(z);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->yy = cos;
		cmf->yx = sin;
		cmf->xy = -sin;
	}
}

void Matrix_MtxFCopy(MtxF* dest, MtxF* src) {
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
}

void Matrix_ToMtxF(MtxF* mtx) {
	Matrix_MtxFCopy(mtx, gCurrentMatrix);
}

void Matrix_MtxToMtxF(Mtx* src, MtxF* dest) {
	u16* m1 = (void*)((u8*)src);
	u16* m2 = (void*)((u8*)src + 0x20);
	
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

Mtx* Matrix_MtxFToMtx(MtxF* src, Mtx* dest) {
	s32 temp;
	u16* m1 = (void*)((u8*)dest);
	u16* m2 = (void*)((u8*)dest + 0x20);
	
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
	
	return dest;
}

Mtx* Matrix_ToMtx(Mtx* dest) {
	return Matrix_MtxFToMtx(gCurrentMatrix, dest);
}

void Matrix_MtxFMtxFMult(MtxF* mfA, MtxF* mfB, MtxF* dest) {
	f32 cx;
	f32 cy;
	f32 cz;
	f32 cw;
	//---ROW1---
	f32 rx = mfA->xx;
	f32 ry = mfA->xy;
	f32 rz = mfA->xz;
	f32 rw = mfA->xw;
	
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

void Matrix_Projection(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
	f32 yscale;
	s32 row;
	s32 col;
	
	Matrix_Clear(mtx);
	
	near *= 100;
	far *= 100;
	
	fovy *= M_PI / 180.0;
	yscale = cosf(fovy / 2) / sinf(fovy / 2);
	mtx->mf[0][0] = yscale / aspect;
	mtx->mf[1][1] = yscale;
	mtx->mf[2][2] = (near + far) / (near - far);
	mtx->mf[2][3] = -1;
	mtx->mf[3][2] = 2 * near * far / (near - far);
	mtx->mf[3][3] = 0.0f;
	
	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			mtx->mf[row][col] *= scale;
		}
	}
}

void Matrix_LookAt(MtxF* mf, Vec3f eye, Vec3f at, s16 roll) {
	f32 length;
	Vec3f look;
	Vec3f right;
	Vec3f multUp = { 0.0f, 1.0f, 0.0f };
	Vec3f up;
	VecSph vecSph;
	
	Vec_Vec3fDiffToVecSphGeo(&vecSph, &eye, &at);
	Vec_CalcUpFromPitchYawRoll(&up, vecSph.pitch, vecSph.yaw, roll);
	
	Matrix_Clear(mf);
	
	look.x = at.x - eye.x;
	look.y = at.y - eye.y;
	look.z = at.z - eye.z;
	length = -1.0 / sqrtf(SQ(look.x) + SQ(look.y) + SQ(look.z));
	look.x *= length;
	look.y *= length;
	look.z *= length;
	
	right.x = up.y * look.z - up.z * look.y;
	right.y = up.z * look.x - up.x * look.z;
	right.z = up.x * look.y - up.y * look.x;
	length = 1.0 / sqrtf(SQ(right.x) + SQ(right.y) + SQ(right.z));
	right.x *= length;
	right.y *= length;
	right.z *= length;
	
	up.x = look.y * right.z - look.z * right.y;
	up.y = look.z * right.x - look.x * right.z;
	up.z = look.x * right.y - look.y * right.x;
	length = 1.0 / sqrtf(SQ(up.x) + SQ(up.y) + SQ(up.z));
	up.x *= length;
	up.y *= length;
	up.z *= length;
	
	mf->mf[0][0] = right.x;
	mf->mf[1][0] = right.y;
	mf->mf[2][0] = right.z;
	mf->mf[3][0] = -(eye.x * right.x + eye.y * right.y + eye.z * right.z);
	
	mf->mf[0][1] = up.x;
	mf->mf[1][1] = up.y;
	mf->mf[2][1] = up.z;
	mf->mf[3][1] = -(eye.x * up.x + eye.y * up.y + eye.z * up.z);
	
	mf->mf[0][2] = look.x;
	mf->mf[1][2] = look.y;
	mf->mf[2][2] = look.z;
	mf->mf[3][2] = -(eye.x * look.x + eye.y * look.y + eye.z * look.z);
	
	mf->mf[0][3] = 0;
	mf->mf[1][3] = 0;
	mf->mf[2][3] = 0;
	mf->mf[3][3] = 1;
	
	mf->xx *= 100.0f;
	mf->yx *= 100.0f;
	mf->zx *= 100.0f;
	mf->xy *= 100.0f;
	mf->yy *= 100.0f;
	mf->zy *= 100.0f;
	mf->xz *= 100.0f;
	mf->yz *= 100.0f;
	mf->zz *= 100.0f;
	mf->wx *= 100.0f;
	mf->wy *= 100.0f;
	mf->wz *= 100.0f;
}

void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation) {
	MtxF* cmf = gCurrentMatrix;
	f32 sin = Math_SinS(rotation->z);
	f32 cos = Math_CosS(rotation->z);
	f32 temp1;
	f32 temp2;
	
	temp1 = cmf->xx;
	temp2 = cmf->xy;
	cmf->xw += temp1 * translation->x + temp2 * translation->y + cmf->xz * translation->z;
	cmf->xx = temp1 * cos + temp2 * sin;
	cmf->xy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->yx;
	temp2 = cmf->yy;
	cmf->yw += temp1 * translation->x + temp2 * translation->y + cmf->yz * translation->z;
	cmf->yx = temp1 * cos + temp2 * sin;
	cmf->yy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->zx;
	temp2 = cmf->zy;
	cmf->zw += temp1 * translation->x + temp2 * translation->y + cmf->zz * translation->z;
	cmf->zx = temp1 * cos + temp2 * sin;
	cmf->zy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->wx;
	temp2 = cmf->wy;
	cmf->ww += temp1 * translation->x + temp2 * translation->y + cmf->wz * translation->z;
	cmf->wx = temp1 * cos + temp2 * sin;
	cmf->wy = temp2 * cos - temp1 * sin;
	
	if (rotation->y != 0) {
		sin = Math_SinS(rotation->y);
		cos = Math_CosS(rotation->y);
		
		temp1 = cmf->xx;
		temp2 = cmf->xz;
		cmf->xx = temp1 * cos - temp2 * sin;
		cmf->xz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->yx;
		temp2 = cmf->yz;
		cmf->yx = temp1 * cos - temp2 * sin;
		cmf->yz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->zx;
		temp2 = cmf->zz;
		cmf->zx = temp1 * cos - temp2 * sin;
		cmf->zz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->wx;
		temp2 = cmf->wz;
		cmf->wx = temp1 * cos - temp2 * sin;
		cmf->wz = temp1 * sin + temp2 * cos;
	}
	
	if (rotation->x != 0) {
		sin = Math_SinS(rotation->x);
		cos = Math_CosS(rotation->x);
		
		temp1 = cmf->xy;
		temp2 = cmf->xz;
		cmf->xy = temp1 * cos + temp2 * sin;
		cmf->xz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->yy;
		temp2 = cmf->yz;
		cmf->yy = temp1 * cos + temp2 * sin;
		cmf->yz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->zy;
		temp2 = cmf->zz;
		cmf->zy = temp1 * cos + temp2 * sin;
		cmf->zz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->wy;
		temp2 = cmf->wz;
		cmf->wy = temp1 * cos + temp2 * sin;
		cmf->wz = temp2 * cos - temp1 * sin;
	}
}
