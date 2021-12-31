#include <z64.h>

static u32 gS;

void SkelAnime_Init(MemFile* memFile, SkelAnime* skelAnime, u32 skeleton, u32 animation, Vec3s* jointTable, Vec3s* morphTable) {
	skelAnime->memFile = memFile;
	
	n64_set_segment(0x6, memFile->data);
	SkeletonHeader* skel = SEGMENTED_TO_VIRTUAL(skeleton);
	
	skelAnime->skeleton = skeleton;
	skelAnime->jointTable = jointTable;
	skelAnime->morphTable = morphTable;
	skelAnime->animation = animation;
	skelAnime->limbCount = skel->limbCount + 1;
}

void SkelAnime_GetFrameData(u32 animation, s32 frame, s32 limbCount, Vec3s* frameTable) {
	AnimationHeader animHeader = *((AnimationHeader*)SEGMENTED_TO_VIRTUAL(animation));
	
	ByteSwap(&animHeader.jointIndices);
	ByteSwap(&animHeader.frameData);
	ByteSwap(&animHeader.common.frameCount);
	ByteSwap(&animHeader.staticIndexMax);
	
	JointIndex* jointIndices = SEGMENTED_TO_VIRTUAL(animHeader.jointIndices);
	s16* frameData = SEGMENTED_TO_VIRTUAL(animHeader.frameData);
	s16* staticData = &frameData[0];
	s16* dynamicData = &frameData[frame];
	u16 staticIndexMax = animHeader.staticIndexMax;
	s32 i;
	
	for (i = 0; i < limbCount; i++, frameTable++, jointIndices++) {
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
}

void SkelAnime_InterpFrameTable(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight) {
	s32 i;
	s16 diff;
	s16 base;
	
	if (weight < 1.0f) {
		for (i = 0; i < limbCount; i++, dst++, start++, target++) {
			base = start->x;
			diff = target->x - base;
			dst->x = (s16)(diff * weight) + base;
			base = start->y;
			diff = target->y - base;
			dst->y = (s16)(diff * weight) + base;
			base = start->z;
			diff = target->z - base;
			dst->z = (s16)(diff * weight) + base;
		}
	} else {
		for (i = 0; i < limbCount; i++, dst++, target++) {
			dst->x = target->x;
			dst->y = target->y;
			dst->z = target->z;
		}
	}
}

void SkelAnime_Update(SkelAnime* skelAnime) {
	n64_set_segment(0x6, skelAnime->memFile->data);
	
	if (!Zelda64_20fpsLimiter())
		return;
	
	AnimationHeader animHeader = *((AnimationHeader*)SEGMENTED_TO_VIRTUAL(skelAnime->animation));
	
	ByteSwap(&animHeader.jointIndices);
	ByteSwap(&animHeader.frameData);
	ByteSwap(&animHeader.common.frameCount);
	ByteSwap(&animHeader.staticIndexMax);
	
	skelAnime->endFrame = animHeader.common.frameCount - 1;
	SkelAnime_GetFrameData(skelAnime->animation, floor(skelAnime->curFrame), skelAnime->limbCount, skelAnime->jointTable);
	SkelAnime_GetFrameData(skelAnime->animation, WrapF(floor(skelAnime->curFrame) + 1, 0, skelAnime->endFrame), skelAnime->limbCount, skelAnime->morphTable);
	
	SkelAnime_InterpFrameTable(
		skelAnime->limbCount,
		skelAnime->jointTable,
		skelAnime->jointTable,
		skelAnime->morphTable,
		fmod(skelAnime->curFrame, 1.0f)
	);
	
	if (skelAnime->curFrame < skelAnime->endFrame) {
		skelAnime->curFrame += skelAnime->playSpeed;
	} else {
		skelAnime->curFrame = 0;
	}
	skelAnime->prevFrame = skelAnime->curFrame;
}

void SkelAnime_Limb(u32 skelSeg, u8 limbId, Mtx** mtx, Vec3s* jointTable) {
	StandardLimb* limb;
	u32* limbList;
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	u32 dlist;
	
	limbList = SEGMENTED_TO_VIRTUAL(skelSeg);
	limb = SEGMENTED_TO_VIRTUAL(ReadBE(limbList[limbId]));
	
	Matrix_Push();
	
	if (limbId == 0) {
		Vec3_Copy(pos, jointTable[0]);
		Vec3_Copy(rot, jointTable[1]);
	} else {
		Vec3_CopyBE(pos, limb->jointPos);
		limbId++;
		Vec3_Copy(rot, jointTable[limbId]);
	}
	
	Vec3_Copy(rpos, pos);
	// Vec3_Mult(&rpos, 0.001f);
	
	Matrix_TranslateRotateZYX(&rpos, &rot);
	
	if (limb->dList) {
		Mtx* thisMtx = n64_graph_alloc(sizeof(*thisMtx));
		Matrix_ToMtx(thisMtx);
		if (*mtx) {
			Matrix_ToMtx((*mtx)++);
		}
		gxSPMatrix(thisMtx);
		gxSPDisplayListSeg(ReadBE(limb->dList));
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
	
	gxSPSegment(0x6, skelAnime->memFile->data);
	if (mtx)
		gxSPSegment(0xD, mtx);
	skel = SEGMENTED_TO_VIRTUAL(skelAnime->skeleton);
	
	SkelAnime_Limb(ReadBE(skel->segment), 0, &mtx, jointTable);
	
	Matrix_Pop();
	
	gS = 1;
}
