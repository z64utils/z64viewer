#include <z_room.h>

static s32 printSetup = 2;

void Room_Draw(Room* room) {
	s8 type = room->mesh->polygon.type;
	
	#ifndef NDEBUG
	if (Decr(printSetup) == 1) {
		for (s32 i = 0; i < 6; i++) {
			u32* ss = (u32*)&gSetupDL[0x19][i];
			if (i == 0) {
				OsPrintfEx("Room gSetupDL OPA:");
			}
			OsPrintf("%08X %08X", ReadBE(ss[0]), ReadBE(ss[i]));
		}
	}
	#endif
	
	gSegment[3] = room->file.data;
	gSPSegment(POLY_OPA_DISP++, 0x3, room->file.data);
	n64_set_onlyZmode(ZMODE_ALL);
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	gSPDisplayList(POLY_OPA_DISP++, gSetupDL[0x19]);
	gDPSetEnvColor(POLY_OPA_DISP++, 0x80, 0x80, 0x80, 0x80);
	
	for (s32 z = 0; z < 2; z++) {
		switch (type) {
		    case 0: {
			    PolygonDlist0* polyDL = SEGMENTED_TO_VIRTUAL(
				    ReadBE(room->mesh->polygon.start)
			    );
			    
			    for (s32 i = 0; i < room->mesh->polygon.num; i++) {
				    if (z == 0 && polyDL->opa) {
					    gSPDisplayList(POLY_OPA_DISP++, ReadBE(polyDL->opa));
				    } else if (z == 1 && polyDL->xlu) {
					    gSPDisplayList(POLY_OPA_DISP++, ReadBE(polyDL->xlu));
				    }
				    
				    polyDL++;
			    }
			    
			    break;
		    }
		    case 2: {
			    PolygonDlist2* polyDL = SEGMENTED_TO_VIRTUAL(
				    ReadBE(room->mesh->polygon.start)
			    );
			    
			    for (s32 i = 0; i < room->mesh->polygon.num; i++) {
				    if (z == 0 && polyDL->opa) {
					    gSPDisplayList(POLY_OPA_DISP++, ReadBE(polyDL->opa));
				    } else if (z == 1 && polyDL->xlu) {
					    gSPDisplayList(POLY_OPA_DISP++, ReadBE(polyDL->xlu));
				    }
				    
				    polyDL++;
			    }
			    break;
		    }
		}
	}
}
