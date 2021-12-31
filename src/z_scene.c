#include <z_scene.h>

// Spawn Player
void Scene_LoadCmd_0x00(Scene* scene, Room* room, SceneCmd* cmd) {
}
// Actor List
void Scene_LoadCmd_0x01(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->numSetupActors = cmd->actorList.num;
	globalCtx->setupActorList = SEGMENTED_TO_VIRTUAL(cmd->actorList.segment);
	#endif
}
// Unused 02
void Scene_LoadCmd_0x02(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->unk_11DFC = SEGMENTED_TO_VIRTUAL(cmd->unused02.segment);
	#endif
}
// Collision Header
void Scene_LoadCmd_0x03(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	CollisionHeader* colHeader = SEGMENTED_TO_VIRTUAL(cmd->colHeader.segment);
	
	colHeader->vtxList = SEGMENTED_TO_VIRTUAL(colHeader->vtxList);
	colHeader->polyList = SEGMENTED_TO_VIRTUAL(colHeader->polyList);
	colHeader->surfaceTypeList = SEGMENTED_TO_VIRTUAL(colHeader->surfaceTypeList);
	colHeader->cameraDataList = SEGMENTED_TO_VIRTUAL(colHeader->cameraDataList);
	colHeader->waterBoxes = SEGMENTED_TO_VIRTUAL(colHeader->waterBoxes);
	
	BgCheck_Allocate(&globalCtx->colCtx, globalCtx, colHeader);
	#endif
}
// Room List
void Scene_LoadCmd_0x04(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->numRooms = cmd->roomList.num;
	globalCtx->roomList = SEGMENTED_TO_VIRTUAL(cmd->roomList.segment);
	#endif
}
// Wind Settings
void Scene_LoadCmd_0x05(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	s8 x = cmd->windSettings.x;
	s8 y = cmd->windSettings.y;
	s8 z = cmd->windSettings.z;
	
	globalCtx->envCtx.windDirection.x = x;
	globalCtx->envCtx.windDirection.y = y;
	globalCtx->envCtx.windDirection.z = z;
	
	globalCtx->envCtx.windSpeed = cmd->windSettings.unk_07;
	#endif
}
// Entrance List
void Scene_LoadCmd_0x06(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->setupEntranceList = SEGMENTED_TO_VIRTUAL(cmd->entranceList.segment);
	#endif
}
// Special Files
void Scene_LoadCmd_0x07(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	if (cmd->specialFiles.keepObjectId != 0) {
		globalCtx->objectCtx.subKeepIndex = Object_Spawn(&globalCtx->objectCtx, cmd->specialFiles.keepObjectId);
		gSegments[5] = VIRTUAL_TO_PHYSICAL(globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	}
	
	if (cmd->specialFiles.cUpElfMsgNum != 0) {
		globalCtx->cUpElfMsgs = Gameplay_LoadFile(globalCtx, &sNaviMsgFiles[cmd->specialFiles.cUpElfMsgNum - 1]);
	}
	#endif
}
// Room Behavior
void Scene_LoadCmd_0x08(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->roomCtx.curRoom.unk_03 = cmd->roomBehavior.gpFlag1;
	globalCtx->roomCtx.curRoom.unk_02 = cmd->roomBehavior.gpFlag2 & 0xFF;
	globalCtx->roomCtx.curRoom.showInvisActors = (cmd->roomBehavior.gpFlag2 >> 8) & 1;
	globalCtx->msgCtx.disableWarpSongs = (cmd->roomBehavior.gpFlag2 >> 0xA) & 1;
	#endif
}
// Undefined
void Scene_LoadCmd_0x09(Scene* scene, Room* room, SceneCmd* cmd) {
}
// Mesh Header
void Scene_LoadCmd_0x0A(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->roomCtx.curRoom.mesh = SEGMENTED_TO_VIRTUAL(cmd->mesh.segment);
	#endif
	
	room->mesh = SEGMENTED_TO_VIRTUAL(ReadBE(cmd->mesh.segment));
}
// Object List
void Scene_LoadCmd_0x0B(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	s32 i;
	s32 j;
	s32 k;
	ObjectStatus* status;
	ObjectStatus* status2;
	ObjectStatus* firstStatus;
	s16* objectEntry = SEGMENTED_TO_VIRTUAL(cmd->objectList.segment);
	void* nextPtr;
	
	k = 0;
	i = globalCtx->objectCtx.unk_09;
	firstStatus = &globalCtx->objectCtx.status[0];
	status = &globalCtx->objectCtx.status[i];
	
	while (i < globalCtx->objectCtx.num) {
		if (status->id != *objectEntry) {
			status2 = &globalCtx->objectCtx.status[i];
			for (j = i; j < globalCtx->objectCtx.num; j++) {
				status2->id = OBJECT_INVALID;
				status2++;
			}
			globalCtx->objectCtx.num = i;
			func_80031A28(globalCtx, &globalCtx->actorCtx);
			
			continue;
		}
		
		i++;
		k++;
		objectEntry++;
		status++;
	}
	
	ASSERT(
		cmd->objectList.num <= OBJECT_EXCHANGE_BANK_MAX,
		"scene_info->object_bank.num <= OBJECT_EXCHANGE_BANK_MAX",
		"../z_scene.c",
		705
	);
	
	if (1) {
	}
	
	while (k < cmd->objectList.num) {
		nextPtr = func_800982FC(&globalCtx->objectCtx, i, *objectEntry);
		if (i < OBJECT_EXCHANGE_BANK_MAX - 1) {
			firstStatus[i + 1].segment = nextPtr;
		}
		i++;
		k++;
		objectEntry++;
	}
	
	globalCtx->objectCtx.num = i;
	#endif
}
// Light List
void Scene_LoadCmd_0x0C(Scene* scene, Room* room, SceneCmd* cmd) {
	OsAssert(scene != NULL && room != NULL);
	scene->lightCtx.room[room->num].lightNum = cmd->lightList.num;
	scene->lightCtx.room[room->num].lightList =
	    SEGMENTED_TO_VIRTUAL(ReadBE(cmd->lightList.segment));
}
// Path List
void Scene_LoadCmd_0x0D(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->setupPathList = SEGMENTED_TO_VIRTUAL(cmd->pathList.segment);
	#endif
}
// Transition Actor List
void Scene_LoadCmd_0x0E(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->transiActorCtx.numActors = cmd->transiActorList.num;
	globalCtx->transiActorCtx.list = SEGMENTED_TO_VIRTUAL(cmd->transiActorList.segment);
	#endif
}
// Light Setting List
void Scene_LoadCmd_0x0F(Scene* scene, Room* room, SceneCmd* cmd) {
	OsAssert(scene != NULL);
	scene->lightCtx.envLight = SEGMENTED_TO_VIRTUAL(ReadBE(cmd->lightSettingList.segment));
	scene->lightCtx.envListNum = cmd->lightSettingList.num;
}
// Skybox Settings
void Scene_LoadCmd_0x11(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->skyboxId = cmd->skyboxSettings.skyboxId;
	globalCtx->envCtx.unk_17 = globalCtx->envCtx.unk_18 = cmd->skyboxSettings.unk_05;
	globalCtx->envCtx.indoors = cmd->skyboxSettings.unk_06;
	#endif
	
	if (room) {
		room->inDoorLights = cmd->skyboxSettings.unk_06;
	}
}
// Skybox Disables
void Scene_LoadCmd_0x12(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->envCtx.skyboxDisabled = cmd->skyboxDisables.unk_04;
	globalCtx->envCtx.sunMoonDisabled = cmd->skyboxDisables.unk_05;
	#endif
}
// Time Settings
void Scene_LoadCmd_0x10(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	if ((cmd->timeSettings.hour != 0xFF) && (cmd->timeSettings.min != 0xFF)) {
		gSaveContext.skyboxTime = gSaveContext.dayTime =
		    ((cmd->timeSettings.hour + (cmd->timeSettings.min / 60.0f)) * 60.0f) / ((f32)(24 * 60) / 0x10000);
	}
	
	if (cmd->timeSettings.unk_06 != 0xFF) {
		globalCtx->envCtx.timeIncrement = cmd->timeSettings.unk_06;
	} else {
		globalCtx->envCtx.timeIncrement = 0;
	}
	
	if (gSaveContext.sunsSongState == SUNSSONG_INACTIVE) {
		gTimeIncrement = globalCtx->envCtx.timeIncrement;
	}
	
	globalCtx->envCtx.sunPos.x = -(Math_SinS(((void)0, gSaveContext.dayTime) - 0x8000) * 120.0f) * 25.0f;
	globalCtx->envCtx.sunPos.y = (Math_CosS(((void)0, gSaveContext.dayTime) - 0x8000) * 120.0f) * 25.0f;
	globalCtx->envCtx.sunPos.z = (Math_CosS(((void)0, gSaveContext.dayTime) - 0x8000) * 20.0f) * 25.0f;
	
	if (((globalCtx->envCtx.timeIncrement == 0) && (gSaveContext.cutsceneIndex < 0xFFF0)) ||
	    (gSaveContext.entranceIndex == 0x0604)) {
		gSaveContext.skyboxTime = ((void)0, gSaveContext.dayTime);
		if ((gSaveContext.skyboxTime >= 0x2AAC) && (gSaveContext.skyboxTime < 0x4555)) {
			gSaveContext.skyboxTime = 0x3556;
		} else if ((gSaveContext.skyboxTime >= 0x4555) && (gSaveContext.skyboxTime < 0x5556)) {
			gSaveContext.skyboxTime = 0x5556;
		} else if ((gSaveContext.skyboxTime >= 0xAAAB) && (gSaveContext.skyboxTime < 0xB556)) {
			gSaveContext.skyboxTime = 0xB556;
		} else if ((gSaveContext.skyboxTime >= 0xC001) && (gSaveContext.skyboxTime < 0xCAAC)) {
			gSaveContext.skyboxTime = 0xCAAC;
		}
	}
	#endif
}
// Exit List
void Scene_LoadCmd_0x13(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->setupExitList = SEGMENTED_TO_VIRTUAL(cmd->exitList.segment);
	#endif
}
// Sound Settings
void Scene_LoadCmd_0x15(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->soundCtx.seqIndex = cmd->soundSettings.seqIndex;
	globalCtx->soundCtx.nightSeqIndex = cmd->soundSettings.nightSeqIndex;
	
	if (gSaveContext.seqIndex == (u8)NA_BGM_DISABLED) {
		Audio_QueueSeqCmd(cmd->soundSettings.bgmId | 0xF0000000);
	}
	#endif
}
// Echo Setting
void Scene_LoadCmd_0x16(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	globalCtx->roomCtx.curRoom.echo = cmd->echoSettings.echo;
	#endif
}
// Cutscene Data
void Scene_LoadCmd_0x17(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	osSyncPrintf("\ngame_play->demo_play.data=[%x]", globalCtx->csCtx.segment);
	globalCtx->csCtx.segment = SEGMENTED_TO_VIRTUAL(cmd->cutsceneData.segment);
	#endif
}
// Alternate Headers
void Scene_LoadCmd_0x18(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	s32 pad;
	SceneCmd* altHeader;
	
	osSyncPrintf("\n[ZU]sceneset age    =[%X]", ((void)0, gSaveContext.linkAge));
	osSyncPrintf("\n[ZU]sceneset time   =[%X]", ((void)0, gSaveContext.cutsceneIndex));
	osSyncPrintf("\n[ZU]sceneset counter=[%X]", ((void)0, gSaveContext.sceneSetupIndex));
	
	if (gSaveContext.sceneSetupIndex != 0) {
		altHeader = ((SceneCmd**)SEGMENTED_TO_VIRTUAL(cmd->altHeaders.segment))[gSaveContext.sceneSetupIndex - 1];
		
		if (1) {
		}
		
		if (altHeader != NULL) {
			Scene_ExecuteCommands(globalCtx, SEGMENTED_TO_VIRTUAL(altHeader));
			(cmd + 1)->base.code = 0x14;
		} else {
			// "Coughh! There is no specified dataaaaa!"
			osSyncPrintf("\nげぼはっ！ 指定されたデータがないでええっす！");
			
			if (gSaveContext.sceneSetupIndex == 3) {
				altHeader =
				    ((SceneCmd**)SEGMENTED_TO_VIRTUAL(cmd->altHeaders.segment))[gSaveContext.sceneSetupIndex - 2];
				
				// "Using adult day data there!"
				osSyncPrintf("\nそこで、大人の昼データを使用するでええっす！！");
				
				if (altHeader != NULL) {
					Scene_ExecuteCommands(globalCtx, SEGMENTED_TO_VIRTUAL(altHeader));
					(cmd + 1)->base.code = 0x14;
				}
			}
		}
	}
	#endif
}
// Misc. Settings (Camera & World Map Area)
void Scene_LoadCmd_0x19(Scene* scene, Room* room, SceneCmd* cmd) {
	#if 0
	YREG(15) = cmd->miscSettings.cameraMovement;
	gSaveContext.worldMapArea = cmd->miscSettings.area;
	
	if ((globalCtx->sceneNum == SCENE_SHOP1) || (globalCtx->sceneNum == SCENE_SYATEKIJYOU)) {
		if (LINK_AGE_IN_YEARS == YEARS_ADULT) {
			gSaveContext.worldMapArea = 1;
		}
	}
	
	if (((globalCtx->sceneNum >= SCENE_SPOT00) && (globalCtx->sceneNum <= SCENE_GANON_TOU)) ||
	    ((globalCtx->sceneNum >= SCENE_ENTRA) && (globalCtx->sceneNum <= SCENE_SHRINE_R))) {
		if (gSaveContext.cutsceneIndex < 0xFFF0) {
			gSaveContext.worldMapAreaData |= gBitFlags[gSaveContext.worldMapArea];
			osSyncPrintf(
				"０００  ａｒｅａ＿ａｒｒｉｖａｌ＝%x (%d)\n",
				gSaveContext.worldMapAreaData,
				gSaveContext.worldMapArea
			);
		}
	}
	#endif
}

SceneCmdFunc sLoadCmdFuncTable[] = {
	Scene_LoadCmd_0x00,
	Scene_LoadCmd_0x01,
	Scene_LoadCmd_0x02,
	Scene_LoadCmd_0x03,
	Scene_LoadCmd_0x04,
	Scene_LoadCmd_0x05,
	Scene_LoadCmd_0x06,
	Scene_LoadCmd_0x07,
	Scene_LoadCmd_0x08,
	Scene_LoadCmd_0x09,
	Scene_LoadCmd_0x0A,
	Scene_LoadCmd_0x0B,
	Scene_LoadCmd_0x0C,
	Scene_LoadCmd_0x0D,
	Scene_LoadCmd_0x0E,
	Scene_LoadCmd_0x0F,
	Scene_LoadCmd_0x10,
	Scene_LoadCmd_0x11,
	Scene_LoadCmd_0x12,
	Scene_LoadCmd_0x13,
	NULL,
	Scene_LoadCmd_0x15,
	Scene_LoadCmd_0x16,
	Scene_LoadCmd_0x17,
	Scene_LoadCmd_0x18,
	Scene_LoadCmd_0x19
};

char* sColorPrint[] = {
	PRNT_REDD,
	PRNT_GREN,
	PRNT_YELW,
	PRNT_BLUE,
	PRNT_PRPL,
	PRNT_CYAN,
};

char* sCommandNameTable[] = {
	"Spawn Player",
	"Actor List",
	"Unused 02",
	"Collision Header",
	"Room List",
	"Wind Settings",
	"Entrance List",
	"Special Files",
	"Room Behavior",
	"Undefined",
	"Mesh Header",
	"Object List",
	"Light List",
	"Path List",
	"Transition Actor List",
	"Light Setting List",
	"Skybox Settings",
	"Skybox Disables",
	"Time Settings",
	"Exit List",
	"Break",
	"Sound Settings",
	"Echo Setting",
	"Cutscene Data",
	"Alternate Headers",
	"Misc. Settings",
};

void Scene_DebugOsPrintf(u8 code, u8* flag) {
	static u8 color = 0;
	
	if (code != 0x14) {
		color = Wrap(color + 1, 1, 5);
	} else {
		color = 0;
	}
	
	if (!*flag) {
		OsPrintfEx("%s%-30s 0x%02X" PRNT_RSET, sColorPrint[color], sCommandNameTable[code], code);
		(*flag)++;
	} else {
		OsPrintf("%s%-30s 0x%02X" PRNT_RSET, sColorPrint[color], sCommandNameTable[code], code);
	}
}

void Scene_ExecuteCommands(Scene* scene, Room* room) {
	u8 cmdCode;
	u8 printFlag = 0;
	
	// Process scene commands separately
	if (room == NULL) {
		OsPrintfEx("Executing Scene Commands");
		SceneCmd* sceneCmd = scene->file.data;
		
		n64_set_segment(0x2, scene->file.data);
		
		while (1) {
			cmdCode = sceneCmd->base.code;
			OsAssert(cmdCode <= 0x19);
			Scene_DebugOsPrintf(cmdCode, &printFlag);
			if (cmdCode == 0x14) {
				break;
			}
			sLoadCmdFuncTable[cmdCode](scene, room, sceneCmd);
			sceneCmd++;
		}
	} else {
		OsPrintfEx("Executing Room Commands");
		SceneCmd* sceneCmd = room->file.data;
		
		n64_set_segment(0x3, room->file.data);
		
		while (1) {
			cmdCode = sceneCmd->base.code;
			OsAssert(cmdCode <= 0x19);
			Scene_DebugOsPrintf(cmdCode, &printFlag);
			if (cmdCode == 0x14) {
				break;
			}
			sLoadCmdFuncTable[cmdCode](scene, room, sceneCmd);
			sceneCmd++;
		}
	}
}
