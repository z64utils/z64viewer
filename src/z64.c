#include <z64.h>

AppInfo* __appInfo;
InputContext* __inputCtx;
ViewContext* __viewCtx;
ObjectContext* __objCtx;
LightContext* __lightCtx;

/* / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / */

void z64_Init(
	const char* title,
	AppInfo* appInfo,
	InputContext* inputCtx,
	ViewContext* viewCtx,
	ObjectContext* objCtx,
	LightContext* lightCtx,
	void* context,
	CallbackFunc updateCall,
	CallbackFunc drawCall3D,
	CallbackFunc drawCall2D
) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_DEBUG);
	
	__appInfo = appInfo;
	__inputCtx = inputCtx;
	__viewCtx = viewCtx;
	__objCtx = objCtx;
	__lightCtx = lightCtx;
	
	appInfo->context = context;
	appInfo->updateCall = updateCall;
	appInfo->drawCall3D = drawCall3D;
	appInfo->drawCall2D = drawCall2D;
	appInfo->winDim.x = 1400;
	appInfo->winDim.y = 700;
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	
	appInfo->mainWindow = glfwCreateWindow(
		appInfo->winDim.x,
		appInfo->winDim.y,
		title,
		NULL,
		NULL
	);
	if (appInfo->mainWindow == NULL) {
		printf_error("Failed to create GLFW window.");
	}
	glfwMakeContextCurrent(appInfo->mainWindow);
	
	glfwSetFramebufferSizeCallback(appInfo->mainWindow, View_FramebufferCallback);
	glfwSetCursorPosCallback(appInfo->mainWindow, Input_CursorCallback);
	glfwSetMouseButtonCallback(appInfo->mainWindow, Input_MouseClickCallback);
	glfwSetKeyCallback(appInfo->mainWindow, Input_KeyCallback);
	glfwSetScrollCallback(appInfo->mainWindow, Input_ScrollCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf_error("Failed to initialize GLAD.");
	}
	
	Matrix_Init();
	View_Init(viewCtx, inputCtx, appInfo);
	Input_SetInputPointer(inputCtx);
	glfwSetTime(2);
	
	viewCtx->cameraControl = true;
}

void z64_Draw() {
	Input_Update(__inputCtx, __appInfo);
	__appInfo->updateCall(__appInfo->context);
	View_Update(__viewCtx, __inputCtx, &__appInfo->winDim);
	Input_End(__inputCtx);
	
	glClearColor(
		__lightCtx->ambient.r,
		__lightCtx->ambient.g,
		__lightCtx->ambient.b,
		1.0f
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	if (__appInfo->drawCall3D) {
		glViewport(
			__appInfo->subscreen.view3D.pos.x,
			__appInfo->subscreen.view3D.pos.y,
			__appInfo->subscreen.view3D.dim.x,
			__appInfo->subscreen.view3D.dim.y
		);
		__appInfo->drawCall3D(__appInfo->context);
	}
	if (__appInfo->drawCall2D) {
		glViewport(0, 0, __appInfo->winDim.x, __appInfo->winDim.y);
		__appInfo->drawCall2D(__appInfo->context);
	}
	__appInfo->isCallback = false;
	glfwSwapBuffers(__appInfo->mainWindow);
	OsPrintfEx("glfwSwapBuffers");
}

void z64_Update() {
	while (!glfwWindowShouldClose(__appInfo->mainWindow)) {
		glfwPollEvents();
		z64_Draw();
	}
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / */

void z64_Draw_SetScene(MemFile* zscene) {
	n64_set_segment(0x02, zscene->data);
	Light_Scene_SetLights(zscene, __lightCtx);
}

void z64_Draw_Room(MemFile* zroom) {
	u8 setup[16] = {
		0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80,
		0xdf
	};
	
	n64_draw(setup);
	zroom_draw(zroom->data);
}

void z64_Draw_Object() {
}

void z64_Draw_Skelanime() {
}