#include <z64.h>

#define HU8(HX) (0.00392156862745f * (HX))
#define HS8(HX) ((char)(HX))

static void* __GetLights(void* zscene) {
	u8* b;
	
	for (b = zscene; *b != 0x14; b += 8) {
		if (*b == 0x0f)
			break;
	}
	
	if (*b != 0x0f)
		return 0;
	
	return n64_virt2phys(u32r(b + 4));
}

void Light_Scene_SetLights(MemFile* zScene, LightContext* lightCtx) {
	u8* lighting;
	
	lighting = __GetLights(zScene->data);
	
	if (lighting) {
		lighting += 22 * 1; /* jump to next lighting list */
		f32 scale = 0.001f;
		f32 fog[2];
		f32 color[3] = { lighting[15], lighting[16], lighting[17] };
		
		color[0] /= 255;
		color[1] /= 255;
		color[2] /= 255;
		memcpy(&lightCtx->ambient, color, sizeof(color));
		
		fog[0] = scale * (u16r(lighting + 18) & 0x3FF);
		fog[1] = scale * u16r(lighting + 20);
		f32 scenelights[16] = {
			// m[0]
			// amb XYZ
			HU8(lighting[0]), HU8(lighting[1]), HU8(lighting[2]),
			// dir1 X
			HS8(lighting[9]),
			// m[1]
			// dif0
			HU8(lighting[6]), HU8(lighting[7]), HU8(lighting[8]),
			// dir1 Y
			HS8(lighting[10]),
			// m[2]
			// dif1
			HU8(lighting[12]), HU8(lighting[13]), HU8(lighting[14]),
			// dir1 Z
			HS8(lighting[11]),
			// m[3]
			// dir0
			HS8(lighting[3]), HS8(lighting[4]), HS8(lighting[5]),
			// unused
			0,
		};
		
		n64_set_fog(fog, color);
		n64_set_lights(scenelights);
	}
}