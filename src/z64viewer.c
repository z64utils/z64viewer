#include <z64viewer.h>

static GlobalContext globalCtx;
static GLFWwindow* window;
static f32 bgcolor[3];

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	globalCtx.app.winScale.x = width;
	globalCtx.app.winScale.y = height;
}

static void* getLighting(void* zscene) {
	u8* b;
	
	for (b = zscene; *b != 0x14; b += 8) {
		if (*b == 0x0f)
			break;
	}
	
	if (*b != 0x0f)
		return 0;
	
	return n64_virt2phys(u32r(b + 4));
}

int z64viewer_shouldWindowClose(void)
{
	return glfwWindowShouldClose(window);
}

void z64viewer_terminate(void)
{
	/* cleanup */
	glfwTerminate();
}

void z64viewer_scene(void *zscene)
{
	u8* lighting;
	
	assert(zscene);
	
	n64_set_segment(0x02, zscene);
	lighting = getLighting(zscene);
	
	if (lighting) {
		lighting += 22 * 1; /* jump to next lighting list */
		f32 scale = 0.001f;
		f32 fog[2];
		f32 color[3] = { lighting[15], lighting[16], lighting[17] };
		
		color[0] /= 255;
		color[1] /= 255;
		color[2] /= 255;
		memcpy(bgcolor, color, sizeof(color));
		
		fog[0] = scale * (u16r(lighting + 18) & 0x3FF);
		fog[1] = scale * u16r(lighting + 20);
		
#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))
		f32 scenelights[16] = {
			// m[0]
			// amb XYZ
			HU8(lighting[0]), HU8(lighting[1]), HU8(lighting[2]),
			// dir1 X
			HS8(lighting[9]),
			// m[1]
			// dif0
			HU8(lighting[6]), HU8(lighting[7]), HU8(lighting[8]),
			// dir1 Y
			HS8(lighting[10]),
			// m[2]
			// dif1
			HU8(lighting[12]), HU8(lighting[13]), HU8(lighting[14]),
			// dir1 Z
			HS8(lighting[11]),
			// m[3]
			// dir0
			HS8(lighting[3]), HS8(lighting[4]), HS8(lighting[5]),
			// unused
			0,
		};
		
		n64_set_fog(fog, color);
		n64_set_lights(scenelights);
	}
}

void z64viewer_room(void *zroom)
{
	/* set up base envcolor */
	u8 setup[16] = {
		0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80,
		0xdf
	};
	
	assert(zroom);
	
	n64_draw(setup);
	
	zroom_draw(zroom);
}

void z64viewer_draw(void drawFunc(void *udata), void *udata)
{
	// render
	glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	drawFunc(udata);
	
	glfwSwapBuffers(window);
}

void z64viewer_update(void)
{
	// input
	Input_Update(&globalCtx);
	View_Update(&globalCtx);
	
	Input_End(&globalCtx);
	glfwPollEvents();
}

s32 z64viewer_init(const char *windowTitle) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_DEBUG);
	
	globalCtx.app.winScale.x = 800;
	globalCtx.app.winScale.y = 600;
	
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	
	// glfw window creation
	// --------------------
	globalCtx.app.mainWindow = window = glfwCreateWindow(globalCtx.app.winScale.x, globalCtx.app.winScale.y, windowTitle, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		
		return -1;
	}
	glfwMakeContextCurrent(window);
	
	// callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, Input_CursorCallback);
	glfwSetMouseButtonCallback(window, Input_MouseClickCallback);
	glfwSetKeyCallback(window, Input_KeyCallback);
	glfwSetScrollCallback(window, Input_ScrollCallback);
	
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Failed to initialize GLAD\n");
		
		return -1;
	}
	
	Matrix_Init();
	View_Init(&globalCtx);
	Input_Init(&globalCtx);
	glfwSetTime(2);
	
	return 0;
}


