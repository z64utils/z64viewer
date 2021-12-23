#include <z64.h>

#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))

void Light_BindLights(Scene* scene) {
	LightContext* lightCtx = &scene->lightCtx;
	
	OsAssert(lightCtx->envLight != NULL);
	EnvLight* envLight = &lightCtx->envLight[Wrap(lightCtx->curLightId, 0, lightCtx->lightListNum)];
	f32 uFog[2];
	f32 uFogColor[3];
	f32 light[16] = {
		/* 0x0 */ HU8(envLight->ambient.r), HU8(envLight->ambient.g), HU8(envLight->ambient.b),
		/* 0x3 */ HU8(envLight->colorA.r), HU8(envLight->colorA.g), HU8(envLight->colorA.b),
		/* 0x6 */ HU8(envLight->colorB.r), HU8(envLight->colorB.g), HU8(envLight->colorB.b),
		/* 0x9 */ (f32)envLight->dirA.x / 127.0, (f32)envLight->dirA.y / 127.0, (f32)envLight->dirA.z / 127.0,
		/* 0xC */ (f32)envLight->dirB.x / 127.0, (f32)envLight->dirB.y / 127.0, (f32)envLight->dirB.z / 127.0,
	};
	
	f32 near = (ReadBE(envLight->fogNear) & 0x3FF);
	f32 far = 1000; // Hardcoded to this value in OoT. z_play.c function Gameplay_SetFog
	f32 fogMultiply;
	f32 fogOffset;
	
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
	
	if (lightCtx->state & LIGHT_STATE_CHANGED) {
		lightCtx->state &= ~LIGHT_STATE_CHANGED;
		
		OsPrintfEx("LightID:    %6X", lightCtx->curLightId);
	}
	
	for (s32 i = 0; i < 3; i++) {
		uFogColor[i] = (f32)envLight->fogColor.c[i] / 255.0;
	}
	
	n64_set_fog(uFog, uFogColor);
	n64_clear_lights();
	n64_add_light(&(LightInfo){
		LIGHT_AMBIENT, {
			.amb = {
				.color = envLight->ambient
			}
		}
	});
	n64_add_light(&(LightInfo){
		LIGHT_DIRECTIONAL, {
			.dir = {
				.color = envLight->colorA,
				.x = envLight->dirA.x,
				.y = envLight->dirA.y,
				.z = envLight->dirA.z,
			}
		}
	});
	n64_add_light(&(LightInfo){
		LIGHT_DIRECTIONAL, {
			.dir = {
				.color = envLight->colorB,
				.x = envLight->dirB.x,
				.y = envLight->dirB.y,
				.z = envLight->dirB.z,
			}
		}
	});
	//n64_set_lights(light);
}
