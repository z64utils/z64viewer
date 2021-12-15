#include <Viewer.h>

void Viewer_Init(ViewerContext* viewerCtx) {
	View_Init(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
	viewerCtx->viewCtx.cameraControl = true;
}

void Viewer_Update(ViewerContext* viewerCtx) {
	View_SetProjectionDimensions(&viewerCtx->viewCtx, &viewerCtx->appInfo.winDim);
	View_Update(&viewerCtx->viewCtx, &viewerCtx->inputCtx);
}

void Viewer_Draw(ViewerContext* viewerCtx) {
	z64_Draw_SetScene(&viewerCtx->objCtx.scene);
	z64_Draw_Room(&viewerCtx->objCtx.room[0]);
}