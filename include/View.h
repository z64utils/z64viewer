#ifndef __Z64VIEW_H__
#define __Z64VIEW_H__
#include <Global.h>
#include <HermosauhuLib.h>
#include <Vector.h>
#include <Matrix.h>
#include <Input.h>

typedef struct {
	Vec3f eye;
	Vec3f at;
	s16   roll;
} Camera;

typedef struct {
	u8 smoothZoom : 1;
} CamSettings;

typedef struct {
	f32     fovy;
	f32     near;
	f32     far;
	f32     scale;
	f32     aspect;
	MtxF*   mtxView;
	MtxF*   mtxProj;
	CamSettings settings;
	Camera* currentCamera;
	Camera  camera[4];
} ViewContext;

void View_Init(ViewContext* view, InputContext* input, AppInfo* appInfo);
void View_Update(ViewContext* view, InputContext* input, Vec2f* winDim);

void View_FramebufferCallback(GLFWwindow* window, s32 width, s32 height);

#endif