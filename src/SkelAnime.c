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
	
	ByteSwap(&animHeader.jointIndices);
	ByteSwap(&animHeader.frameData);
	ByteSwap(&animHeader.common.frameCount);
	ByteSwap(&animHeader.staticIndexMax);
	
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
		ByteSwap(&swapInd.x);
		ByteSwap(&swapInd.y);
		ByteSwap(&swapInd.z);
		frameTable->x =
		    (swapInd.x >= staticIndexMax) ? dynamicData[swapInd.x] : staticData[swapInd.x];
		frameTable->y =
		    (swapInd.y >= staticIndexMax) ? dynamicData[swapInd.y] : staticData[swapInd.y];
		frameTable->z =
		    (swapInd.z >= staticIndexMax) ? dynamicData[swapInd.z] : staticData[swapInd.z];
		
		ByteSwap(&frameTable->x);
		ByteSwap(&frameTable->y);
		ByteSwap(&frameTable->z);
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
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	u32 dlist;
	MtxF mtxF;
	
	limbList = SEGMENTED_TO_VIRTUAL(skelSeg);
	limb = SEGMENTED_TO_VIRTUAL(ReadBE(limbList[limbId]));
	
	Matrix_Push();
	
	if (limbId == 0) {
		Vec3_Copy(&pos, &jointTable[0]);
		Vec3_Copy(&rot, &jointTable[1]);
	} else {
		Vec3_CopyBE(&pos, &limb->jointPos);
		limbId++;
		Vec3_Copy(&rot, &jointTable[limbId]);
	}
	
	Vec3_Copy(&rpos, &pos);
	Vec3_Mult(&rpos, 0.001f);
	
	Matrix_TranslateRotateZYX(&rpos, &rot);
	
	if (limb->dList) {
		Matrix_ToMtxF(&mtxF);
		if (*mtx) {
			Matrix_ToMtx((*mtx)++);
		}
		gSPMatrix(&mtxF);
		gSPDisplayList(ReadBE(limb->dList));
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
	SkeletonHeader* skel;
	
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	Matrix_Push();
	
	gSPSegment(0x6, skelAnime->memFile->data);
	if (mtx)
		gSPSegment(0xD, mtx);
	skel = SEGMENTED_TO_VIRTUAL(skelAnime->skeleton);
	
	SkelAnime_Limb(ReadBE(skel->segment), 0, &mtx, jointTable);
	
	Matrix_Pop();
	
	gS = 1;
}
