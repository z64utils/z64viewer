#include <z64.h>

#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))

void Light_SetFog(Scene* scene, ViewContext* viewCtx) {
	LightContext* lightCtx = &scene->lightCtx;
	EnvLight* envLight;
	f32 uFog[2];
	f32 uFogColor[3];
	f32 near;
	f32 far = 1000; // Hardcoded to this value in OoT. z_play.c function Gameplay_SetFog
	f32 fogMultiply;
	f32 fogOffset;
	
	envLight = &lightCtx->envLight[Wrap(lightCtx->curEnvId, 0, lightCtx->envListNum)];
	OsAssert(envLight != NULL);
	near = (ReadBE(envLight->fogNear) & 0x3FF);
	
	if (near >= 1000) {
		fogMultiply = 0;
		fogOffset = 0;
	} else if (near >= 997) {
		fogMultiply = 3276700.0;
		fogOffset = 3302400.0;
	} else if (near < 0) {
		fogMultiply = 0;
		fogOffset = 25500.0;
	} else {
		fogMultiply = (500.0 * 256.0) / (far - near);
		fogOffset = (500.0 - near) * 256.0 / (far - near);
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

void Light_BindEnvLights(Scene* scene) {
	LightContext* lightCtx = &scene->lightCtx;
	EnvLight* envLight;
	
	OsAssert(lightCtx->envLight != NULL);
	envLight = &lightCtx->envLight[Wrap(lightCtx->curEnvId, 0, lightCtx->envListNum)];
	
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
				.x = envLight->dirA.x,
				.y = envLight->dirA.y,
				.z = envLight->dirA.z,
			}
		}
	}
	);
	n64_add_light(
		&(LightInfo) {
		LIGHT_DIRECTIONAL, {
			.dir = {
				.color = envLight->colorB,
				.x = envLight->dirB.x,
				.y = envLight->dirB.y,
				.z = envLight->dirB.z,
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