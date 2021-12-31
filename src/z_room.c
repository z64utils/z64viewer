#include <z_room.h>
#include <bigendian.h>

void Room_Draw(Room* room) {
	s8 type = room->mesh->polygon.type;
	
	gDPSetEnvColor(OpaNow++, 0x80, 0x80, 0x80, 0x80);
	
	gxSPSegment(0x03, room->file.data);
	n64_set_onlyZmode(ZMODE_ALL);
	n64_set_onlyGeoLayer(GEOLAYER_ALL);
	
	for (s32 z = 0; z < 2; z++) {
		switch (type) {
		    case 0: {
			    PolygonDlist0* polyDL = SEGMENTED_TO_VIRTUAL(
				    ReadBE(room->mesh->polygon.start)
			    );
			    
			    for (s32 i = 0; i < room->mesh->polygon.num; i++) {
				    if (z == 0 && polyDL->opa) {
					    gxSPDisplayListSeg(ReadBE(polyDL->opa));
				    }
				    else if (z == 1 && polyDL->xlu) {
					    gxSPDisplayListSeg(ReadBE(polyDL->xlu));
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
					    gxSPDisplayListSeg(ReadBE(polyDL->opa));
				    }
				    else if (z == 1 && polyDL->xlu) {
					    gxSPDisplayListSeg(ReadBE(polyDL->xlu));
				    }
				    
				    polyDL++;
			    }
			    break;
		    }
		}
	}
}
