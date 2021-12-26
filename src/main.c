#ifdef Z64VIEWER_WANT_MAIN

#include <Viewer.h>

static ViewerContext* viewerCtx;

int main(void) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_DEBUG);
	viewerCtx = Lib_Malloc(0, sizeof(ViewerContext));
	memset(viewerCtx, 0, sizeof(ViewerContext));
	z64_Init(
		"z64viewer",
		&viewerCtx->appInfo,
		&viewerCtx->inputCtx,
		viewerCtx,
		(CallbackFunc)Viewer_Update,
		(CallbackFunc)Viewer_Draw,
		NULL,
		1400,
		700,
		0
	);
	
	Viewer_Init(viewerCtx);
	z64_Update();
	glfwTerminate();
}

#endif
