#include <z64.h>

typedef struct {
	Vec3s jointPos;
	u8    child;
	u8    sibling;
	u32   dList;
} StandardLimb;

void SkelAnime_Limb(u32* limbList, MtxF* mtx) {
	StandardLimb* limb;
	u32 limbListSeg;
	Vec3s rot = { 0 };
	Vec3f pos;
	u32 dlist;
	
	limbListSeg = *limbList;
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
	Matrix_Push();
	Vec3_Copy(&pos, &limb->jointPos);
	
	Matrix_TranslateRotateZYX(&pos, &rot);
	Matrix_ToMtxF(mtx);
	dlist = limb->dList;
	Lib_ByteSwap(&dlist, SWAP_U32);
	if (dlist) {
		gSPMatrix(mtx);
		gSPDisplayList(dlist);
	}
	
	limbList++;
	mtx++;
	
	if (limb->child != 0xFF)
		SkelAnime_Limb(limbList, mtx);
	
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(limbList, mtx);
}

void SkelAnime_Draw(MemFile* zobj, u32 skeleton, MtxF* mtx) {
	StandardLimb* limb;
	u32* limbList;
	u32 limbListSeg;
	u32* skel;
	u32 skelSeg;
	Vec3s rot = { 0 };
	Vec3f pos;
	u32 dlist;
	
	Matrix_Push();
	
	gSPSegment(0x6, zobj->data);
	skel = n64_virt2phys(skeleton);
	skelSeg = *skel;
	Lib_ByteSwap(&skelSeg, SWAP_U32);
	limbList = n64_virt2phys(skelSeg);
	limbListSeg = *limbList;
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
	Vec3_Copy(&pos, &limb->jointPos);
	
	Matrix_TranslateRotateZYX(&pos, &rot);
	Matrix_ToMtxF(mtx);
	dlist = limb->dList;
	Lib_ByteSwap(&dlist, SWAP_U32);
	if (dlist) {
		gSPMatrix(mtx);
		gSPDisplayList(dlist);
	}
	
	limbList++;
	mtx++;
	
	if (limb->child != 0xFF)
		SkelAnime_Limb(limbList, mtx);
	
	Matrix_Pop();
}