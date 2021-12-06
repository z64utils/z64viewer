#include <z64viewer.h>

static GlobalContext globalCtx;

void framebuffer_size_callback(GLFWwindow* window, s32 width, s32 height);

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

s32 main(void) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_DEBUG);
	GLFWwindow* window;
	// void* zobj = loadfile("object_link_boy.zobj", 0);
	// void* gameplay_keep = loadfile("gameplay_keep.zobj", 0);
	MemFile zScene = MemFile_Initialize();
	MemFile zRoom = MemFile_Initialize();
	
	MemFile_LoadFile(&zScene, "scene.zscene");
	MemFile_LoadFile(&zRoom, "room_0.zmap");
	
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
	globalCtx.app.mainWindow = window = glfwCreateWindow(globalCtx.app.winScale.x, globalCtx.app.winScale.y, "z64scene", NULL, NULL);
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
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	Matrix_Init();
	View_Init(&globalCtx);
	Input_Init(&globalCtx);
	glfwSetTime(2);
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window)) {
		static f32 bgcolor[3];
		// input
		Input_Update(&globalCtx);
		View_Update(&globalCtx);
		// render
		glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if (zScene.data) {
			u8* lighting;
			n64_set_segment(0x02, zScene.data);
			lighting = getLighting(zScene.data);
			
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
		// if (0 && zobj) {
		// 	if (gameplay_keep)
		// 		n64_set_segment(0x04, gameplay_keep);
		// 	n64_set_segment(0x06, zobj);
		// 	n64_draw(n64_virt2phys(0x06021F78));
		// }
		if (zRoom.data) {
			/* set up base envcolor */
			u8 setup[16] = {
				0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80,
				0xdf
			};
			n64_draw(setup);
			
			zroom_draw(zRoom.data);
		}
		
		Input_End(&globalCtx);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	/* clenaup */
	// if (zroom)
	// 	free(zroom);
	// if (zscene)
	// 	free(zscene);
	// if (zobj)
	// 	free(zobj);
	// if (gameplay_keep)
	// 	free(gameplay_keep);
	glfwTerminate();
	
	return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	globalCtx.app.winScale.x = width;
	globalCtx.app.winScale.y = height;
}
