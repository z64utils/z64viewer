#include <Viewer.h>

void Viewer_Update(ViewerContext* viewerCtx) {
	// input
	Input_Update(&viewerCtx->inputCtx, &viewerCtx->appInfo);
	View_Update(&viewerCtx->viewCtx, &viewerCtx->inputCtx, &viewerCtx->appInfo.winScale);
	
	Input_End(&viewerCtx->inputCtx);
	glfwPollEvents();
}

void Viewer_Init(ViewerContext* viewerCtx) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_DEBUG);
	MemFile_LoadFile(&viewerCtx->objCtx.scene, "scene.zscene");
	MemFile_LoadFile(&viewerCtx->objCtx.room[0], "room_0.zmap");
	
	viewerCtx->appInfo.mainCtx = viewerCtx;
	viewerCtx->appInfo.drawCall = (CallDraw)Viewer_Draw;
	viewerCtx->appInfo.winScale.x = 1400;
	viewerCtx->appInfo.winScale.y = 700;
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	
	viewerCtx->appInfo.mainWindow = glfwCreateWindow(
		viewerCtx->appInfo.winScale.x,
		viewerCtx->appInfo.winScale.y,
		"z64viewer",
		NULL,
		NULL
	);
	if (viewerCtx->appInfo.mainWindow == NULL) {
		printf_error("Failed to create GLFW window.");
	}
	glfwMakeContextCurrent(viewerCtx->appInfo.mainWindow);
	
	glfwSetFramebufferSizeCallback(viewerCtx->appInfo.mainWindow, View_FramebufferCallback);
	glfwSetCursorPosCallback(viewerCtx->appInfo.mainWindow, Input_CursorCallback);
	glfwSetMouseButtonCallback(viewerCtx->appInfo.mainWindow, Input_MouseClickCallback);
	glfwSetKeyCallback(viewerCtx->appInfo.mainWindow, Input_KeyCallback);
	glfwSetScrollCallback(viewerCtx->appInfo.mainWindow, Input_ScrollCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf_error("Failed to initialize GLAD.");
	}
	
	Matrix_Init();
	View_Init(&viewerCtx->viewCtx, &viewerCtx->inputCtx, &viewerCtx->appInfo);
	Input_SetInputPointer(&viewerCtx->inputCtx);
	glfwSetTime(2);
}

void Viewer_Draw_3DViewport(ViewerContext* viewerCtx) {
	u8 setup[16] = {
		0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80,
		0xdf
	};
	
	n64_set_segment(0x02, viewerCtx->objCtx.scene.data);
	Light_Scene_SetLights(&viewerCtx->objCtx.scene, &viewerCtx->lightCtx);
	
	n64_draw(setup);
	zroom_draw(viewerCtx->objCtx.room[0].data);
}

void Viewer_Draw(ViewerContext* viewerCtx) {
	AppInfo* appInfo = &viewerCtx->appInfo;
	InputContext* inputCtx = &viewerCtx->inputCtx;
	LightContext* lightCtx = &viewerCtx->lightCtx;
	ViewContext* viewCtx = &viewerCtx->viewCtx;
	
	Input_Update(inputCtx, appInfo);
	View_Update(viewCtx, inputCtx, &appInfo->winScale);
	Input_End(inputCtx);
	
	glClearColor(
		lightCtx->ambient.r,
		lightCtx->ambient.g,
		lightCtx->ambient.b,
		1.0f
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	Viewer_Draw_3DViewport(viewerCtx);
	glfwSwapBuffers(appInfo->mainWindow);
}
