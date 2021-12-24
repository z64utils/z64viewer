#include <z_room.h>
#include <bigendian.h>

void Room_Draw(Room* room) {
	u8 setup[16] = {
		0xfb, 0, 0, 0, 0x80, 0x80, 0x80, 0x80,
		0xdf
	};
	s8 type = room->mesh->polygon.type;
	
	gSPSegment(0x03, room->file.data);
	n64_draw(setup);
	
	for (s32 z = 0; z < 2; z++) {
		if (z == 0) {
			n64_set_onlyGeoLayer(GEOLAYER_OPAQUE);
		} else {
			n64_set_onlyGeoLayer(GEOLAYER_OVERLAY);
		}
		
		switch (type) {
		    case 0: {
			    PolygonDlist0* polyDL = SEGMENTED_TO_VIRTUAL(
				    ReadBE(room->mesh->polygon.start)
			    );
			    
			    for (s32 i = 0; i < room->mesh->polygon.num; i++) {
				    if (polyDL->opa) {
					    gSPDisplayList(ReadBE(polyDL->opa));
				    }
				    if (polyDL->xlu) {
					    gSPDisplayList(ReadBE(polyDL->xlu));
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
				    if (polyDL->opa) {
					    gSPDisplayList(ReadBE(polyDL->opa));
				    }
				    if (polyDL->xlu) {
					    gSPDisplayList(ReadBE(polyDL->xlu));
				    }
				    
				    polyDL++;
			    }
			    break;
		    }
		}
	}
}
