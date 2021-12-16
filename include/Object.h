#ifndef __Z64OBJECT_H__
#define __Z64OBJECT_H__
#include <HermosauhuLib.h>

typedef struct {
	MemFile scene;
	MemFile room[64];
	MemFile zobj;
} ObjectContext;

#endif