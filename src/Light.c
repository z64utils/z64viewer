#include <z64.h>

#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))

void Light_SetFog(Scene* scene, ViewContext* viewCtx) {
	LightContext* lightCtx = &scene->lightCtx;
	EnvLight* envLight;
	f32 uFog[2];
	f32 uFogColor[3];
	s16 near;
	/* Hardcoded to this value in OoT. z_play.c function Gameplay_SetFog
	 * NOTE: Adjusted from 1000 to 999.01 to match Angrylion & GlideN64.
	 * Possibly not ideal change, but does not seem to affect results
	 * negatively. Might take another look at this later.
	 */
	f32 far = 999.01;
	s16 fogMultiply;
	s16 fogOffset;
	
	envLight = &lightCtx->envLight[Wrap(lightCtx->curEnvId, 0, lightCtx->envListNum)];
	OsAssert(envLight != NULL);
	near = (ReadBE(envLight->fogNear) & 0x3FF);
	
	if (near >= 1000) {
		fogMultiply = 0;
		fogOffset = 0;
	} else if (near >= 997) {
		fogMultiply = 32767;
		fogOffset = 33024;
	} else if (near < 0) {
		fogMultiply = 0;
		fogOffset = 255;
	} else {
		fogMultiply = ((500 * 0x100) / (far - near));
		fogOffset = ((500 - near) * 0x100 / (far - near));
	}
	
	uFog[0] = fogMultiply;
	uFog[1] = fogOffset;
	
	for (s32 i = 0; i < 3; i++) {
		uFogColor[i] = (f32)envLight->fogColor.c[i] / 255.0;
	}
	
	n64_set_fog(uFog, uFogColor);
	
	if (viewCtx->matchDrawDist)
		viewCtx->far = ReadBE(lightCtx->envLight[lightCtx->curEnvId].fogFar);
}

void Light_BindEnvLights(Scene* scene, Room* currentRoom) {
	OsAssert(scene->lightCtx.envLight != NULL);
	LightContext* lightCtx = &scene->lightCtx;
	EnvLight* envLight = &lightCtx->envLight[Wrap(lightCtx->curEnvId, 0, lightCtx->envListNum)];
	Vec3c dirA = {
		envLight->dirA.x,
		envLight->dirA.y,
		envLight->dirA.z
	};
	Vec3c dirB = {
		envLight->dirB.x,
		envLight->dirB.y,
		envLight->dirB.z
	};
	
	if (currentRoom && currentRoom->inDoorLights == false && lightCtx->curEnvId < 4) {
		switch (lightCtx->curEnvId) {
		    case 0: {
			    lightCtx->dayTime = 0x6000; // 06.00
			    break;
		    }
		    case 1: {
			    lightCtx->dayTime = 0x8001; // 12.00
			    break;
		    }
		    case 2: {
			    lightCtx->dayTime = 0xb556; // 17.00
			    break;
		    }
		    case 3: {
			    lightCtx->dayTime = 0xFFFF; // 24.00
			    break;
		    }
		}
		
		dirA = (Vec3c) {
			(Math_SinS(((void)0, lightCtx->dayTime) - 0x8000) * 120.0f),
			Math_CosS(((void)0, lightCtx->dayTime) - 0x8000) * 120.0f,
			(Math_CosS(((void)0, lightCtx->dayTime) - 0x8000) * 20.0f)
		};
		
		Vec3_Substract(dirB, (Vec3c) { 0 }, dirA);
	}
	
	if (lightCtx->state & LIGHT_STATE_CHANGED) {
		lightCtx->state &= ~LIGHT_STATE_CHANGED;
		
		OsPrintfEx("LightID:    %6X", lightCtx->curEnvId);
	}
	
	n64_clear_lights();
	
	n64_add_light(
		&(LightInfo) {
		LIGHT_AMBIENT, {
			.amb = {
				.color = envLight->ambient
			}
		}
	}
	);
	n64_add_light(
		&(LightInfo) {
		LIGHT_DIRECTIONAL, {
			.dir = {
				.color = envLight->colorA,
				.x = dirA.x,
				.y = dirA.y,
				.z = dirA.z,
			}
		}
	}
	);
	n64_add_light(
		&(LightInfo) {
		LIGHT_DIRECTIONAL, {
			.dir = {
				.color = envLight->colorB,
				.x = dirB.x,
				.y = dirB.y,
				.z = dirB.z,
			}
		}
	}
	);
}

void Light_BindRoomLights(Scene* scene, Room* room) {
	LightContext* lightCtx = &scene->lightCtx;
	LightInfo* infoList = lightCtx->room[room->num].lightList;
	
	for (s32 i = 0; i < lightCtx->room[room->num].lightNum; i++, infoList++) {
		LightInfo light = *infoList;
		
		if (light.type != LIGHT_DIRECTIONAL) {
			light.params.point.radius = ReadBE(light.params.point.radius);
			light.params.point.x = ReadBE(light.params.point.x);
			light.params.point.y = ReadBE(light.params.point.y);
			light.params.point.z = ReadBE(light.params.point.z);
		} else {
		}
		
		if (n64_add_light(&light)) {
			// gLight Full
			break;
		}
	}
}