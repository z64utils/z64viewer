#include <z_room.h>

static s32 printSetup = 2;

void Room_Draw(Room* room) {
	s8 type = room->mesh->polygon.type;
	
	n64_set_onlyZmode(ZMODE_ALL);
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	gxSPSegment(POLY_OPA_DISP++, 0x3, room->file.data);
	gSPDisplayList(POLY_OPA_DISP++, gSetupDList(0x19));
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
