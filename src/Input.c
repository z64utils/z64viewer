#include <_global.h>

InputContext* __pInput;

void Input_SetInputPointer(InputContext* input) {
	__pInput = input;
}

void Input_Update(InputContext* input, AppInfo* app) {
	{
		static double last = 0;
		double cur = glfwGetTime();
		gDeltaTime = (cur - last) * 50;
		last = cur;
	}
	
	for (s32 i = 0; i < KEY_MAX; i++) {
		input->key[i].press = (input->key[i].prev == 0 && input->key[i].hold);
		input->key[i].prev = input->key[i].hold;
	}
	
	input->mouse.clickL.press = (input->mouse.clickL.prev == 0 && input->mouse.clickL.hold);
	input->mouse.clickL.prev = input->mouse.clickL.hold;
	input->mouse.clickR.press = (input->mouse.clickR.prev == 0 && input->mouse.clickR.hold);
	input->mouse.clickR.prev = input->mouse.clickR.hold;
	input->mouse.clickMid.press = (input->mouse.clickMid.prev == 0 && input->mouse.clickMid.hold);
	input->mouse.clickMid.prev = input->mouse.clickMid.hold;
	
	if (glfwGetKey(app->mainWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(app->mainWindow, true);
	
	Vec2i* mPos = &input->mouse.pos;
	Vec2i* mVel = &input->mouse.vel;
	Vec2i* mPrev = &input->mouse.prevPos;
	
	mVel->x = mPos->x - mPrev->x;
	mVel->y = mPos->y - mPrev->y;
	*mPrev = *mPos;
}

void Input_KeyCallback(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
	InputContext* input = __pInput;
	int hold = action != GLFW_RELEASE;
	s32 pressed = action == GLFW_RELEASE;
	
	input->key[key].hold = hold;
}

void Input_CursorCallback(GLFWwindow* window, f64 xpos, f64 ypos) {
	InputContext* input = __pInput;
	
	input->mouse.pos.x = xpos;
	input->mouse.pos.y = ypos;
}

void Input_MouseClickCallback(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	InputContext* input = __pInput;
	s32 pressed = action != GLFW_RELEASE;
	s32 hold = action == GLFW_PRESS;
	
	switch (button) {
	    case GLFW_MOUSE_BUTTON_RIGHT:
		    input->mouse.clickR.hold = hold;
		    break;
		    
	    case GLFW_MOUSE_BUTTON_LEFT:
		    input->mouse.clickL.hold = hold;
		    break;
	    case GLFW_MOUSE_BUTTON_MIDDLE:
		    input->mouse.clickMid.hold = hold;
	}
}

void Input_ScrollCallback(GLFWwindow* window, f64 x, f64 y) {
	__pInput->mouse.scrollY = y;
}

void Input_End(InputContext* input) {
	input->mouse.scrollY = 0;
}
