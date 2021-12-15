#ifdef Z64VIEWER_WANT_MAIN

#include <Viewer.h>

static ViewerContext* viewerCtx;

int main(void) {
	viewerCtx = Lib_Malloc(0, sizeof(ViewerContext));
	memset(viewerCtx, 0, sizeof(ViewerContext));
	z64_Init(
		"z64viewer",
		&viewerCtx->appInfo,
		&viewerCtx->inputCtx,
		&viewerCtx->objCtx,
		&viewerCtx->lightCtx,
		viewerCtx,
		(CallbackFunc)Viewer_Update,
		(CallbackFunc)Viewer_Draw,
		1400,
		700
	);
	
	MemFile_LoadFile(&viewerCtx->objCtx.scene, "scene.zscene");
	MemFile_LoadFile(&viewerCtx->objCtx.room[0], "room_0.zmap");
	Viewer_Init(viewerCtx);
	z64_Update();
	glfwTerminate();
}

#endif
