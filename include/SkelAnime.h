#include <Global.h>
#include <Matrix.h>

typedef struct {
	Vec3s jointPos;
	u8    child;
	u8    sibling;
	u32   dList;
} StandardLimb;

typedef struct {
	u32 segment;
	u8  limbCount;
} SkeletonHeader;

typedef struct {
	u16 x;
	u16 y;
	u16 z;
} JointIndex;

typedef struct {
	s16 frameCount;
} AnimationHeaderCommon;
typedef struct {
	AnimationHeaderCommon common;
	u32 frameData;
	u32 jointIndices;
	u16 staticIndexMax;
} AnimationHeader;

typedef struct SkelAnime {
	u8       limbCount;
	u8       mode;
	u8       dListCount;
	s8       taper;
	u32      skeleton;
	u32      animation;
	f32      startFrame;
	f32      endFrame;
	f32      animLength;
	f32      curFrame;
	f32      playSpeed;
	Vec3s*   jointTable;
	Vec3s*   morphTable;
	f32      morphWeight;
	f32      morphRate;
	s32 (* update)();
	s8       initFlags;
	u8       moveFlags;
	s16      prevRot;
	Vec3s    prevTransl;
	Vec3s    baseTransl;
	f32      prevFrame;
	MemFile* memFile;
} SkelAnime;

void SkelAnime_Init(MemFile* memFile, SkelAnime* skelAnime, u32 skeleton, u32 animation, Vec3s* jointTable, Vec3s* morphTable);
void SkelAnime_Update(SkelAnime* skelAnime);
void SkelAnime_Draw(SkelAnime* skelAnime, Mtx* mtx, Vec3s* jointTable);