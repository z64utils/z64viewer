#include <z64.h>

#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))

void Light_BindLights(Scene* scene) {
	LightContext* lightCtx = &scene->lightCtx;
	
	OsAssert(lightCtx->envLight != NULL);
	EnvLight* envLight = &lightCtx->envLight[Wrap(lightCtx->curLightId, 0, lightCtx->lightListNum)];
	f32 fogParam[2];
	f32 fogColor[3];
	f32 light[16] = {
		/* 0x0 */ HU8(envLight->ambient.r), HU8(envLight->ambient.g), HU8(envLight->ambient.b),
		/* 0x3 */ HU8(envLight->colorA.r), HU8(envLight->colorA.g), HU8(envLight->colorA.b),
		/* 0x6 */ HU8(envLight->colorB.r), HU8(envLight->colorB.g), HU8(envLight->colorB.b),
		/* 0x9 */ (f32)envLight->dirA.x / 127.0, (f32)envLight->dirA.y / 127.0, (f32)envLight->dirA.z / 127.0,
		/* 0xC */ (f32)envLight->dirB.x / 127.0, (f32)envLight->dirB.y / 127.0, (f32)envLight->dirB.z / 127.0,
	};
	
	f32 near = (ReadBE(envLight->fogNear) & 0x3FF) * 100;
	f32 far = ReadBE(envLight->fogFar) * 100;
	f32 fogM = (50000.0 * 25600.0) / (far - near);
	f32 fogO = (50000.0 - near) * 25600.0 / (far - near);
	
	fogParam[0] = fogM;
	fogParam[1] = fogO;
	
	if (lightCtx->state & LIGHT_STATE_CHANGED) {
		lightCtx->state &= ~LIGHT_STATE_CHANGED;
		OsPrintfEx("LightID:    %6X", lightCtx->curLightId);
		OsPrintf("Near:       %6.0f", near * 0.01);
		OsPrintf("Far:        %6.0f", far * 0.01);
		OsPrintf("Multiplier: %6.0f", fogM);
		OsPrintf("Offset:     %6.0f", fogO);
	}
	
	for (s32 i = 0; i < 3; i++) {
		fogColor[i] = (f32)envLight->fogColor.c[i] / 255.0;
	}
	
	n64_set_fog(fogParam, fogColor);
	n64_set_lights(light);
}