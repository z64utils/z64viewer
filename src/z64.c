#include <z64.h>

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
	void* context,
	CallbackFunc updateCall,
	CallbackFunc drawCall,
	DropCallback dropCallback,
	u32 x,
	u32 y,
	u32 samples
) {
	__appInfo = appInfo;
	__inputCtx = inputCtx;
	
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
	if (samples)
		glfwWindowHint(GLFW_SAMPLES, samples);
	
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
	if (dropCallback)
		glfwSetDropCallback(appInfo->mainWindow, dropCallback);
	
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
		0.0f,
		0.0f,
		0.0f,
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

bool Zelda64_20fpsLimiter() {
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

s8 Zelda64_EyeBlink(s16* frame) {
	if ( *frame == 0 )
		*frame = Rand_S16Offset(30, 30);
	*frame -= 1;
	if ( *frame > 1 )
		return 0;
	
	return 2 - *frame;
}
