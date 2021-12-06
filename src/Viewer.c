#include <Viewer.h>

void Viewer_Update(ViewerContext* viewerCtx) {
	SubScreens* subsceens = &viewerCtx->appInfo.subscreen;
	
	subsceens->view3D.dim.x = viewerCtx->appInfo.winDim.x;
	subsceens->view3D.dim.y = viewerCtx->appInfo.winDim.y;
}

void Viewer_Draw_3DViewport(ViewerContext* viewerCtx) {
	z64_Draw_SetScene(&viewerCtx->objCtx.scene);
	z64_Draw_Room(&viewerCtx->objCtx.room[0]);
}