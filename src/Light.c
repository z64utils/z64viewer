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
	
	s16 fogA = (ReadBE(envLight->fogNear) & 0x3FF);
	s16 fogB = ReadBE(envLight->fogFar);
	s16 fogM = (500 * 0x100) / (fogB - fogA);
	s16 fogO = (500 - fogA) * 0x100 / (fogB - fogA);
	
	fogParam[0] = fogM;
	fogParam[1] = fogO;
	
	for (s32 i = 0; i < 3; i++) {
		fogColor[i] = (f32)envLight->fogColor.c[i] / 255.0;
	}
	
	n64_set_fog(fogParam, fogColor);
	n64_set_lights(light);
}