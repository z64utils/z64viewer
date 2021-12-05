#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <flythrough_camera.h>
#include <zroom.h>
#include <n64.h>
#include <bigendian.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
unsigned int gWindowWidth = 800;
unsigned int gWindowHeight = 600;

struct
{
	struct
	{
		struct
		{
			int x;
			int y;
		} pos;
		struct
		{
			int dy; /* delta y */
		} wheel;
		struct
		{
			int left;
			int right;
		} button;
	} mouse;
	struct
	{
		int w;
		int a;
		int s;
		int d;
		int lshift;
	} key;
	float delta_time_sec;
} gInput = {0};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	int pressed = action != GLFW_RELEASE;
	
	switch (key)
	{
		case GLFW_KEY_W:
			gInput.key.w = pressed;
			break;
		
		case GLFW_KEY_A:
			gInput.key.a = pressed;
			break;
		
		case GLFW_KEY_S:
			gInput.key.s = pressed;
			break;
		
		case GLFW_KEY_D:
			gInput.key.d = pressed;
			break;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	gInput.mouse.pos.x = xpos;
	gInput.mouse.pos.y = ypos;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	int pressed = action != GLFW_RELEASE;
	
	switch (button)
	{
		case GLFW_MOUSE_BUTTON_RIGHT:
			gInput.mouse.button.right = pressed;
			break;
		
		case GLFW_MOUSE_BUTTON_LEFT:
			gInput.mouse.button.left = pressed;
			break;
	}
}

static inline void *camera_flythrough(float m[16])
{
	/* camera magic */
	static float pos[3] = { 0.0f, 0.0f, 0.0f };
	static float look[3] = {0, 0, -1};
	const float up[3] = {0, 1, 0};
	
	static struct {
		int x;
		int y;
	} oldcursor = {0}, cursor = {0};
	
	cursor.x = gInput.mouse.pos.x;
	cursor.y = gInput.mouse.pos.y;
	
	if (gInput.mouse.button.right)
		gInput.mouse.wheel.dy = oldcursor.y - cursor.y;
	else
		gInput.mouse.wheel.dy = 0;
	
	int activated = gInput.mouse.button.left;
	float speed = 1;
	
	flythrough_camera_update(
		pos, look, up, m
		, gInput.delta_time_sec
		, speed * (gInput.key.lshift ? 2.0f : 1.0f)
		, 0.5f * activated
		, 80.0f
		, cursor.x - oldcursor.x, cursor.y - oldcursor.y
		, gInput.key.w
		, gInput.key.a
		, gInput.key.s
		, gInput.key.d
		, 0//gInput.key.space
		, 0//gInput.key.lctrl
		, 0
	);
	
	oldcursor = cursor;
	
	return m;
}

static void *getLighting(void *zscene)
{
	uint8_t *b;
	
	for (b = zscene; *b != 0x14; b += 8)
	{
		if (*b == 0x0f)
			break;
	}
	
	if (*b != 0x0f)
		return 0;
	
	return n64_virt2phys(u32r(b + 4));
}

/* minimal file loader
 * returns 0 on failure
 * returns pointer to loaded file on success
 */
void *loadfile(const char *fn, size_t *sz)
{
	FILE *fp;
	void *dat;
	size_t sz1;
	if (!sz)
		sz = &sz1;
	
	/* rudimentary error checking returns 0 on any error */
	if (
		!fn
		|| !sz
		|| !(fp = fopen(fn, "rb"))
		|| fseek(fp, 0, SEEK_END)
		|| !(*sz = ftell(fp))
		|| fseek(fp, 0, SEEK_SET)
		|| !(dat = malloc(*sz))
		|| fread(dat, 1, *sz, fp) != *sz
		|| fclose(fp)
	)
		return 0;
	
	return dat;
}

static float *identity(float dst[16])
{
	float *m = dst;
	
	memset(dst, 0, sizeof(float[16]));
	
	m[0 * 4 + 0] = 1;
	m[1 * 4 + 1] = 1;
	m[2 * 4 + 2] = 1;
	m[3 * 4 + 3] = 1;
	
	return m;
}

static float *projection(float dst[16], float winw, float winh, float near, float far)
{
	float *m = dst;
	float aspect;
	float f;
	float iNF;

	/* intialize projection matrix */
	aspect = winw / winh;
	f      = 1.0 / tan(60.0f * (3.14159265359f / 180.0f) * 0.5f);
	iNF    = 1.0 / (near - far);

	m[0]=f/aspect; m[4]=0.0f; m[ 8]= 0.0f;           m[12]=0.0f;
	m[1]=0.0f;     m[5]=f;    m[ 9]= 0.0f;           m[13]=0.0f;
	m[2]=0.0f;     m[6]=0.0f; m[10]= (far+near)*iNF; m[14]=2.0f*far*near*iNF;
	m[3]=0.0f;     m[7]=0.0f; m[11]=-1.0f;           m[15]=0.0f;
	
	return m;
}

int main(void)
{
	GLFWwindow* window;
	void *zroom = loadfile("spot04_room_0.zmap", 0);
	void *zscene = loadfile("spot04_scene.zscene", 0);
	void *zobj = loadfile("object_link_boy.zobj", 0);
	void *gameplay_keep = loadfile("gameplay_keep.zobj", 0);
	
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
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "z64viewer", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
	// callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		return -1;
	}
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		static float bgcolor[3];
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// mvp matrix
		float model[16];
		float view[16];
		float p[16];
		
		identity(model);
		camera_flythrough(view);
		projection(p, gWindowWidth, gWindowHeight, 0.1, 500);
		
		n64_setMatrix_model(model);
		n64_setMatrix_view(view);
		n64_setMatrix_projection(p);
		
		if (zscene)
		{
			uint8_t *lighting;
			n64_set_segment(0x02, zscene);
			lighting = getLighting(zscene);
			
			if (lighting)
			{
				lighting += 22 * 1; /* jump to next lighting list */
				float scale = 0.001f;
				float fog[2];
				float color[3] = {lighting[15], lighting[16], lighting[17]};
				
				color[0] /= 255;
				color[1] /= 255;
				color[2] /= 255;
				memcpy(bgcolor, color, sizeof(color));
				
				fog[0] = scale * (u16r(lighting + 18) & 0x3FF);
				fog[1] = scale * u16r(lighting + 20);
				
				#define HU8(HX) (0.00392156862745f*(HX))
				#define HS8(HX) ((char)(HX))
				float scenelights[16] = {
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
		
		if (0 && zobj)
		{
			if (gameplay_keep)
				n64_set_segment(0x04, gameplay_keep);
			n64_set_segment(0x06, zobj);
			n64_draw(n64_virt2phys(0x06021F78));
		}
		if (zroom)
		{
			/* set up base envcolor */
			uint8_t setup[16] = {
				0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80
				, 0xdf
			};
			n64_draw(setup);
			
			zroom_draw(zroom);
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	/* clenaup */
	if (zroom)
		free(zroom);
	if (zscene)
		free(zscene);
	if (zobj)
		free(zobj);
	if (gameplay_keep)
		free(gameplay_keep);
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	/* time since last execution */
	{
		static double last = 0;
		double cur = glfwGetTime();
		gInput.delta_time_sec = (cur - last);
		last = cur;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	gWindowWidth = width;
	gWindowHeight = height;
}


