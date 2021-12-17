#include <z64.h>

static ObjectContext* __objCtx;
static LightContext* __lightCtx;

InputContext* __inputCtx;
AppInfo* __appInfo;

void z64_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	__appInfo->prevWinDim.x = __appInfo->winDim.x;
	__appInfo->prevWinDim.y = __appInfo->winDim.y;
	__appInfo->winDim.x = width;
	__appInfo->winDim.y = height;
	__appInfo->isResizeCallback = true;
	
	z64_Draw();
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / */

void z64_Init(
	const char* title,
	AppInfo* appInfo,
	InputContext* inputCtx,
	ObjectContext* objCtx,
	LightContext* lightCtx,
	void* context,
	CallbackFunc updateCall,
	CallbackFunc drawCall,
	u32 x,
	u32 y
) {
	__appInfo = appInfo;
	__inputCtx = inputCtx;
	__objCtx = objCtx;
	__lightCtx = lightCtx;
	
	appInfo->context = context;
	appInfo->updateCall = updateCall;
	appInfo->drawCall = drawCall;
	appInfo->winDim.x = x;
	appInfo->winDim.y = y;
	
	Rand_Seed(0xDEADBEEF);
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// TODO Set this in settings?
	// glfwWindowHint(GLFW_SAMPLES, 4);
	
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
	
	glfwSetFramebufferSizeCallback(appInfo->mainWindow, z64_FramebufferCallback);
	glfwSetCursorPosCallback(appInfo->mainWindow, Input_CursorCallback);
	glfwSetMouseButtonCallback(appInfo->mainWindow, Input_MouseClickCallback);
	glfwSetKeyCallback(appInfo->mainWindow, Input_KeyCallback);
	glfwSetScrollCallback(appInfo->mainWindow, Input_ScrollCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf_error("Failed to initialize GLAD.");
	}
	
	Matrix_Init();
	Input_Init(inputCtx);
	glfwSetTime(2);
}

void z64_Draw() {
	Input_Update(__inputCtx, __appInfo);
	__appInfo->updateCall(__appInfo->context);
	
	glClearColor(
		__lightCtx->ambient.r,
		__lightCtx->ambient.g,
		__lightCtx->ambient.b,
		1.0f
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	if (__appInfo->drawCall) {
		__appInfo->drawCall(__appInfo->context);
	}
	__appInfo->isResizeCallback = false;
	Input_End(__inputCtx);
	glfwSwapBuffers(__appInfo->mainWindow);
}

static f64 prevTime = 0;
static f64 curTime = 0;

bool z64_ExecuteIn20Fps() {
	curTime = glfwGetTime();
	
	if (curTime - prevTime < 1.0 / 20.0)
		return 0;
	
	return 1;
}

void z64_20fpsUpdate() {
	if (curTime - prevTime < 1.0 / 20.0)
		return;
	prevTime = curTime;
}

void z64_Update() {
	while (!glfwWindowShouldClose(__appInfo->mainWindow)) {
		glfwPollEvents();
		z64_Draw();
		z64_20fpsUpdate();
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
	n64_set_onlyGeoLayer(GEOLAYER_OPAQUE);
	zroom_draw(zroom->data);
	n64_set_onlyGeoLayer(GEOLAYER_OVERLAY);
	zroom_draw(zroom->data);
}

void z64_Draw_Object() {
}

void z64_Draw_Skelanime() {
}
