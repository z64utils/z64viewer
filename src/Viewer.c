#include <Viewer.h>

void Viewer_Update(ViewerContext* viewerCtx) {
	View_SetProjectionDimensions(&viewerCtx->viewCtx, &viewerCtx->appInfo.winDim);
}

void Viewer_Draw(ViewerContext* viewerCtx) {
	z64_Draw_SetScene(&viewerCtx->objCtx.scene);
	z64_Draw_Room(&viewerCtx->objCtx.room[0]);
}