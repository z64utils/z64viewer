#ifndef __Z64GLOBAL_H__
#define __Z64GLOBAL_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ExtLib.h>
#include <Vector.h>

struct ViewerContext;
struct Scene;

typedef void (* CallbackFunc)(void*);

typedef struct {
	GLFWwindow*  mainWindow;
	CallbackFunc updateCall;
	CallbackFunc drawCall;
	void* context;
	Vec2s winDim;
	Vec2s prevWinDim;
	bool  isResizeCallback;
} AppInfo;

extern f64 gDeltaTime;

#endif