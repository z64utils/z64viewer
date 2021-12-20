#include <z64.h>

void View_Camera_FlyMode(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	Vec3f zro = { 0 };
	Vec3f thisPos = { 0 };
	Vec3f nextPos = { 0 };
	
	if (viewCtx->cameraControl) {
		if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
			Math_SmoothStepToF(&viewCtx->flyMode.speed, 4.0f, 0.25f, 1.00f, 0.00001f);
		} else if (inputCtx->key[KEY_SPACE].hold) {
			Math_SmoothStepToF(&viewCtx->flyMode.speed, 16.0f, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_SmoothStepToF(&viewCtx->flyMode.speed, 0.5f, 0.25f, 1.00f, 0.00001f);
		}
		
		if (inputCtx->key[KEY_A].hold || inputCtx->key[KEY_D].hold) {
			if (inputCtx->key[KEY_A].hold)
				Math_SmoothStepToF(&viewCtx->flyMode.vel.x, viewCtx->flyMode.speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_D].hold)
				Math_SmoothStepToF(&viewCtx->flyMode.vel.x, -viewCtx->flyMode.speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_SmoothStepToF(&viewCtx->flyMode.vel.x, 0, 0.25f, 1.00f, 0.00001f);
		}
		
		if (inputCtx->key[KEY_W].hold || inputCtx->key[KEY_S].hold) {
			if (inputCtx->key[KEY_W].hold)
				Math_SmoothStepToF(&viewCtx->flyMode.vel.z, viewCtx->flyMode.speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_S].hold)
				Math_SmoothStepToF(&viewCtx->flyMode.vel.z, -viewCtx->flyMode.speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_SmoothStepToF(&viewCtx->flyMode.vel.z, 0, 0.25f, 1.00f, 0.00001f);
		}
	} else {
		Math_SmoothStepToF(&viewCtx->flyMode.speed, 0.5f, 0.25f, 1.00f, 0.00001f);
		Math_SmoothStepToF(&viewCtx->flyMode.vel.x, 0, 0.25f, 1.00f, 0.00001f);
		Math_SmoothStepToF(&viewCtx->flyMode.vel.z, 0, 0.25f, 1.00f, 0.00001f);
	}
	
	Vec3f* eye = &cam->eye;
	Vec3f* at = &cam->at;
	
	if (viewCtx->flyMode.vel.z || viewCtx->flyMode.vel.x || (viewCtx->cameraControl && inputCtx->mouse.clickL.hold)) {
		VecSph camSph = {
			.r = Vec_Vec3f_DistXYZ(eye, at),
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		if (viewCtx->cameraControl && inputCtx->mouse.clickL.hold) {
			camSph.yaw -= inputCtx->mouse.vel.x * 65;
			camSph.pitch -= inputCtx->mouse.vel.y * 65;
		}
		
		*at = *eye;
		
		Vec_AddVecSphToVec3f(at, &camSph);
	}
	
	if (viewCtx->flyMode.vel.z || viewCtx->flyMode.vel.x) {
		VecSph velSph = {
			.r = viewCtx->flyMode.vel.z * 1000,
			.yaw = Vec_Yaw(at, eye),
			.pitch = Vec_Pitch(at, eye)
		};
		
		Vec_AddVecSphToVec3f(eye, &velSph);
		Vec_AddVecSphToVec3f(at, &velSph);
		
		velSph = (VecSph) {
			.r = viewCtx->flyMode.vel.x * 1000,
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
		.r = Vec_Vec3f_DistXYZ(&cam->at, &cam->eye),
		.yaw = Vec_Yaw(&cam->eye, &cam->at),
		.pitch = Vec_Pitch(&cam->eye, &cam->at)
	};
	f32 distMult = (orbitSph.r * 0.1);
	
	if (viewCtx->cameraControl) {
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
			if (inputCtx->key[KEY_LEFT_CONTROL].hold) {
				viewCtx->fovyTarget = CLAMP(viewCtx->fovyTarget * (1.0 + (inputCtx->mouse.scrollY / 20)), 20, 170);
			} else {
				orbitSph.r = CLAMP_MIN(orbitSph.r - (distMult * (inputCtx->mouse.scrollY)), 0.00001f);
				cam->eye = cam->at;
				
				Vec_AddVecSphToVec3f(&cam->eye, &orbitSph);
			}
		}
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
	
	viewCtx->fovyTarget = viewCtx->fovy = 65;
	viewCtx->near = 0.1 * 1000;
	viewCtx->far = 1000.0 * 1000;
	viewCtx->scale = 0.01;
}

void View_Update(ViewContext* viewCtx, InputContext* inputCtx) {
	Camera* cam = viewCtx->currentCamera;
	MtxF model = gMtxFClear;
	Vec3f up;
	s16 yaw;
	s16 pitch;
	
	Math_SmoothStepToF(&viewCtx->fovy, viewCtx->fovyTarget, 0.25, 5.25f, 0.00001);
	
	Matrix_Projection(
		&viewCtx->projMtx,
		viewCtx->fovy,
		(f32)viewCtx->projectDim.x / (f32)viewCtx->projectDim.y,
		viewCtx->near,
		viewCtx->far,
		viewCtx->scale
	);
	
	if (inputCtx->mouse.click.press || inputCtx->mouse.scrollY) {
		viewCtx->setCamMove = 1;
	}
	if (inputCtx->mouse.cursorAction == 0) {
		viewCtx->setCamMove = 0;
	}
	View_Camera_OrbitMode(viewCtx, inputCtx);
	View_Camera_FlyMode(viewCtx, inputCtx);
	yaw = Vec_Yaw(&cam->eye, &cam->at);
	pitch = Vec_Pitch(&cam->eye, &cam->at);
	Matrix_LookAt(&viewCtx->viewMtx, cam->eye, cam->at, cam->roll);
	
	Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
	Matrix_ToMtxF(&model);
	n64_setMatrix_model(&model);
	n64_setMatrix_view(&viewCtx->viewMtx);
	n64_setMatrix_projection(&viewCtx->projMtx);
	
	// Billboarding Matrices
	Matrix_Push(); {
		static Mtx mtx[2];
		MtxF* vm = &viewCtx->viewMtx;
		u16* cyl = (void*)(mtx + 1); // cylinder matrix
		
		float scale = 0.01f; // XXX magic scale value, why?
		vm->xx *= scale;
		vm->yx *= scale;
		vm->zx *= scale;
		vm->xy *= scale;
		vm->yy *= scale;
		vm->zy *= scale;
		vm->xz *= scale;
		vm->yz *= scale;
		vm->zz *= scale;
		vm->wx *= scale;
		vm->wy *= scale;
		vm->wz *= scale;
		
		/* create spherical matrix, upload to segment 0x01 */
		vm->mf[0][3] = vm->mf[1][3] = vm->mf[2][3] =
		    vm->mf[3][0] = vm->mf[3][1] = vm->mf[3][2] = 0.0f;
		Matrix_Transpose(vm);
		Matrix_MtxFToMtx(vm, &mtx[0]);
		gSPSegment(0x1, mtx);
		
		/* cylinder = spherical, with up vector reverted to identity */
		memcpy(cyl, mtx, sizeof(*mtx));
		cyl[0x08 / 2] = 0; /* x */
		cyl[0x0A / 2] = 1; /* y */
		cyl[0x0C / 2] = 0; /* z */
		
		cyl[0x28 / 2] = 0; /* x */
		cyl[0x2A / 2] = 0; /* y */
		cyl[0x2C / 2] = 0; /* z */
	} Matrix_Pop();
}

void View_SetProjectionDimensions(ViewContext* viewCtx, Vec2s* dim) {
	viewCtx->projectDim = *dim;
}
