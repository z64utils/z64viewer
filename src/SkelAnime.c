#include <z64.h>

typedef struct {
	Vec3s jointPos;
	u8    child;
	u8    sibling;
	u32   dList;
} StandardLimb;

static u32 gS;

void SkelAnime_Limb(u32 skelSeg, u8 limbId, Mtx** mtx) {
	StandardLimb* limb;
	u32* limbList;
	u32 limbListSeg;
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	u32 dlist;
	MtxF mtxF;
	
	limbList = SEGMENTED_TO_VIRTUAL(skelSeg);
	limbListSeg = *limbList;
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = SEGMENTED_TO_VIRTUAL(limbListSeg);
	
	limbListSeg = limbList[limbId];
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = SEGMENTED_TO_VIRTUAL(limbListSeg);
	
	Matrix_Push();
	Vec3_Copy(&pos, &limb->jointPos);
	
	Lib_ByteSwap(&pos.x, SWAP_U16);
	Lib_ByteSwap(&pos.y, SWAP_U16);
	Lib_ByteSwap(&pos.z, SWAP_U16);
	Vec3_Copy(&rpos, &pos);
	Vec3_Mult(&rpos, 0.001f);
	if (gS == 0)
		OsPrintfEx("%f %f %f", rpos.x, rpos.y, rpos.z);
	
	Matrix_TranslateRotateZYX(&rpos, &rot);
	dlist = limb->dList;
	Lib_ByteSwap(&dlist, SWAP_U32);
	
	if (dlist) {
		Matrix_ToMtxF(&mtxF);
		if (*mtx) {
			Matrix_ToMtx((*mtx)++);
			if (gS == 0)
				OsPrintf("mtx");
		}
		gSPMatrix(&mtxF);
		gSPDisplayList(dlist);
		if (gS == 0)
			OsPrintf("dl");
	}
	
	limbList++;
	
	if (limb->child != 0xFF)
		SkelAnime_Limb(skelSeg, limb->child, mtx);
	
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(skelSeg, limb->sibling, mtx);
}

void SkelAnime_Draw(MemFile* zobj, u32 skeleton, Mtx* mtx) {
	StandardLimb* limb;
	u32* skel;
	u32 skelSeg;
	
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	Matrix_Push();
	
	gSPSegment(0x6, zobj->data);
	if (mtx)
		gSPSegment(0xD, mtx);
	skel = SEGMENTED_TO_VIRTUAL(skeleton);
	skelSeg = *skel;
	Lib_ByteSwap(&skelSeg, SWAP_U32);
	
	SkelAnime_Limb(skelSeg, 0, &mtx);
	
	Matrix_Pop();
	
	gS = 1;
}
