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

void z64viewer_update(ViewerContext* viewerCtx);
void z64viewer_init(ViewerContext* viewerCtx);
void z64viewer_draw3Dviewport(ViewerContext* z64Ctx);

#endif
