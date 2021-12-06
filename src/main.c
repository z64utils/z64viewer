#ifdef Z64VIEWER_WANT_MAIN

#include <Viewer.h>

static ViewerContext* viewerCtx;

int main(void) {
	viewerCtx = Lib_Malloc(0, sizeof(ViewerContext));
	bzero(viewerCtx, sizeof(ViewerContext));
	z64_Init(
		"z64viewer",
		&viewerCtx->appInfo,
		&viewerCtx->inputCtx,
		&viewerCtx->viewCtx,
		&viewerCtx->objCtx,
		&viewerCtx->lightCtx,
		viewerCtx,
		(CallbackFunc)Viewer_Update,
		(CallbackFunc)Viewer_Draw_3DViewport,
		NULL
	);
	
	MemFile_LoadFile(&viewerCtx->objCtx.scene, "scene.zscene");
	MemFile_LoadFile(&viewerCtx->objCtx.room[0], "room_0.zmap");
	
	z64_Update();
	glfwTerminate();
}

#endif
