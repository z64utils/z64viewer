#ifndef __Z64GLOBAL_H__
#define __Z64GLOBAL_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <HermosauhuLib.h>
#include <Vector.h>

struct ViewerContext;

typedef void (* CallDraw)(void*);

typedef struct {
	Vec2f       winScale;
	Vec2f       viewportScale;
	GLFWwindow* mainWindow;
	void*       mainCtx;
	CallDraw    drawCall;
} AppInfo;

extern f64 gDeltaTime;

#endif