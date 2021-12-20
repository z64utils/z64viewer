#ifndef __Z_SCENE_H__
#define __Z_SCENE_H__
#include <Global.h>
#include <Light.h>
#include <z_room.h>
#include <n64.h>

typedef struct Scene {
	LightContext lightCtx;
	MemFile file;
} Scene;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  data1;
	/* 0x04 */ u32 data2;
} SCmdBase;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdSpawnList;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    num;
	/* 0x04 */ void* segment;
} SCmdActorList;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdUnused02;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdColHeader;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    num;
	/* 0x04 */ void* segment;
} SCmdRoomList;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   data1;
	/* 0x02 */ char pad[2];
	/* 0x04 */ u8   x;
	/* 0x05 */ u8   y;
	/* 0x06 */ u8   z;
	/* 0x07 */ u8   unk_07;
} SCmdWindSettings;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdEntranceList;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  cUpElfMsgNum;
	/* 0x04 */ u32 keepObjectId;
} SCmdSpecialFiles;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  gpFlag1;
	/* 0x04 */ u32 gpFlag2;
} SCmdRoomBehavior;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  data1;
	/* 0x04 */ u32 segment;
} SCmdMesh;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  num;
	/* 0x04 */ u32 segment;
} SCmdObjectList;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  num;
	/* 0x04 */ u32 segment;
} SCmdLightList;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  data1;
	/* 0x04 */ u32 segment;
} SCmdPathList;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  num;
	/* 0x04 */ u32 segment;
} SCmdTransiActorList;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  num;
	/* 0x04 */ u32 segment;
} SCmdLightSettingList;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   data1;
	/* 0x02 */ char pad[2];
	/* 0x04 */ u8   hour;
	/* 0x05 */ u8   min;
	/* 0x06 */ u8   unk_06;
} SCmdTimeSettings;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   data1;
	/* 0x02 */ char pad[2];
	/* 0x04 */ u8   skyboxId;
	/* 0x05 */ u8   unk_05;
	/* 0x06 */ u8   unk_06;
} SCmdSkyboxSettings;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   data1;
	/* 0x02 */ char pad[2];
	/* 0x04 */ u8   unk_04;
	/* 0x05 */ u8   unk_05;
} SCmdSkyboxDisables;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  data1;
	/* 0x04 */ u32 data2;
} SCmdEndMarker;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdExitList;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   bgmId;
	/* 0x02 */ char pad[4];
	/* 0x06 */ u8   nightSeqIndex;
	/* 0x07 */ u8   seqIndex;
} SCmdSoundSettings;

typedef struct {
	/* 0x00 */ u8   code;
	/* 0x01 */ u8   data1;
	/* 0x02 */ char pad[5];
	/* 0x07 */ u8   echo;
} SCmdEchoSettings;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdCutsceneData;

typedef struct {
	/* 0x00 */ u8    code;
	/* 0x01 */ u8    data1;
	/* 0x04 */ void* segment;
} SCmdAltHeaders;

typedef struct {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  cameraMovement;
	/* 0x04 */ u32 area;
} SCmdMiscSettings;

typedef struct {
	u8 headerType;
} MeshHeaderBase;

typedef struct {
	MeshHeaderBase base;
	u8  numEntries;
	u32 dListStart;
	u32 dListEnd;
} MeshHeader0;

typedef struct {
	u32 opaDList;
	u32 xluDList;
} MeshEntry0;

typedef struct {
	s16 playerXMax, playerZMax;
	s16 playerXMin, playerZMin;
	u32 opaDList;
	u32 xluDList;
} MeshEntry2;

typedef struct {
	MeshHeaderBase base;
	u8  numEntries;
	u32 dListStart;
	u32 dListEnd;
} MeshHeader2;

typedef struct {
	/* 0x00 */ RGB8 ambientColor;
	/* 0x03 */ RGB8 diffuseDir1;
	/* 0x06 */ RGB8 diffuseColor1;
	/* 0x09 */ RGB8 diffuseDir2;
	/* 0x0C */ RGB8 diffuseColor2;
	/* 0x0F */ RGB8 fogColor;
	/* 0x12 */ u16  fogNear;
	/* 0x14 */ u16  fogFar;
} LightSettings; // size = 0x16

typedef struct {
	/* 0x00 */ u8 count; // number of points in the path
	/* 0x04 */ Vec3s* points; // Segment Address to the array of points
} Path; // size = 0x8

typedef union {
	SCmdBase base;
	SCmdSpawnList        spawnList;
	SCmdActorList        actorList;
	SCmdUnused02         unused02;
	SCmdRoomList         roomList;
	SCmdEntranceList     entranceList;
	SCmdObjectList       objectList;
	SCmdLightList        lightList;
	SCmdPathList         pathList;
	SCmdTransiActorList  transiActorList;
	SCmdLightSettingList lightSettingList;
	SCmdExitList         exitList;
	SCmdColHeader        colHeader;
	SCmdMesh mesh;
	SCmdSpecialFiles     specialFiles;
	SCmdCutsceneData     cutsceneData;
	SCmdRoomBehavior     roomBehavior;
	SCmdWindSettings     windSettings;
	SCmdTimeSettings     timeSettings;
	SCmdSkyboxSettings   skyboxSettings;
	SCmdSkyboxDisables   skyboxDisables;
	SCmdEndMarker        endMarker;
	SCmdSoundSettings    soundSettings;
	SCmdEchoSettings     echoSettings;
	SCmdMiscSettings     miscSettings;
	SCmdAltHeaders       altHeaders;
} SceneCmd; // size = 0x8

typedef void (* SceneCmdFunc)(Scene*,Room*,SceneCmd*);

void Scene_ExecuteCommands(Scene* scene, Room* room);

#endif