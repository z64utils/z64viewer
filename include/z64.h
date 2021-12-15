#ifndef __GET_INC_H__
#define __GET_INC_H__

#include <zroom.h>
#include <n64.h>
#include <bigendian.h>
#include <assert.h>

#include <HermosauhuLib.h>
#include <Input.h>
#include <Matrix.h>
#include <View.h>
#include <Vector.h>
#include <Light.h>
#include <Object.h>

extern InputContext* __inputCtx;
extern AppInfo* __appInfo;

void z64_FramebufferCallback(GLFWwindow* window, s32 width, s32 height);

void z64_Init(
	const char* title,
	AppInfo* appInfo,
	InputContext* inputCtx,
	ObjectContext* objCtx,
	LightContext* lightCtx,
	void* context,
	CallbackFunc updateCall,
	CallbackFunc drawCall
);
void z64_Draw();
void z64_Update();

void z64_Draw_SetScene(MemFile* zscene);
void z64_Draw_Room(MemFile* zroom);

#endif
