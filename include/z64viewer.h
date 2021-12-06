#ifndef __Z64VIEWER_H__
#define __Z64VIEWER_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <zroom.h>
#include <n64.h>
#include <bigendian.h>

#include <HermosauhuLib.h>
#include <Input.h>
#include <Matrix.h>
#include <View.h>
#include <Vector.h>

typedef struct {
	Vec2f winScale;
	GLFWwindow* mainWindow;
} AppInfo;

typedef struct GlobalContext {
	AppInfo app;
	View    view;
	Input   input;
} GlobalContext;

#endif
