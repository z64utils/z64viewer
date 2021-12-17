#include <z64.h>

static u32 gS;

void SkelAnime_Init(MemFile* memFile, SkelAnime* skelAnime, u32 skeleton, u32 animation, Vec3s* jointTable) {
	skelAnime->memFile = memFile;
	
	gSPSegment(0x6, memFile->data);
	SkeletonHeader* skel = SEGMENTED_TO_VIRTUAL(skeleton);
	
	skelAnime->skeleton = skeleton;
	skelAnime->jointTable = jointTable;
	skelAnime->animation = animation;
	skelAnime->limbCount = skel->limbCount + 1;
}

void SkelAnime_Update(SkelAnime* skelAnime) {
	gSPSegment(0x6, skelAnime->memFile->data);
	
	AnimationHeader animHeader = *((AnimationHeader*)SEGMENTED_TO_VIRTUAL(skelAnime->animation));
	
	Lib_ByteSwap(&animHeader.jointIndices, SWAP_U32);
	Lib_ByteSwap(&animHeader.frameData, SWAP_U32);
	Lib_ByteSwap(&animHeader.common.frameCount, SWAP_U16);
	Lib_ByteSwap(&animHeader.staticIndexMax, SWAP_U16);
	
	if (!z64_ExecuteIn20Fps())
		return;
	
	skelAnime->endFrame = animHeader.common.frameCount - 1;
	JointIndex* jointIndices = SEGMENTED_TO_VIRTUAL(animHeader.jointIndices);
	s16* frameData = SEGMENTED_TO_VIRTUAL(animHeader.frameData);
	s16* staticData = &frameData[0];
	s16* dynamicData = &frameData[(s32)skelAnime->curFrame];
	u16 staticIndexMax = animHeader.staticIndexMax;
	Vec3s* frameTable = skelAnime->jointTable;
	s32 i;
	
	for (i = 0; i < skelAnime->limbCount; i++, frameTable++, jointIndices++) {
		Vec3s swapInd = {
			jointIndices->x,
			jointIndices->y,
			jointIndices->z
		};
		Lib_ByteSwap(&swapInd.x, SWAP_U16);
		Lib_ByteSwap(&swapInd.y, SWAP_U16);
		Lib_ByteSwap(&swapInd.z, SWAP_U16);
		frameTable->x =
		    (swapInd.x >= staticIndexMax) ? dynamicData[swapInd.x] : staticData[swapInd.x];
		frameTable->y =
		    (swapInd.y >= staticIndexMax) ? dynamicData[swapInd.y] : staticData[swapInd.y];
		frameTable->z =
		    (swapInd.z >= staticIndexMax) ? dynamicData[swapInd.z] : staticData[swapInd.z];
		
		Lib_ByteSwap(&frameTable->x, SWAP_U16);
		Lib_ByteSwap(&frameTable->y, SWAP_U16);
		Lib_ByteSwap(&frameTable->z, SWAP_U16);
	}
	
	if (skelAnime->curFrame < skelAnime->endFrame) {
		skelAnime->curFrame++;
	} else {
		skelAnime->curFrame = 0;
	}
}

void SkelAnime_Limb(u32 skelSeg, u8 limbId, Mtx** mtx, Vec3s* jointTable) {
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
	
	if (limbId == 0) {
		Vec3_Copy(&pos, &jointTable[0]);
		Vec3_Copy(&rot, &jointTable[1]);
	} else {
		Vec3_Copy(&pos, &limb->jointPos);
		limbId++;
		Vec3_Copy(&rot, &jointTable[limbId]);
		
		Lib_ByteSwap(&pos.x, SWAP_U16);
		Lib_ByteSwap(&pos.y, SWAP_U16);
		Lib_ByteSwap(&pos.z, SWAP_U16);
	}
	
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
		SkelAnime_Limb(skelSeg, limb->child, mtx, jointTable);
	
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(skelSeg, limb->sibling, mtx, jointTable);
}

void SkelAnime_Draw(SkelAnime* skelAnime, Mtx* mtx, Vec3s* jointTable) {
	StandardLimb* limb;
	u32* skel;
	u32 skelSeg;
	
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	Matrix_Push();
	
	gSPSegment(0x6, skelAnime->memFile->data);
	if (mtx)
		gSPSegment(0xD, mtx);
	skel = SEGMENTED_TO_VIRTUAL(skelAnime->skeleton);
	skelSeg = *skel;
	Lib_ByteSwap(&skelSeg, SWAP_U32);
	
	SkelAnime_Limb(skelSeg, 0, &mtx, jointTable);
	
	Matrix_Pop();
	
	gS = 1;
}
