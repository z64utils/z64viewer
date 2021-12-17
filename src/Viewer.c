#include <Viewer.h>

void Viewer_Init(ViewerContext* viewerCtx) {
	View_Init(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
	viewerCtx->viewCtx.cameraControl = true;
	MemFile_LoadFile(&viewerCtx->objCtx.zobj, "zobj.zobj");
}

void Viewer_Update(ViewerContext* viewerCtx) {
	View_SetProjectionDimensions(&viewerCtx->viewCtx, &viewerCtx->appInfo.winDim);
	View_Update(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
}

void Viewer_Draw(ViewerContext* viewerCtx) {
	static MtxF mtx[128];
	z64_Draw_SetScene(&viewerCtx->objCtx.scene);
	z64_Draw_Room(&viewerCtx->objCtx.room[0]);
	Matrix_Translate(0, 0, 0, MTXMODE_NEW);
	Matrix_Scale(0.1, 0.1, 0.1, MTXMODE_APPLY);
	SkelAnime_Draw(&viewerCtx->objCtx.zobj, 0x0600E988, mtx);
}
