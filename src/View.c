#include <z64.h>

void View_Camera_FlyMode(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	static Vec3f vel = { 0 };
	Vec3f zro = { 0 };
	Vec3f thisPos = { 0 };
	Vec3f nextPos = { 0 };
	static f32 speed;
	
	if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
		Math_SmoothStepToF(&speed, 4.0f, 0.25f, 1.00f, 0.00001f);
	} else {
		Math_SmoothStepToF(&speed, 0.5f, 0.25f, 1.00f, 0.00001f);
	}
	
	if (inputCtx->key[KEY_A].hold || inputCtx->key[KEY_D].hold) {
		if (inputCtx->key[KEY_A].hold)
			Math_SmoothStepToF(&vel.x, speed, 0.25f, 1.00f, 0.00001f);
		if (inputCtx->key[KEY_D].hold)
			Math_SmoothStepToF(&vel.x, -speed, 0.25f, 1.00f, 0.00001f);
	} else {
		Math_SmoothStepToF(&vel.x, 0, 0.25f, 1.00f, 0.00001f);
	}
	
	if (inputCtx->key[KEY_W].hold || inputCtx->key[KEY_S].hold) {
		if (inputCtx->key[KEY_W].hold)
			Math_SmoothStepToF(&vel.z, speed, 0.25f, 1.00f, 0.00001f);
		if (inputCtx->key[KEY_S].hold)
			Math_SmoothStepToF(&vel.z, -speed, 0.25f, 1.00f, 0.00001f);
	} else {
		Math_SmoothStepToF(&vel.z, 0, 0.25f, 1.00f, 0.00001f);
	}
	
	Vec3f* eye = &cam->eye;
	Vec3f* at = &cam->at;
	
	if (vel.z || vel.x || inputCtx->mouse.clickL.hold) {
		VecSph camSph = {
			.r = Vec_DistXYZ(eye, at),
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		if (inputCtx->mouse.clickL.hold) {
			camSph.yaw -= inputCtx->mouse.vel.x * 65;
			camSph.pitch -= inputCtx->mouse.vel.y * 65;
		}
		
		*at = *eye;
		
		Vec_AddVecSphToVec3f(at, &camSph);
	}
	
	if (vel.z || vel.x) {
		VecSph velSph = {
			.r = vel.z * 1000,
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		Vec_AddVecSphToVec3f(eye, &velSph);
		Vec_AddVecSphToVec3f(at, &velSph);
		
		velSph = (VecSph) {
			.r = vel.x * 1000,
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
	
	if (inputCtx->mouse.clickMid.hold || inputCtx->mouse.scrollY) {
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
		orbitSph.r = CLAMP_MIN(orbitSph.r - (distMult * (inputCtx->mouse.scrollY)), 0.00001f);
		cam->eye = cam->at;
		
		Vec_AddVecSphToVec3f(&cam->eye, &orbitSph);
	}
	
}

void View_Init(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam;
	
	viewCtx->currentCamera = &viewCtx->camera[0];
	cam = viewCtx->currentCamera;
	
	cam->eye = (Vec3f) { -50.0f * 100, 50.0f * 100, 50.0f * 100 };
	cam->at = (Vec3f) { 0, 0, 0 };
	
	cam->eye = (Vec3f) { 0, 0, 50.0f * 100 };
	cam->at = (Vec3f) { 0, 0, 0 };
	cam->roll = 0;
	
	Vec3f up;
	s16 yaw = Vec_Yaw(&cam->eye, &cam->at);
	s16 pitch = Vec_Pitch(&cam->eye, &cam->at);
	
	Matrix_LookAt(&viewCtx->viewMtx, cam->eye, cam->at, cam->roll);
	
	viewCtx->fovy = 65;
	viewCtx->near = 0.1 * 100;
	viewCtx->far = 5000.0 * 100;
	viewCtx->scale = 0.01 * 0.001;
}

void View_Update(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	MtxF model = gMtxFClear;
	Vec3f up;
	s16 yaw;
	s16 pitch;
	
	Matrix_Projection(
		&viewCtx->projMtx,
		viewCtx->fovy,
		(f32)viewCtx->projectDim.x / (f32)viewCtx->projectDim.y,
		viewCtx->near,
		viewCtx->far,
		viewCtx->scale
	);
	
	if (viewCtx->cameraControl) {
		if (inputCtx->mouse.click.press || inputCtx->mouse.scrollY) {
			viewCtx->setCamMove = 1;
		}
		if (inputCtx->mouse.cursorAction == 0) {
			viewCtx->setCamMove = 0;
		}
		View_Camera_OrbitMode(viewCtx, inputCtx);
		View_Camera_FlyMode(viewCtx, inputCtx);
	}
	yaw = Vec_Yaw(&cam->eye, &cam->at);
	pitch = Vec_Pitch(&cam->eye, &cam->at);
	Matrix_LookAt(&viewCtx->viewMtx, cam->eye, cam->at, cam->roll);
	
	Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
	Matrix_ToMtxF(&model);
	n64_setMatrix_model(&model);
	n64_setMatrix_view(&viewCtx->viewMtx);
	n64_setMatrix_projection(&viewCtx->projMtx);
	
	// Billboarding Matrix
	#if 0
	Matrix_Push(); {
		static Mtx mtx;
		Matrix_Mult(&viewCtx->projMtx, MTXMODE_NEW);
		Matrix_Mult(&viewCtx->viewMtx, MTXMODE_APPLY);
		Matrix_Get(&viewCtx->projMtx);
		viewCtx->viewMtx.mf[0][3] = viewCtx->viewMtx.mf[1][3] = viewCtx->viewMtx.mf[2][3] =
		    viewCtx->viewMtx.mf[3][0] = viewCtx->viewMtx.mf[3][1] = viewCtx->viewMtx.mf[3][2] = 0.0f;
		Matrix_Transpose(&viewCtx->viewMtx);
		Matrix_MtxFToMtx(&viewCtx->viewMtx, &mtx);
		gSPSegment(0x1, &mtx);
	} Matrix_Pop();
	#endif
}

void View_SetProjectionDimensions(ViewContext* viewCtx, Vec2s* dim) {
	viewCtx->projectDim = *dim;
}