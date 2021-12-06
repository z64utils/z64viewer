#ifdef Z64VIEWER_WANT_MAIN

#include <Viewer.h>

static ViewerContext* viewerCtx;

int main(void) {
	viewerCtx = Lib_Malloc(0, sizeof(ViewerContext));
	bzero(viewerCtx, sizeof(ViewerContext));
	Viewer_Init(viewerCtx);
	
	while (!glfwWindowShouldClose(viewerCtx->appInfo.mainWindow)) {
		glfwPollEvents();
		Viewer_Draw(viewerCtx);
	}
	
	glfwTerminate();
}

#endif
