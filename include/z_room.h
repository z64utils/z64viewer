#ifndef __Z_ROOM_H__
#define __Z_ROOM_H__
#include <Global.h>
#include <n64.h>

typedef struct {
	u32 opa;
	u32 xlu;
} PolygonDlist0;

typedef struct {
	Vec3s pos;
	s16   unk_06;
	u32   opa;
	u32   xlu;
} PolygonDlist2;

typedef struct {
	u8  type;
	u8  num;
	u32 start;
	u32 end;
} Polygon;

typedef union {
	Polygon polygon;
} Mesh;

typedef struct Room {
	s8      num;
	u8      unk_01;
	u8      unk_02;
	u8      unk_03;
	s8      echo;
	u8      showInvisActors;
	Mesh*   mesh;
	u32     segment;
	char    unk_10[0x4];
	MemFile file;
} Room;

void Room_Draw(Room* room);

#endif
