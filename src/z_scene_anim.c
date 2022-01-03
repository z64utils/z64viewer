#include <Global.h>
#include <z_scene.h>

u32 gGameplayFrames;
s16 gRoomUnk[2];
SceneID gSceneNum;

Gfx sDefaultDisplayList[24];
Gfx sEmptyDL[] = {
	gsSPEndDisplayList(),
};

#define NIGHT_FLAG (scene->lightCtx.curEnvId == 3)

void Scene_DrawConfig_00(Scene* scene) {
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 128, 128, 128, 128);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
}

void Scene_DrawConfig_01(Scene* scene) {
	Gfx* displayListHead;
	
	displayListHead = Graph_Alloc(3 * sizeof(Gfx));
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gSPSegment(POLY_XLU_DISP++, 0x0A, displayListHead);
	
	#if 0
		if ((gSaveContext.dayTime >= 0x4AAC) && (gSaveContext.dayTime <= 0xC555)) {
			gSPEndDisplayList(displayListHead);
		} else {
			if (gSaveContext.dayTime > 0xC555) {
				if (gRoomUnk[0] != 255) {
					Math_StepToS(&gRoomUnk[0], 255, 5);
				}
			} else if (gSaveContext.dayTime >= 0x4000) {
				if (gRoomUnk[0] != 0) {
					Math_StepToS(&gRoomUnk[0], 0, 10);
				}
			}
			
			gDPSetPrimColor(displayListHead++, 0, 0, 255, 255, 255, gRoomUnk[0]);
			gSPDisplayList(displayListHead++, spot00_room_0DL_012B20);
			gSPEndDisplayList(displayListHead);
		}
	#endif
}
void Scene_DrawConfig_02_KakarikoVillage(Scene* scene) {
	#define gKakarikoVillageDayWindowTex   0x02000000 | 0x15B50
	#define gKakarikoVillageNightWindowTex 0x02000000 | 0x16B50
	u32 sKakarikoWindowTextures[] = {
		gKakarikoVillageDayWindowTex,
		gKakarikoVillageNightWindowTex,
	};
	
	gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sKakarikoWindowTextures[NIGHT_FLAG]));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_03(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 6) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 6) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_04(Scene* scene) {
	u8 spA3;
	u16 spA0;
	Gfx* displayListHead;
	
	spA3 = 128;
	spA0 = 500;
	displayListHead = Graph_Alloc(6 * sizeof(Gfx));
	
	if (1) {
	}
	if (1) {
	}
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	#if 0
		if (gSaveContext.sceneSetupIndex == 4) {
			spA3 = 255 - (u8)gRoomUnk[0];
		} else if (gSaveContext.sceneSetupIndex == 6) {
			spA0 = gRoomUnk[0] + 500;
		} else if (((gSaveContext.sceneSetupIndex < 4) || LINK_IS_ADULT) && (gSaveContext.eventChkInf[0] & 0x80)) {
			spA0 = 2150;
		}
	#else
		spA0 = 2150;
	#endif
	
	gSPSegment(POLY_OPA_DISP++, 0x0A, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 128, 128, 128, spA3);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_XLU_DISP++, 0x0B, displayListHead);
	gSPSegment(POLY_OPA_DISP++, 0x0B, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 128, 128, 128, spA0 * 0.1f);
	gSPEndDisplayList(displayListHead);
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x0C,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(s16)(-gRoomUnk[0] * 0.02f),
			32,
			16,
			1,
			0,
			(s16)(-gRoomUnk[0] * 0.02f),
			32,
			16
		)
	);
}
void Scene_DrawConfig_05(Scene* scene) {
	#if 0
		if ((gSaveContext.sceneSetupIndex > 3) || (LINK_IS_ADULT && !(gSaveContext.eventChkInf[6] & 0x200))) {
			gRoomUnk[0] = 87;
		}
	#endif
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScrollEnvColor(
			
			0,
			gGameplayFrames,
			gGameplayFrames,
			32,
			32,
			1,
			0,
			0,
			32,
			32,
			0,
			0,
			0,
			gRoomUnk[0] + 168
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		Gfx_TwoTexScrollEnvColor(
			
			0,
			-gGameplayFrames,
			-gGameplayFrames,
			32,
			32,
			1,
			0,
			0,
			16,
			64,
			0,
			0,
			0,
			gRoomUnk[0] + 168
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 128);
}
void Scene_DrawConfig_06_ZorasDomain(Scene* scene) {
	#define gZorasDomainDayEntranceTex   0x02000000 | 0x8F98
	#define gZorasDomainNightEntranceTex 0x02000000 | 0x8FD8
	u32 sZorasDomainEntranceTextures[] = {
		gZorasDomainDayEntranceTex,
		gZorasDomainNightEntranceTex,
	};
	u32 var;
	
	var = 127 - (gGameplayFrames * 1) % 128;
	#if 0
		if (LINK_IS_ADULT) {
			var = 0;
		}
	#endif
	gSPSegment(
		POLY_OPA_DISP++,
		0x0C,
		Gfx_TwoTexScroll( 0, 0, 0, 64, 32, 1, 0, var, 64, 32)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(sZorasDomainEntranceTextures[NIGHT_FLAG])
	);
}
void Scene_DrawConfig_07(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll( 0, (gGameplayFrames * 1) % 128, 0, 32, 32, 1, 0, 0, 32, 32)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			255 - (gGameplayFrames * 2) % 256,
			64,
			64,
			1,
			0,
			255 - (gGameplayFrames * 2) % 256,
			64,
			64
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_08(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 3) % 1024,
			32,
			256,
			1,
			0,
			(gGameplayFrames * 3) % 1024,
			32,
			256
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 256,
			64,
			64,
			1,
			0,
			(gGameplayFrames * 1) % 256,
			64,
			64
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 2) % 128,
			32,
			32,
			1,
			0,
			(gGameplayFrames * 2) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0B,
		Gfx_TwoTexScroll( 0, 0, 0, 32, 32, 1, 0, 127 - (gGameplayFrames * 3) % 128, 32, 32)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0C,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0D,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 64,
			16,
			16,
			1,
			0,
			(gGameplayFrames * 1) % 64,
			16,
			16
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_09(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			gGameplayFrames % 128,
			0,
			32,
			16,
			1,
			gGameplayFrames % 128,
			0,
			32,
			16
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			gGameplayFrames % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			gGameplayFrames % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	#if 0
		if ((gRoomUnk[0] == 0) && (INV_CONTENT(ITEM_COJIRO) == ITEM_COJIRO)) {
			if (gRoomUnk[1] == 50) {
				func_8002F7DC(&GET_PLAYER(globalCtx)->actor, NA_SE_EV_CHICKEN_CRY_M);
				gRoomUnk[0] = 1;
			}
			gRoomUnk[1]++;
		}
	#endif
}
void Scene_DrawConfig_10(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll( 0, 0, 0, 32, 32, 1, 0, 127 - gGameplayFrames % 128, 32, 32)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_11_GerudoFortress(Scene* scene) {
	#define gSpot12_009678Tex 0x02000000 | 0x9678
	#define gSpot12_00DE78Tex 0x02000000 | 0xDE78
	u32 D_8012A380[] = {
		gSpot12_009678Tex,
		gSpot12_00DE78Tex,
	};
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(D_8012A380[NIGHT_FLAG])
	);
}
void Scene_DrawConfig_12(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			gGameplayFrames % 128,
			32,
			32,
			1,
			0,
			gGameplayFrames % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			gGameplayFrames % 128,
			32,
			32,
			1,
			0,
			gGameplayFrames % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_13(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 10) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_14(Scene* scene) {
	Gfx* displayListHead = Graph_Alloc(3 * sizeof(Gfx));
	
	gSPSegment(POLY_XLU_DISP++, 0x08, displayListHead);
	
	#if 0
		if ((gSaveContext.dayTime >= 0x4AAC) && (gSaveContext.dayTime <= 0xC000)) {
			gSPEndDisplayList(displayListHead);
		} else {
			if (gSaveContext.dayTime > 0xC000) {
				if (gRoomUnk[0] != 255) {
					Math_StepToS(&gRoomUnk[0], 255, 5);
				}
			} else if (gSaveContext.dayTime >= 0x4000) {
				if (gRoomUnk[0] != 0) {
					Math_StepToS(&gRoomUnk[0], 0, 10);
				}
			}
			
			gDPSetPrimColor(displayListHead++, 0, 0, 255, 255, 255, gRoomUnk[0]);
			gSPDisplayList(displayListHead++, spot16_room_0DL_00AA48);
			gSPEndDisplayList(displayListHead);
		}
	#endif
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_15(Scene* scene) {
	s8 sp6F = coss((gGameplayFrames * 1500) & 0xFFFF) >> 8;
	s8 sp6E = coss((gGameplayFrames * 1500) & 0xFFFF) >> 8;
	
	sp6F = (sp6F >> 1) + 192;
	sp6E = (sp6E >> 1) + 192;
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			gGameplayFrames % 128,
			32,
			32,
			1,
			0,
			gGameplayFrames % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, sp6F, sp6E, 255, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_16_GoronCity(Scene* scene) {
	#define gGoronCityDayEntranceTex   0x02000000 | 0x9808
	#define gGoronCityNightEntranceTex 0x02000000 | 0x8FC8
	u32 sGoronCityEntranceTextures[] = {
		gGoronCityDayEntranceTex,
		gGoronCityNightEntranceTex,
	};
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			127 - gGameplayFrames % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			0,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sGoronCityEntranceTextures[NIGHT_FLAG]));
	
	{ s32 pad[2]; }
}
void Scene_DrawConfig_17_LonLonRanch(Scene* scene) {
	#define gLonLonRanchDayWindowTex    0x02000000 | 0x81E0
	#define gLonLonRangeNightWindowsTex 0x02000000 | 0xFBE0
	u32 sLonLonRanchWindowTextures[] = {
		gLonLonRanchDayWindowTex,
		gLonLonRangeNightWindowsTex,
	};
	
	gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sLonLonRanchWindowTextures[NIGHT_FLAG]));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_18(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			127 - gGameplayFrames % 128,
			32,
			32,
			1,
			127 - gGameplayFrames % 128,
			0,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			(gGameplayFrames * 3) % 128,
			127 - (gGameplayFrames * 6) % 128,
			32,
			32,
			1,
			(gGameplayFrames * 6) % 128,
			127 - (gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 64);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 64);
}
void Scene_DrawConfig_19(Scene* scene) {
	u32 D_8012A2F8[] = {
		0x0200BA18,
		0x0200CA18,
	};
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			0,
			127 - (gGameplayFrames % 128),
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(D_8012A2F8[NIGHT_FLAG]));
}
void Scene_DrawConfig_20(Scene* scene) {
	#define gDCDayEntranceTex   0x02000000 | 0x12378
	#define gDCNightEntranceTex 0x02000000 | 0x13378
	#define gDCLavaFloor1Tex    0x02000000 | 0x11F78
	#define gDCLavaFloor2Tex    0x02000000 | 0x14778
	#define gDCLavaFloor3Tex    0x02000000 | 0x14378
	#define gDCLavaFloor4Tex    0x02000000 | 0x13F78
	#define gDCLavaFloor5Tex    0x02000000 | 0x14B78
	#define gDCLavaFloor6Tex    0x02000000 | 0x13B78
	#define gDCLavaFloor7Tex    0x02000000 | 0x12F78
	#define gDCLavaFloor8Tex    0x02000000 | 0x12B78
	u32 gDCEntranceTextures[] = {
		gDCDayEntranceTex,
		gDCNightEntranceTex,
	};
	u32 sDCLavaFloorTextures[] = {
		gDCLavaFloor1Tex, gDCLavaFloor2Tex, gDCLavaFloor3Tex, gDCLavaFloor4Tex,
		gDCLavaFloor5Tex, gDCLavaFloor6Tex, gDCLavaFloor7Tex, gDCLavaFloor8Tex,
	};
	
	s32 pad;
	Gfx* displayListHead = Graph_Alloc(6 * sizeof(Gfx));
	
	gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(gDCEntranceTextures[NIGHT_FLAG]));
	gSPSegment(POLY_OPA_DISP++, 0x09, SEGMENTED_TO_VIRTUAL(sDCLavaFloorTextures[(s32)(gGameplayFrames & 14) >> 1]));
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			0,
			(gGameplayFrames * 1) % 256,
			0,
			64,
			32,
			1,
			0,
			(gGameplayFrames * 1) % 128,
			64,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			0,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			0,
			(gGameplayFrames * 2) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gSPSegment(POLY_OPA_DISP++, 0x0B, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 255, 255, 255, gRoomUnk[0]);
	gSPEndDisplayList(displayListHead++);
	gSPSegment(POLY_OPA_DISP++, 0x0C, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 255, 255, 255, gRoomUnk[1]);
	gSPEndDisplayList(displayListHead);
}
void Scene_DrawConfig_21(Scene* scene) {
	#if 0
		f32 D_8012A398 = 0.0f;
		static s16 D_8012A39C = 538;
		static s16 D_8012A3A0 = 4272;
		
		f32 temp;
		
		if (gSceneNum == SCENE_BDAN) {
			gSPSegment(
				POLY_OPA_DISP++,
				0x08,
				Gfx_TwoTexScroll(
					0,
					gGameplayFrames % 128,
					(gGameplayFrames * 2) % 128,
					32,
					32,
					1,
					127 - gGameplayFrames % 128,
					(gGameplayFrames * 2) % 128,
					32,
					32
				)
			);
			gSPSegment(
				POLY_OPA_DISP++,
				0x0B,
				Gfx_TwoTexScroll(
					0,
					0,
					255 - (gGameplayFrames * 4) % 256,
					32,
					64,
					1,
					0,
					255 - (gGameplayFrames * 4) % 256,
					32,
					64
				)
			);
		} else {
			gSPSegment(
				POLY_OPA_DISP++,
				0x08,
				Gfx_TexScroll(
					
					(127 - (gGameplayFrames * 1)) % 128,
					(gGameplayFrames * 1) % 128,
					32,
					32
				)
			);
		}
		
		gDPPipeSync(POLY_OPA_DISP++);
		gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
		
		gDPPipeSync(POLY_XLU_DISP++);
		gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
		
		if (FrameAdvance_IsEnabled(globalCtx) != true) {
			D_8012A39C += 1820;
			D_8012A3A0 += 1820;
			
			temp = 0.020000001f;
			func_800AA76C(
				&globalCtx->view,
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_CosS(D_8012A39C),
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_8012A39C),
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_8012A3A0)
			);
			func_800AA78C(
				&globalCtx->view,
				1.f + (0.79999995f * temp * Math_SinS(D_8012A3A0)),
				1.f + (0.39999998f * temp * Math_CosS(D_8012A3A0)),
				1.f + (1 * temp * Math_CosS(D_8012A39C))
			);
			func_800AA7AC(&globalCtx->view, 0.95f);
			
			switch (gRoomUnk[0]) {
				case 0:
					break;
				case 1:
					if (gRoomUnk[1] < 1200) {
						gRoomUnk[1] += 200;
					} else {
						gRoomUnk[0]++;
					}
					break;
				case 2:
					if (gRoomUnk[1] > 0) {
						gRoomUnk[1] -= 30;
					} else {
						gRoomUnk[1] = 0;
						gRoomUnk[0] = 0;
					}
					break;
			}
			
			D_8012A398 += 0.15f + (gRoomUnk[1] * 0.001f);
		}
		
		if (globalCtx->roomCtx.curRoom.num == 2) {
			Matrix_Scale(1.0f, sinf(D_8012A398) * 0.8f, 1.0f, MTXMODE_NEW);
		} else {
			Matrix_Scale(1.005f, sinf(D_8012A398) * 0.8f, 1.005f, MTXMODE_NEW);
		}
		
		gSPSegment(POLY_OPA_DISP++, 0x0D, Matrix_NewMtx( "../z_scene_table.c", 7809));
	#endif
}
void Scene_DrawConfig_22_ForestTemple(Scene* scene) {
	#define gForestTempleDayEntranceTex   0x02000000 | 0x14D90
	#define gForestTempleNightEntranceTex 0x02000000 | 0x14590
	u32 sForestTempleEntranceTextures[] = {
		gForestTempleDayEntranceTex,
		gForestTempleNightEntranceTex,
	};
	
	gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sForestTempleEntranceTextures[NIGHT_FLAG]));
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_23_WaterTemple(Scene* scene) {
	#define gWaterTempleDayEntranceTex   0x02000000
	#define gWaterTempleNightEntranceTex 0x02000000
	u32 D_8012A330[] = {
		gWaterTempleDayEntranceTex,
		gWaterTempleNightEntranceTex,
	};
	s32 spB0;
	s32 spAC;
	
	if (1) {
	} // Necessary to match
	
	spB0 = (gRoomUnk[1] >> 8) & 0xFF;
	spAC = gRoomUnk[1] & 0xFF;
	
	gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(D_8012A330[NIGHT_FLAG]));
	
	if (spB0 == 1) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x08,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				spAC
			)
		);
	} else if (spB0 < 1) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x08,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				255
			)
		);
	} else {
		gSPSegment(
			POLY_OPA_DISP++,
			0x08,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				160
			)
		);
	}
	
	if (spB0 == 2) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x09,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				spAC
			)
		);
	} else if (spB0 < 2) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x09,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				255
			)
		);
	} else {
		gSPSegment(
			POLY_OPA_DISP++,
			0x09,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				160
			)
		);
	}
	
	if (spB0 != 0) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x0A,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 1,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				160
			)
		);
		gSPSegment(
			POLY_OPA_DISP++,
			0x0B,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 3,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				180
			)
		);
	} else {
		gSPSegment(
			POLY_OPA_DISP++,
			0x0A,
			Gfx_TwoTexScrollEnvColor(
				0,
				(gGameplayFrames * 1) % 128,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				160 + (s32)((spAC / 200.0f) * 95.0f)
			)
		);
		gSPSegment(
			POLY_OPA_DISP++,
			0x0B,
			Gfx_TwoTexScrollEnvColor(
				0,
				gGameplayFrames * 3,
				0,
				32,
				32,
				1,
				0,
				0,
				32,
				32,
				0,
				0,
				0,
				185 + (s32)((spAC / 200.0f) * 70.0f)
			)
		);
	}
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x0C,
		Gfx_TwoTexScrollEnvColor(
			
			0,
			gGameplayFrames * 1,
			gGameplayFrames * 1,
			32,
			32,
			1,
			0,
			127 - (gGameplayFrames * 1),
			32,
			32,
			0,
			0,
			0,
			128
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0D,
		Gfx_TwoTexScrollEnvColor(
			
			0,
			gGameplayFrames * 4,
			0,
			32,
			32,
			1,
			gGameplayFrames * 4,
			0,
			32,
			32,
			0,
			0,
			0,
			128
		)
	);
	
	{ s32 pad[2]; }
}
void Scene_DrawConfig_24(Scene* scene) {
	if (gSceneNum == SCENE_HAKADAN_BS) {
		gSPSegment(
			POLY_OPA_DISP++,
			0x08,
			Gfx_TwoTexScroll(
				0,
				(gGameplayFrames * 2) % 128,
				0,
				32,
				32,
				1,
				(gGameplayFrames * 2) % 128,
				0,
				32,
				32
			)
		);
	} else {
		gSPSegment(
			POLY_XLU_DISP++,
			0x08,
			Gfx_TwoTexScroll(
				0,
				(gGameplayFrames * 2) % 128,
				0,
				32,
				32,
				1,
				(gGameplayFrames * 2) % 128,
				0,
				32,
				32
			)
		);
	}
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_25_SpiritTemple(Scene* scene) {
	#define gSpiritTempleDayEntranceTex   0x02000000 | 0x18920
	#define gSpiritTempleNightEntranceTex 0x02000000 | 0x18020
	u32 sSpiritTempleEntranceTextures[] = {
		gSpiritTempleDayEntranceTex,
		gSpiritTempleNightEntranceTex,
	};
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(sSpiritTempleEntranceTextures[NIGHT_FLAG])
	);
}
void Scene_DrawConfig_26(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 512,
			32,
			128,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 512,
			32,
			128
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_27(Scene* scene) {
	#define gGTGDayEntranceTex   0x02000000 | 0xF8C0
	#define gGTGNightEntranceTex 0x02000000 | 0x100C0
	u32 sGTGEntranceTextures[] = {
		gGTGDayEntranceTex,
		gGTGNightEntranceTex,
	};
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(sGTGEntranceTextures[NIGHT_FLAG])
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	{ s32 pad[2]; }
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_28(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			0,
			(gGameplayFrames * 2) % 256,
			0,
			64,
			32,
			1,
			0,
			(gGameplayFrames * 2) % 128,
			64,
			32
		)
	);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_29(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll( 0, gGameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, gRoomUnk[0]);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 145);
}
void Scene_DrawConfig_30(Scene* scene) {
	f32 temp;
	Gfx* displayListHead = Graph_Alloc(18 * sizeof(Gfx));
	
	temp = gRoomUnk[0] / 255.0f;
	
	gSPSegment(POLY_XLU_DISP++, 0x08, displayListHead);
	gSPSegment(POLY_OPA_DISP++, 0x08, displayListHead);
	gDPSetPrimColor(
		displayListHead++,
		0,
		0,
		255 - (u8)(185.0f * temp),
		255 - (u8)(145.0f * temp),
		255 - (u8)(105.0f * temp),
		255
	);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_XLU_DISP++, 0x09, displayListHead);
	gSPSegment(POLY_OPA_DISP++, 0x09, displayListHead);
	gDPSetPrimColor(
		displayListHead++,
		0,
		0,
		76 + (u8)(6.0f * temp),
		76 + (u8)(34.0f * temp),
		76 + (u8)(74.0f * temp),
		255
	);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_OPA_DISP++, 0x0A, displayListHead);
	gSPSegment(POLY_XLU_DISP++, 0x0A, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 0, 0, 0, gRoomUnk[0]);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_OPA_DISP++, 0x0B, displayListHead);
	gSPSegment(POLY_XLU_DISP++, 0x0B, displayListHead);
	gDPSetPrimColor(
		displayListHead++,
		0,
		0,
		89 + (u8)(166.0f * temp),
		89 + (u8)(166.0f * temp),
		89 + (u8)(166.0f * temp),
		255
	);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 0, 0, 0, gRoomUnk[0]);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_OPA_DISP++, 0x0C, displayListHead);
	gSPSegment(POLY_XLU_DISP++, 0x0C, displayListHead);
	gDPSetPrimColor(
		displayListHead++,
		0,
		0,
		255 + (u8)(179.0f * temp),
		255 + (u8)(179.0f * temp),
		255 + (u8)(179.0f * temp),
		255
	);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 0, 0, 0, gRoomUnk[0]);
	gSPEndDisplayList(displayListHead++);
	
	gSPSegment(POLY_OPA_DISP++, 0x0D, displayListHead);
	gSPSegment(POLY_XLU_DISP++, 0x0D, displayListHead);
	gDPPipeSync(displayListHead++);
	gDPSetEnvColor(displayListHead++, 0, 0, 0, gRoomUnk[1]);
	gSPEndDisplayList(displayListHead);
	
	#if 0
		if (gSaveContext.sceneSetupIndex == 5) {
			gCustomLensFlareOn = true;
			gCustomLensFlarePos.x = -20.0f;
			gCustomLensFlarePos.y = 1220.0f;
			gCustomLensFlarePos.z = -684.0f;
			D_8015FD06 = 10;
			D_8015FD08 = 8.0f;
			D_8015FD0C = 200;
		}
	#endif
}
void Scene_DrawConfig_31(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TexScroll(
			0,
			(gGameplayFrames * 1) % 64,
			256,
			16
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			0,
			127 - (gGameplayFrames % 128),
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			0,
			0,
			0,
			32,
			32,
			1,
			0,
			127 - (gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0B,
		Gfx_TexScroll(
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0C,
		Gfx_TwoTexScroll(
			0,
			0,
			(gGameplayFrames * 50) % 2048,
			8,
			512,
			1,
			0,
			(gGameplayFrames * 60) % 2048,
			8,
			512
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0D,
		Gfx_TwoTexScroll(
			0,
			0,
			0,
			32,
			64,
			1,
			0,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_32(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TexScroll(
			0,
			(gGameplayFrames * 2) % 256,
			64,
			64
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			0,
			127 - (gGameplayFrames * 1) % 128,
			(gGameplayFrames * 1) % 256,
			32,
			64,
			1,
			0,
			0,
			32,
			128
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_33(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 256,
			32,
			64,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 256,
			32,
			64
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_34(Scene* scene) {
	gSPSegment(POLY_OPA_DISP++, 0x08, Gfx_TexScroll( 0, gGameplayFrames % 64, 4, 16));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_35(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	
	if (gSceneNum == SCENE_HAIRAL_NIWA) {
		gSPSegment(
			POLY_XLU_DISP++,
			0x09,
			Gfx_TexScroll( 0, (gGameplayFrames * 10) % 256, 32, 64)
		);
	}
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_36(Scene* scene) {
	s8 sp83;
	
	if (1) {
	} // Necessary to match
	
	sp83 = coss((gGameplayFrames * 1500) & 0xFFFF) >> 8;
	
	if (gSceneNum == SCENE_GANON_TOU) {
		gSPSegment(
			POLY_XLU_DISP++,
			0x09,
			Gfx_TexScroll( 0, (gGameplayFrames * 1) % 256, 64, 64)
		);
		gSPSegment(
			POLY_XLU_DISP++,
			0x08,
			Gfx_TwoTexScroll(
				0,
				0,
				255 - (gGameplayFrames * 1) % 256,
				64,
				64,
				1,
				0,
				(gGameplayFrames * 1) % 256,
				64,
				64
			)
		);
	}
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x0B,
		Gfx_TwoTexScroll(
			
			0,
			255 - (gGameplayFrames * 1) % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			(gGameplayFrames * 1) % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	sp83 = (sp83 >> 1) + 192;
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, sp83, sp83, sp83, 128);
}
void Scene_DrawConfig_37(Scene* scene) {
	#define gIceCavernDayEntranceTex   0x02000000 | 0xFAC0
	#define gIceCavernNightEntranceTex 0x02000000 | 0xF8C0
	u32 sIceCavernEntranceTextures[] = {
		gIceCavernDayEntranceTex,
		gIceCavernNightEntranceTex,
	};
	
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(sIceCavernEntranceTextures[NIGHT_FLAG])
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_38(Scene* scene) {
	s8 sp7B;
	
	if (1) {
	} // Necessary to match
	
	sp7B = coss((gGameplayFrames * 1500) & 0xFFFF) >> 8;
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 512,
			64,
			128,
			1,
			0,
			511 - (gGameplayFrames * 1) % 512,
			64,
			128
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 1) % 256,
			32,
			64,
			1,
			0,
			255 - (gGameplayFrames * 1) % 256,
			32,
			64
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 20) % 2048,
			16,
			512,
			1,
			0,
			(gGameplayFrames * 30) % 2048,
			16,
			512
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	sp7B = (sp7B >> 1) + 192;
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, sp7B, sp7B, sp7B, 128);
	
	#if 0
		if (Flags_GetSwitch(globalCtx, 0x37)) {
			if ((gSceneNum == SCENE_GANON_DEMO) || (gSceneNum == SCENE_GANON_FINAL) ||
			    (gSceneNum == SCENE_GANON_SONOGO) || (gSceneNum == SCENE_GANONTIKA_SONOGO)) {
				// GanonShakeFunc(globalCtx);
			}
		}
	#endif
}
void Scene_DrawConfig_39(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 3) % 128,
			32,
			32
		)
	);
	gSPSegment(POLY_XLU_DISP++, 0x09, Gfx_TexScroll( 0, gGameplayFrames % 64, 256, 16));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_40(Scene* scene) {
	#define gThievesHideoutDayEntranceTex   0x02000000 | 0x0
	#define gThievesHideoutNightEntranceTex 0x02000000 | 0x0
	u32 sThievesHideoutEntranceTextures[] = {
		gThievesHideoutDayEntranceTex,
		gThievesHideoutNightEntranceTex,
	};
	
	gSPSegment(POLY_OPA_DISP++, 0x09, Gfx_TexScroll( 0, (gGameplayFrames * 3) % 128, 32, 32));
	
	{ s32 pad[2]; }
	
	gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sThievesHideoutEntranceTextures[NIGHT_FLAG]));
}
void Scene_DrawConfig_41(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TexScroll( 127 - (gGameplayFrames * 4) % 128, 0, 32, 32)
	);
	gSPSegment(POLY_OPA_DISP++, 0x09, Gfx_TexScroll( 0, (gGameplayFrames * 5) % 64, 16, 16));
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TexScroll( 0, 63 - (gGameplayFrames * 2) % 64, 16, 16)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0B,
		Gfx_TwoTexScroll( 0, 0, 127 - (gGameplayFrames * 3) % 128, 32, 32, 1, 0, 0, 32, 32)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_42(Scene* scene) {
	gSPSegment(POLY_XLU_DISP++, 0x08, Gfx_TexScroll( 0, (gGameplayFrames * 1) % 64, 256, 16));
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			(gGameplayFrames * 60) % 2048,
			8,
			512,
			1,
			0,
			(gGameplayFrames * 50) % 2048,
			8,
			512
		)
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - (gGameplayFrames * 1) % 128,
			0,
			32,
			32,
			1,
			(gGameplayFrames * 1) % 128,
			0,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0B,
		Gfx_TwoTexScroll(
			
			0,
			0,
			1023 - (gGameplayFrames * 6) % 1024,
			16,
			256,
			1,
			0,
			1023 - (gGameplayFrames * 3) % 1024,
			16,
			256
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_43(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TwoTexScroll( 0, 0, 0, 32, 32, 1, 0, (gGameplayFrames * 1) % 128, 32, 32)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x0A,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TexScroll( 0, 255 - (gGameplayFrames * 10) % 256, 32, 64)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_44_LonLonHouse(Scene* scene) {
	#define gLonLonHouseDayEntranceTex   0x02000000 | 0x5210
	#define gLonLonHouseNightEntranceTex 0x02000000 | 0x5010
	u32 sLonLonHouseEntranceTextures[] = {
		gLonLonHouseDayEntranceTex,
		gLonLonHouseNightEntranceTex,
	};
	
	gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sLonLonHouseEntranceTextures[NIGHT_FLAG]));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_45_GuardHouse(Scene* scene) {
	#define gGuardHouseOutSideView1DayTex   0x02000000 | 0x6550
	#define gGuardHouseOutSideView1NightTex 0x02000000 | 0x3550
	#define gGuardHouseOutSideView2DayTex   0x02000000 | 0x2350
	#define gGuardHouseOutSideView2NightTex 0x02000000 | 0x1350
	u32 sGuardHouseView2Textures[] = {
		gGuardHouseOutSideView1DayTex,
		gGuardHouseOutSideView1NightTex,
	};
	u32 sGuardHouseView1Textures[] = {
		gGuardHouseOutSideView2DayTex,
		gGuardHouseOutSideView2NightTex,
	};
	s32 var;
	
	// if (LINK_IS_ADULT) {
	// 	var = 1;
	// } else {
	var = NIGHT_FLAG;
	// }
	
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		SEGMENTED_TO_VIRTUAL(sGuardHouseView1Textures[var])
	);
	gSPSegment(
		POLY_OPA_DISP++,
		0x09,
		SEGMENTED_TO_VIRTUAL(sGuardHouseView2Textures[var])
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_46(Scene* scene) {
	gSPSegment(POLY_OPA_DISP++, 0x08, Gfx_TexScroll( 0, (gGameplayFrames * 3) % 128, 32, 32));
	gSPSegment(
		POLY_XLU_DISP++,
		0x09,
		Gfx_TwoTexScroll(
			
			0,
			0,
			1023 - (gGameplayFrames * 3) % 1024,
			16,
			256,
			1,
			0,
			1023 - (gGameplayFrames * 6) % 1024,
			16,
			256
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_47(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScroll(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_48(Scene* scene) {
	gSPSegment(POLY_XLU_DISP++, 0x08, Gfx_TexScroll( 0, gGameplayFrames % 64, 256, 16));
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_49(Scene* scene) {
	gSPSegment(
		POLY_OPA_DISP++,
		0x08,
		Gfx_TexScroll( 127 - (gGameplayFrames * 2) % 128, 0, 32, 64)
	);
	gSPSegment(POLY_OPA_DISP++, 0x09, Gfx_TexScroll( 0, (gGameplayFrames * 2) % 512, 128, 128));
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_50(Scene* scene) {
	gSPSegment(
		POLY_XLU_DISP++,
		0x08,
		Gfx_TwoTexScrollPrimColor(
			
			0,
			127 - gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			1,
			gGameplayFrames % 128,
			(gGameplayFrames * 1) % 128,
			32,
			32,
			255,
			255,
			255,
			gRoomUnk[0] + 127
		)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}
void Scene_DrawConfig_51(Scene* scene) {
	// GanonShakeFunc(globalCtx);
}
void Scene_DrawConfig_52(Scene* scene) {
	// GanonShakeFunc(globalCtx);
}

SceneDrawConf gSceneDrawConf[53] = {
	Scene_DrawConfig_00,                 Scene_DrawConfig_01,
	Scene_DrawConfig_02_KakarikoVillage, Scene_DrawConfig_03,
	Scene_DrawConfig_04,                 Scene_DrawConfig_05,
	Scene_DrawConfig_06_ZorasDomain,     Scene_DrawConfig_07,
	Scene_DrawConfig_08,                 Scene_DrawConfig_09,
	Scene_DrawConfig_10,                 Scene_DrawConfig_11_GerudoFortress,
	Scene_DrawConfig_12,                 Scene_DrawConfig_13,
	Scene_DrawConfig_14,                 Scene_DrawConfig_15,
	Scene_DrawConfig_16_GoronCity,       Scene_DrawConfig_17_LonLonRanch,
	Scene_DrawConfig_18,                 Scene_DrawConfig_19,
	Scene_DrawConfig_20,                 Scene_DrawConfig_21,
	Scene_DrawConfig_22_ForestTemple,    Scene_DrawConfig_23_WaterTemple,
	Scene_DrawConfig_24,                 Scene_DrawConfig_25_SpiritTemple,
	Scene_DrawConfig_26,                 Scene_DrawConfig_27,
	Scene_DrawConfig_28,                 Scene_DrawConfig_29,
	Scene_DrawConfig_30,                 Scene_DrawConfig_31,
	Scene_DrawConfig_32,                 Scene_DrawConfig_33,
	Scene_DrawConfig_34,                 Scene_DrawConfig_35,
	Scene_DrawConfig_36,                 Scene_DrawConfig_37,
	Scene_DrawConfig_38,                 Scene_DrawConfig_39,
	Scene_DrawConfig_40,                 Scene_DrawConfig_41,
	Scene_DrawConfig_42,                 Scene_DrawConfig_43,
	Scene_DrawConfig_44_LonLonHouse,     Scene_DrawConfig_45_GuardHouse,
	Scene_DrawConfig_46,                 Scene_DrawConfig_47,
	Scene_DrawConfig_48,                 Scene_DrawConfig_49,
	Scene_DrawConfig_50,                 Scene_DrawConfig_51,
	Scene_DrawConfig_52,
};
