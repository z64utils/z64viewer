#ifndef __Z64VIEWER_H__
#define __Z64VIEWER_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <zroom.h>
#include <n64.h>
#include <bigendian.h>
#include <assert.h>

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

int z64viewer_shouldWindowClose(void);
void z64viewer_terminate(void);
void z64viewer_scene(void *zscene);
void z64viewer_room(void *zroom);
void z64viewer_update(void);
void z64viewer_draw(void drawFunc(void *udata), void *udata);
s32 z64viewer_init(const char *windowTitle);
void z64viewer_get_windowDimensions(int *w, int *h);

#endif
