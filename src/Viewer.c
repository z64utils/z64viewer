#include <Viewer.h>

void Viewer_Init(ViewerContext* viewerCtx) {
	View_Init(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
	viewerCtx->viewCtx.cameraControl = true;
	
	MemFile_LoadFile(&viewerCtx->scene.file, "scene.zscene");
	MemFile_LoadFile(&viewerCtx->room[0].file, "room_0.zmap");
	Scene_ExecuteCommands(&viewerCtx->scene, NULL);
}

void Viewer_Update(ViewerContext* viewerCtx) {
	View_SetProjectionDimensions(&viewerCtx->viewCtx, &viewerCtx->appInfo.winDim);
	View_Update(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
}

void Viewer_Draw(ViewerContext* viewerCtx) {
	static Mtx mtx[128];
	
	n64_ClearSegments();
	gSPSegment(0x2, viewerCtx->scene.file.data);
	Light_BindLights(&viewerCtx->scene);
	z64_Draw_Room(&viewerCtx->room[0].file);
}
