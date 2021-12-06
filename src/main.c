#ifdef Z64VIEWER_WANT_MAIN

#include <z64viewer.h>

static ViewerContext* viewerCtx;

int main(void) {
	viewerCtx = Lib_Malloc(0, sizeof(ViewerContext));
	bzero(viewerCtx, sizeof(ViewerContext));
	z64viewer_init(viewerCtx);
	
	while (!glfwWindowShouldClose(viewerCtx->appInfo.mainWindow)) {
		AppInfo* appInfo = &viewerCtx->appInfo;
		InputContext* inputCtx = &viewerCtx->inputCtx;
		LightContext* lightCtx = &viewerCtx->lightCtx;
		ViewContext* viewCtx = &viewerCtx->viewCtx;
		
		Input_Update(inputCtx, appInfo);
		View_Update(viewCtx, inputCtx, &appInfo->winScale);
		Input_End(inputCtx);
		glfwPollEvents();
		
		glClearColor(
			lightCtx->ambient.r,
			lightCtx->ambient.g,
			lightCtx->ambient.b,
			1.0f
		);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		z64viewer_draw3Dviewport(viewerCtx);
		glfwSwapBuffers(appInfo->mainWindow);
	}
	
	glfwTerminate();
}

#endif
