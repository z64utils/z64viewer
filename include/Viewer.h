#ifndef __Z64VIEWER_H__
#define __Z64VIEWER_H__
#include <_global.h>

typedef struct ViewerContext {
	AppInfo appInfo;
	ViewContext   viewCtx;
	InputContext  inputCtx;
	LightContext  lightCtx;
	ObjectContext objCtx;
} ViewerContext;

void Viewer_Update(ViewerContext* viewerCtx);
void Viewer_Init(ViewerContext* viewerCtx);
void Viewer_Draw_3DViewport(ViewerContext* viewerCtx);
void Viewer_Draw(ViewerContext* viewerCtx);

#endif
