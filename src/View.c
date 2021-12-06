#include <_global.h>

MtxF sMtxView;
MtxF sMtxProj;
AppInfo* __appInfo;

void View_Camera_FlyMode(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	Vec3f vel = { 0 };
	Vec3f zro = { 0 };
	Vec3f thisPos = { 0 };
	Vec3f nextPos = { 0 };
	static f32 speed;
	
	if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
		Math_SmoothStepToF(&speed, 4.0f, 0.25f, 1.00f, 0.00001f);
	} else {
		Math_SmoothStepToF(&speed, 1.0f, 0.25f, 1.00f, 0.00001f);
	}
	
	vel.x += inputCtx->key[KEY_A].hold ? speed : 0.0f;
	vel.x -= inputCtx->key[KEY_D].hold ? speed : 0.0f;
	vel.z += inputCtx->key[KEY_W].hold ? speed : 0.0f;
	vel.z -= inputCtx->key[KEY_S].hold ? speed : 0.0f;
	
	Vec3f* eye = &cam->eye;
	Vec3f* at = &cam->at;
	
	if (inputCtx->mouse.clickL.hold) {
		VecSph camSph = {
			.r = Vec_DistXYZ(eye, at),
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		camSph.yaw -= inputCtx->mouse.vel.x * 65;
		camSph.pitch -= inputCtx->mouse.vel.y * 65;
		
		*at = *eye;
		
		Vec_AddVecSphToVec3f(at, &camSph);
	}
	
	if (vel.z || vel.x) {
		VecSph velSph = {
			.r = vel.z,
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		Vec_AddVecSphToVec3f(eye, &velSph);
		Vec_AddVecSphToVec3f(at, &velSph);
		
		velSph = (VecSph) {
			.r = vel.x,
			.yaw = Vec_Yaw(at, eye) + 0x3FFF,
			.pitch = 0
		};
		
		Vec_AddVecSphToVec3f(eye, &velSph);
		Vec_AddVecSphToVec3f(at, &velSph);
	}
}

void View_Camera_OrbitMode(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	VecSph orbitSph = {
		.r = Vec_DistXYZ(&cam->at, &cam->eye),
		.yaw = Vec_Yaw(&cam->eye, &cam->at),
		.pitch = Vec_Pitch(&cam->eye, &cam->at)
	};
	f32 distMult = (orbitSph.r * 0.1);
	
	if (inputCtx->mouse.clickMid.hold) {
		if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
			VecSph velSph = {
				.r = inputCtx->mouse.vel.y * distMult * 0.01f,
				.yaw = Vec_Yaw(&cam->at, &cam->eye),
				.pitch = Vec_Pitch(&cam->at, &cam->eye) + 0x3FFF
			};
			
			Vec_AddVecSphToVec3f(&cam->eye, &velSph);
			Vec_AddVecSphToVec3f(&cam->at, &velSph);
			
			velSph = (VecSph) {
				.r = inputCtx->mouse.vel.x * distMult * 0.01f,
				.yaw = Vec_Yaw(&cam->at, &cam->eye) + 0x3FFF,
				.pitch = 0
			};
			
			Vec_AddVecSphToVec3f(&cam->eye, &velSph);
			Vec_AddVecSphToVec3f(&cam->at, &velSph);
		} else {
			orbitSph.yaw -= inputCtx->mouse.vel.x * 67;
			orbitSph.pitch += inputCtx->mouse.vel.y * 67;
		}
	}
	
	orbitSph.r = CLAMP_MIN(orbitSph.r - (distMult * (inputCtx->mouse.scrollY * 1.5f)), 2.0f);
	cam->eye = cam->at;
	
	Vec_AddVecSphToVec3f(&cam->eye, &orbitSph);
}

void View_Init(ViewContext* viewCtx, InputContext* inputCtx, AppInfo* appInfo) {
	Camera* cam;
	
	__appInfo = appInfo;
	viewCtx->currentCamera = &viewCtx->camera[0];
	cam = viewCtx->currentCamera;
	
	cam->eye = (Vec3f) { 0.0f, 8.0f, 0 };
	
	cam->at = (Vec3f) { 0, 0, -50.0f };
	cam->roll = 0;
	
	Vec3f up;
	s16 yaw = Vec_Yaw(&cam->eye, &cam->at);
	s16 pitch = Vec_Pitch(&cam->eye, &cam->at);
	
	Matrix_LookAt(&sMtxView, cam->eye, cam->at, cam->roll);
}

void View_Update(ViewContext* viewCtx, InputContext* inputCtx, Vec2f* winDim) {
	Camera* cam = viewCtx->currentCamera;
	MtxF model = gMtxFClear;
	Vec3f up;
	s16 yaw;
	s16 pitch;
	
	viewCtx->mtxProj = &sMtxProj;
	viewCtx->mtxView = &sMtxView;
	
	Matrix_Projection(
		&sMtxProj,
		50,
		__appInfo->subscreen.view3D.dim.x / __appInfo->subscreen.view3D.dim.y,
		0.1,
		5000,
		0.01f
	);
	
	if (viewCtx->cameraControl) {
		View_Camera_OrbitMode(viewCtx, inputCtx);
		View_Camera_FlyMode(viewCtx, inputCtx);
	}
	yaw = Vec_Yaw(&cam->eye, &cam->at);
	pitch = Vec_Pitch(&cam->eye, &cam->at);
	Matrix_LookAt(&sMtxView, cam->eye, cam->at, cam->roll);
	
	n64_setMatrix_model(&model);
	n64_setMatrix_view(&sMtxView);
	n64_setMatrix_projection(&sMtxProj);
}

void View_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	__appInfo->prevWinDim.x = __appInfo->winDim.x;
	__appInfo->prevWinDim.y = __appInfo->winDim.y;
	__appInfo->winDim.x = width;
	__appInfo->winDim.y = height;
	__appInfo->isCallback = true;
	
	if (__appInfo->drawCall && __appInfo->mainCtx) {
		__appInfo->drawCall(__appInfo->mainCtx);
	}
}
