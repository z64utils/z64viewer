#include <z64.h>

static Mtx n64stack[256];

typedef struct {
	Vec3s jointPos;
	u8    child;
	u8    sibling;
	u32   dList;
} StandardLimb;

static u32 gS;

void SkelAnime_Limb(u32 skelSeg, u8 limbId, MtxF* mtx) {
	StandardLimb* limb;
	u32* limbList;
	u32 limbListSeg;
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	u32 dlist;
	
	limbList = n64_virt2phys(skelSeg);
	limbListSeg = *limbList;
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
	limbListSeg = limbList[limbId];
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
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
		SkelAnime_Limb(skelSeg, limb->child, mtx);
	
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(skelSeg, limb->sibling, mtx);
}

void SkelAnime_Bake(u32 skelSeg, u8 limbId, MtxF* mtx) {
	static int m;
	StandardLimb* limb;
	u32* limbList;
	u32 limbListSeg;
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	u32 dlist;
	
	if (!limbId)
		m = 0;
	
	limbList = n64_virt2phys(skelSeg);
	limbListSeg = *limbList;
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
	limbListSeg = limbList[limbId];
	Lib_ByteSwap(&limbListSeg, SWAP_U32);
	limb = n64_virt2phys(limbListSeg);
	
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
	Matrix_ToMtxF(mtx);
	dlist = limb->dList;
	
	if (dlist) {
		Matrix_ToMtx(&n64stack[m], __FILE__, __LINE__);
		++m;
	}
	
	limbList++;
	mtx++;
	
	if (limb->child != 0xFF)
		SkelAnime_Bake(skelSeg, limb->child, mtx);
	
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Bake(skelSeg, limb->sibling, mtx);
}

void SkelAnime_Draw(MemFile* zobj, u32 skeleton, MtxF* mtx) {
	StandardLimb* limb;
	u32* skel;
	u32 skelSeg;
	
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	Matrix_Push();
	
	gSPSegment(0x6, zobj->data);
	gSPSegment(0xD, n64stack);
	skel = n64_virt2phys(skeleton);
	skelSeg = *skel;
	Lib_ByteSwap(&skelSeg, SWAP_U32);
	
	SkelAnime_Bake(skelSeg, 0, mtx); // TODO flex only
	SkelAnime_Limb(skelSeg, 0, mtx);
	
	Matrix_Pop();
	
	gS = 1;
}
