#include <z64viewer.h>

MtxF sMtxView;
MtxF sMtxProj;

void View_Camera_FlyMode(GlobalContext* globalCtx) {
	Camera* cam = ACTIVE_CAM;
	Input* input = &globalCtx->input;
	Vec3f vel = { 0 };
	Vec3f zro = { 0 };
	Vec3f thisPos = { 0 };
	Vec3f nextPos = { 0 };
	static f32 speed;
	
	if (input->key[KEY_LEFT_SHIFT].hold) {
		Math_SmoothStepToF(&speed, 4.0f, 0.25f, 1.00f, 0.00001f);
	} else {
		Math_SmoothStepToF(&speed, 1.0f, 0.25f, 1.00f, 0.00001f);
	}
	
	vel.x += input->key[KEY_A].hold ? speed : 0.0f;
	vel.x -= input->key[KEY_D].hold ? speed : 0.0f;
	vel.z += input->key[KEY_W].hold ? speed : 0.0f;
	vel.z -= input->key[KEY_S].hold ? speed : 0.0f;
	
	Vec3f* eye = &cam->eye;
	Vec3f* at = &cam->at;
	
	if (input->mouse.clickL.hold) {
		VecSph camSph = {
			.r = Vec_DistXYZ(eye, at),
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		camSph.yaw -= input->mouse.vel.x * 100;
		camSph.pitch -= input->mouse.vel.y * 100;
		
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

void View_Camera_OrbitMode(GlobalContext* globalCtx) {
	Camera* cam = ACTIVE_CAM;
	VecSph orbitSph = {
		.r = Vec_DistXYZ(&cam->at, &cam->eye),
		.yaw = Vec_Yaw(&cam->eye, &cam->at),
		.pitch = Vec_Pitch(&cam->eye, &cam->at)
	};
	f32 distMult = (orbitSph.r * 0.1);
	
	if (globalCtx->input.mouse.clickMid.hold) {
		if (globalCtx->input.key[KEY_LEFT_SHIFT].hold) {
			VecSph velSph = {
				.r = globalCtx->input.mouse.vel.y * distMult * 0.02f,
				.yaw = Vec_Yaw(&cam->at, &cam->eye),
				.pitch = Vec_Pitch(&cam->at, &cam->eye) + 0x3FFF
			};
			
			Vec_AddVecSphToVec3f(&cam->eye, &velSph);
			Vec_AddVecSphToVec3f(&cam->at, &velSph);
			
			velSph = (VecSph) {
				.r = globalCtx->input.mouse.vel.x * distMult * 0.02f,
				.yaw = Vec_Yaw(&cam->at, &cam->eye) + 0x3FFF,
				.pitch = 0
			};
			
			Vec_AddVecSphToVec3f(&cam->eye, &velSph);
			Vec_AddVecSphToVec3f(&cam->at, &velSph);
		} else {
			orbitSph.yaw -= globalCtx->input.mouse.vel.x * 100;
			orbitSph.pitch += globalCtx->input.mouse.vel.y * 100;
		}
	}
	
	orbitSph.r = CLAMP_MIN(orbitSph.r - (distMult * (globalCtx->input.mouse.scrollY)), 2.0f);
	cam->eye = cam->at;
	
	Vec_AddVecSphToVec3f(&cam->eye, &orbitSph);
}

void View_Init(GlobalContext* globalCtx) {
	View* view = &globalCtx->view;
	Camera* cam;
	
	globalCtx->view.currentCamera = &globalCtx->view.camera[0];
	cam = ACTIVE_CAM;
	
	cam->eye = (Vec3f) { 0.0f, 8.0f, 0 };
	
	cam->at = (Vec3f) { 0, 0, -50.0f };
	cam->roll = 0;
	
	Vec3f up;
	s16 yaw = Vec_Yaw(&cam->eye, &cam->at);
	s16 pitch = Vec_Pitch(&cam->eye, &cam->at);
	
	Matrix_LookAt(&sMtxView, cam->eye, cam->at, cam->roll);
}

void View_Update(GlobalContext* globalCtx) {
	Camera* cam = ACTIVE_CAM;
	View* view = &globalCtx->view;
	MtxF model = gMtxFClear;
	Vec3f up;
	s16 yaw;
	s16 pitch;
	
	view->mtxProj = &sMtxProj;
	view->mtxView = &sMtxView;
	
	Matrix_Projection(
		&sMtxProj,
		50,
		(f32)globalCtx->app.winScale.x / globalCtx->app.winScale.y,
		0.1,
		500,
		0.01f
	);
	
	View_Camera_OrbitMode(globalCtx);
	View_Camera_FlyMode(globalCtx);
	yaw = Vec_Yaw(&cam->eye, &cam->at);
	pitch = Vec_Pitch(&cam->eye, &cam->at);
	Matrix_LookAt(&sMtxView, cam->eye, cam->at, cam->roll);
	
	n64_setMatrix_model(&model);
	n64_setMatrix_view(&sMtxView);
	n64_setMatrix_projection(&sMtxProj);
}
