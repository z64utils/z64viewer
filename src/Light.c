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
		HU8(envLight->ambient.r), HU8(envLight->ambient.g), HU8(envLight->ambient.b),
		HS8(envLight->dirB.x),
		HU8(envLight->colorA.r), HU8(envLight->colorA.g), HU8(envLight->colorA.b),
		HS8(envLight->dirB.y),
		HU8(envLight->colorB.r), HU8(envLight->colorB.g), HU8(envLight->colorB.b),
		HS8(envLight->dirB.z),
		HS8(envLight->dirA.x), HS8(envLight->dirA.y), HS8(envLight->dirA.z),
		0,
	};
	
	fogParam[0] = (ReadBE(envLight->fogNear) & 0x3FF);
	fogParam[1] = ReadBE(envLight->fogFar);
	
	for (s32 i = 0; i < 3; i++) {
		fogColor[i] = (f32)envLight->fogColor.c[i] / 255.0;
	}
	
	n64_set_fog(fogParam, fogColor);
	n64_set_lights(light);
}