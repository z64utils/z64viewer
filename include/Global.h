#ifndef __Z64GLOBAL_H__
#define __Z64GLOBAL_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <HermosauhuLib.h>
#include <Vector.h>

struct ViewerContext;

typedef void (* CallDraw)(void*);

typedef struct {
	PosDim view3D;
	PosDim sidePanel;
} SubScreens;

typedef struct {
	SubScreens  subscreen;
	GLFWwindow* mainWindow;
	CallDraw    drawCall;
	void* mainCtx;
	Vec2f winDim;
	Vec2f prevWinDim;
	bool  isCallback;
} AppInfo;

extern f64 gDeltaTime;

#endif