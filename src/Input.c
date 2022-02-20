#include <z64.h>

InputContext* __pInput;
AppInfo* __pAppInfo;

void Input_Init(InputContext* input) {
	__pInput = input;
}

void Input_Update(InputContext* input, AppInfo* app) {
	MouseInput* mouse = &input->mouse;
	
	__pAppInfo = app;
	{
		static double last = 0;
		double cur = glfwGetTime();
		gDeltaTime = (cur - last) * 50;
		last = cur;
	}
	
	for (s32 i = 0; i < KEY_MAX; i++) {
		input->key[i].press = (input->key[i].prev == 0 && input->key[i].hold != 0);
		input->key[i].prev = input->key[i].hold;
	}
	
	mouse->clickL.press = (mouse->clickL.prev == 0 && mouse->clickL.hold);
	mouse->clickL.prev = mouse->clickL.hold;
	mouse->clickR.press = (mouse->clickR.prev == 0 && mouse->clickR.hold);
	mouse->clickR.prev = mouse->clickR.hold;
	mouse->clickMid.press = (mouse->clickMid.prev == 0 && mouse->clickMid.hold);
	mouse->clickMid.prev = mouse->clickMid.hold;
	
	mouse->click.press = (
		mouse->clickL.press ||
		mouse->clickR.press ||
		mouse->clickMid.press
	);
	
	mouse->click.hold = (
		mouse->clickL.hold ||
		mouse->clickR.hold ||
		mouse->clickMid.hold
	);
	
	mouse->cursorAction = (
		mouse->click.press ||
		mouse->click.hold ||
		mouse->scrollY
	);
	
	if (glfwGetKey(app->mainWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(app->mainWindow, true);
	
	Vec2s* mPos = &mouse->pos;
	Vec2s* mVel = &mouse->vel;
	Vec2s* mPrev = &mouse->prevPos;
	
	mVel->x = (mPos->x - mPrev->x) + mouse->jumpVelComp.x;
	mVel->y = (mPos->y - mPrev->y) + mouse->jumpVelComp.y;
	*mPrev = *mPos;
	
	if (mouse->click.press) {
		mVel->x = mVel->y = 0;
	}
	
	mouse->jumpVelComp.x = 0;
	mouse->jumpVelComp.y = 0;
}

void Input_KeyCallback(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
	InputContext* input = __pInput;
	int hold = action != GLFW_RELEASE;
	s32 pressed = action == GLFW_RELEASE;
	
	input->key[key].hold = hold;
}

void Input_TextCallback(GLFWwindow* window, u32 scancode) {
	InputContext* input = __pInput;
	
	if (!(scancode & 0xFFFFFF00)) {
		if (scancode > 0x7F) {
			printf("\a");
		} else {
			strcat(input->buffer, Tmp_Printf("%c", (char)scancode));
		}
	}
}

void Input_CursorCallback(GLFWwindow* window, f64 xpos, f64 ypos) {
	InputContext* input = __pInput;
	MouseInput* mouse = &input->mouse;
	
	mouse->pos.x = xpos;
	mouse->pos.y = ypos;
}

void Input_MouseClickCallback(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	InputContext* input = __pInput;
	MouseInput* mouse = &input->mouse;
	s32 pressed = action != GLFW_RELEASE;
	s32 hold = action == GLFW_PRESS;
	
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			mouse->clickR.hold = hold;
			break;
			
		case GLFW_MOUSE_BUTTON_LEFT:
			mouse->clickL.hold = hold;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			mouse->clickMid.hold = hold;
	}
}

void Input_ScrollCallback(GLFWwindow* window, f64 x, f64 y) {
	__pInput->mouse.scrollY = y;
}

void Input_End(InputContext* input) {
	MouseInput* mouse = &input->mouse;
	s32 i = 0;
	
	mouse->scrollY = 0;
	
	while (input->buffer[i] != '\0') {
		input->buffer[i++] = '\0';
	}
}

const char* Input_GetClipboardStr() {
	return glfwGetClipboardString(__pAppInfo->mainWindow);
}

void Input_SetClipboardStr(char* str) {
	glfwSetClipboardString(__pAppInfo->mainWindow, str);
}