/*
 * n64.h <z64.me>
 *
 * simple HLE (high level emulation) N64 rendering engine
 *
 */

#ifndef Z64_N64_H_INCLUDED
#define Z64_N64_H_INCLUDED

enum n64_zmode {
	ZMODE_OPA,
	ZMODE_INTER,
	ZMODE_XLU,
	ZMODE_DEC,
	ZMODE_ALL
};

enum n64_geoLayer {
	GEOLAYER_ALL,
	GEOLAYER_OPAQUE,
	GEOLAYER_OVERLAY
};

void n64_set_segment(int seg, void* data);
void* n64_virt2phys(unsigned int segaddr);
unsigned int n64_phys2virt(void* cmd);
void n64_draw(void* dlist);
void n64_set_onlyZmode(enum n64_zmode zmode);
void n64_set_onlyGeoLayer(enum n64_geoLayer geoLayer);

void n64_setMatrix_model(void* data);
void n64_setMatrix_view(void* data);
void n64_setMatrix_projection(void* data);

void n64_set_fog(float fog[2], float color[3]);
void n64_set_lights(float lights[16]);

void n64_clearShaderCache(void);

#define gSPMatrix(mtx)            n64_setMatrix_model(mtx)
#define gSPDisplayList(dl)        n64_draw(n64_virt2phys(dl))
#define gSPSegment(sed, data)     n64_set_segment(sed, data)
#define SEGMENTED_TO_VIRTUAL(seg) n64_virt2phys(seg)

#endif
