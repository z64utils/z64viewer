#ifndef __Z_ROOM_H__
#define __Z_ROOM_H__
#include <Global.h>
#include <n64.h>

typedef struct {
	u32 opa;
	u32 xlu;
} PolygonDlist;

typedef struct {
	u8  type;
	u8  num;
	u32 start;
	u32 end;
} Polygon;

typedef struct {
	u8  type;
	u8  num;
	u32 start;
	u32 end;
} PolygonType0;

typedef struct {
	Vec3s pos;
	s16   unk_06;
	u32   opa;
	u32   xlu;
} PolygonDlist2;

typedef struct {
	u8  type;
	u8  num; // number of dlist entries
	u32 start;
	u32 end;
} PolygonType2;

typedef union {
	Polygon polygon;
	PolygonType0 polygon0;
	PolygonType2 polygon2;
} Mesh;

typedef struct {
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
