#include "n64.h"
#include "n64types.h"

static Object s_object_list[OBJ_REG_A_SIZE][OBJ_REG_B_SIZE];
static void* s_prev_seg;
static int s_prev_seg_id;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void segment_swap_to(int seg, const void* data) {
	s_prev_seg = n64_segment[seg];
	s_prev_seg_id = seg;
	n64_segment[seg] = (void*)data;
}

static void segment_swap_back(void) {
	n64_segment[s_prev_seg_id] = s_prev_seg;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static Object* get_obj(uint16_t obj_id, uint8_t uniq_id) {
	assert(obj_id < OBJ_REG_A_SIZE);
	assert(uniq_id < OBJ_REG_B_SIZE);
	
	return &s_object_list[obj_id][uniq_id];
}

void n64_register_skeleton(uint16_t obj_id, uint8_t uniq_id, const void* data, uint8_t seg_id, uint32_t skel_offset) {
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint32_t list_segment;
		uint8_t  limb_num;
	} ZSkelHead;
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint32_t segment;
	} ZSegList;
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t  pos_x, pos_y, pos_z;
		uint8_t  child;
		uint8_t  sibling;
		uint32_t dlist;
	} ZLimb;
	
	Object* obj = get_obj(obj_id, uniq_id);
	ZSkelHead* header;
	ZSegList* list;
	
	assert(obj->type == OBJ_TYPE_NONE);
	obj->type = OBJ_TYPE_SKELETON;
	obj->data = data;
	obj->seg_id = seg_id;
	
	segment_swap_to(seg_id, obj->data);
	header = n64_segment_get(skel_offset);
	list = n64_segment_get(header->list_segment);
	
	obj->limb_num = header->limb_num;
	assert((obj->limb = calloc(obj->limb_num, sizeof(Limb))) != NULL);
	
	for (uint32_t i = 0; i < obj->limb_num; i++) {
		ZLimb* limb = n64_segment_get(list[i].segment);
		
		if (!limb) continue;
		
		obj->limb[i] = (Limb) {
			.pos = { limb->pos_x, limb->pos_y, limb->pos_z },
			.child = limb->child,
			.sibling = limb->sibling,
			.dlist = limb->dlist,
		};
	}
	
	segment_swap_back();
}

void n64_register_dlist(uint16_t obj_id, uint8_t uniq_id, const void* data, uint8_t seg_id, GbiGfx* dlist, int dlist_num) {
	Object* obj = get_obj(obj_id, uniq_id);
	
	assert(obj->type == OBJ_TYPE_NONE);
	
	obj->type = OBJ_TYPE_DLIST;
	obj->data = data;
	obj->seg_id = seg_id;
	assert((obj->dlist = calloc(dlist_num, sizeof(GbiGfx))) != NULL);
	obj->dlist_num = dlist_num;
	
	memcpy(obj->dlist, dlist, sizeof(GbiGfx[dlist_num]));
}

void n64_unregister(uint16_t obj_id, uint8_t uniq_id) {
	Object* obj = get_obj(obj_id, uniq_id);
	
	if (obj->type == OBJ_TYPE_NONE)
		return;
	
	// free limb/dlist
	if (obj->limb)
		free(obj->limb);
	*obj = (Object) {};
}

N64Object* n64_object_new(uint16_t obj_id, uint8_t uniq_id) {
	N64Object* obj = calloc(1, sizeof(N64Object));
	
	obj->obj_id = obj_id;
	obj->uniq_id = uniq_id;
	obj->mtl_setup_dl_id = 0x25;
	
	return obj;
}

static void n64anim_destroy(Anim** self) {
	if (*self) {
		free((*self)->frame_tbl);
		free(*self);
	}
	*self = NULL;
}

void n64_object_destroy(N64Object* self) {
	n64anim_destroy(&self->anim);
}

static void update_anim(N64Object* self, Object* obj) {
	Anim* anim = self->anim;
	int id_cur_frame = floor(anim->cur);
	float mod = anim->cur - id_cur_frame;
	int id_next_frame = (id_cur_frame + 1) % anim->end;
	
	N64Quat* frame = &anim->frame_tbl[id_cur_frame * obj->limb_num];
	N64Quat* next_frame = &anim->frame_tbl[id_next_frame * obj->limb_num];
	
	anim->root_pos.x = lerp(mod, frame[0].x, next_frame[0].x);
	anim->root_pos.y = lerp(mod, frame[0].y, next_frame[0].y);
	anim->root_pos.z = lerp(mod, frame[0].z, next_frame[0].z);
	
	for (uint32_t i = 1; i < obj->limb_num; i++)
		anim->limb_rot[i - 1] = quat_to_vec3(quat_lerp(mod, frame[i], next_frame[i]));
	
	anim->cur = fmod(anim->cur + anim->speed, anim->end);
}

void n64_object_set_anim(N64Object* self, uint32_t seg_anim, float speed) {
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t binang;
	} ZFrame;
	typedef struct N64_ATTR_BIG_ENDIAN {
		uint16_t x, y, z;
	} ZJointID;
	typedef struct N64_ATTR_BIG_ENDIAN {
		int16_t  frame_num;
		uint32_t seg_tbl;
		uint32_t seg_jnt_id_tbl;
		uint16_t max;
	} ZAnimHead;
	
	#define BIN_TO_RAD(binang) ((float)((int32_t)binang) * (3.14159265359 / 0x8000))
	Object* obj = get_obj(self->obj_id, self->uniq_id);
	
	assert(obj->type == OBJ_TYPE_SKELETON);
	
	segment_swap_to(obj->seg_id, obj->data);
	n64anim_destroy(&self->anim);
	
	ZAnimHead* header = n64_segment_get(seg_anim);
	ZFrame* frames = n64_segment_get(header->seg_tbl);
	Anim* anim;
	N64Quat* dst;
	
	assert((anim = self->anim = calloc(1, sizeof(Anim))) != NULL);
	
	anim->speed = speed;
	anim->frame_num = anim->end = header->frame_num;
	
	assert((dst = anim->frame_tbl = calloc(anim->frame_num * obj->limb_num, sizeof(N64Quat))) != NULL);
	assert((anim->limb_rot = calloc(obj->limb_num, sizeof(N64Vector3))) != NULL);
	
	for (uint32_t k = 0; k < anim->frame_num; k++) {
		uint16_t static_max = header->max;
		ZFrame* fstatic = frames;
		ZFrame* fdynamic = &frames[k];
		ZJointID* jnt_id = n64_segment_get(header->seg_jnt_id_tbl);
		
		*dst = (N64Quat) {
			.x = (jnt_id->x >= static_max) ? fdynamic[jnt_id->x].binang : fstatic[jnt_id->x].binang,
			.y = (jnt_id->y >= static_max) ? fdynamic[jnt_id->y].binang : fstatic[jnt_id->y].binang,
			.z = (jnt_id->z >= static_max) ? fdynamic[jnt_id->z].binang : fstatic[jnt_id->z].binang,
		};
		dst++;
		jnt_id++;
		
		for (uint32_t i = 1; i < obj->limb_num; i++, dst++, jnt_id++)
			*dst = vec3_to_quat((N64Vector3) {
				.x = BIN_TO_RAD( (jnt_id->x >= static_max) ? fdynamic[jnt_id->x].binang : fstatic[jnt_id->x].binang ),
				.y = BIN_TO_RAD( (jnt_id->y >= static_max) ? fdynamic[jnt_id->y].binang : fstatic[jnt_id->y].binang ),
				.z = BIN_TO_RAD( (jnt_id->z >= static_max) ? fdynamic[jnt_id->z].binang : fstatic[jnt_id->z].binang ),
			});
	}
	
	update_anim(self, obj);
	
	segment_swap_back();
}

void n64_object_set_material(N64Object* self, uint8_t id) {
	assert(id < N64_ARRAY_COUNT(n64_material_setup_dl));
	self->mtl_setup_dl_id = id;
}

static void draw_limb(N64Object* self, Object* obj, int id, GbiMtx** gbimtx, Mtx* mtx) {
	Limb* limb = &obj->limb[id];
	
	#define MTX_PUSH() mtx++; *mtx = mtx[-1]
	#define MTX_POP()  mtx--
	
	MTX_PUSH();
	
	N64Vector3 zero = {};
	N64Vector3* pos = &limb->pos;
	N64Vector3* rot = &zero;
	
	if (self->anim) {
		Anim* anim = self->anim;
		
		if (id == 0) pos = &anim->root_pos;
		rot = &anim->limb_rot[id];
	}
	
	mtx_translate_rot(mtx, pos, rot);
	
	if (limb->dlist) {
		if (gbimtx && *gbimtx) {
			mtx_to_gbimtx(mtx, (*gbimtx));
			gSPMatrix(POLY_OPA_DISP++, (*gbimtx), G_MTX_LOAD);
			(*gbimtx)++;
		}
		gSPDisplayList(POLY_OPA_DISP++, limb->dlist);
	}
	
	if (limb->child != 0xFF)
		draw_limb(self, obj, limb->child, gbimtx, mtx);
	
	MTX_POP();
	
	if (limb->sibling != 0xFF)
		draw_limb(self, obj, limb->sibling, gbimtx, mtx);
	
#undef MTX_PUSH
#undef MTX_POP
}

static void draw_skeleton(N64Object* self, Object* obj) {
	Mtx mtx[obj->limb_num + 1];
	GbiMtx* gbimtx = n64_graph_alloc(sizeof(GbiMtx[obj->limb_num + 1]));
	GbiMtx curmtx;
	
	memcpy(mtx, &self->mtx, sizeof(Mtx));
	mtx_to_gbimtx(&self->mtx, &curmtx);
	
	gSPDisplayList(POLY_OPA_DISP++, n64_material_setup_dl[self->mtl_setup_dl_id]);
	gSPSegment(POLY_OPA_DISP++, obj->seg_id, (void*)obj->data);
	gSPMatrix(POLY_OPA_DISP++, &curmtx, G_MTX_LOAD);
	gSPSegment(POLY_OPA_DISP++, 0xD, gbimtx);
	
	if (self->anim && n64_tick_20fps && self->anim->speed > __FLT_EPSILON__)
		update_anim(self, obj);
	
	draw_limb(self, obj, 0, &gbimtx, mtx);
}

void n64_object_draw(N64Object* self) {
	Object* obj = get_obj(self->obj_id, self->uniq_id);
	
	switch (obj->type) {
		case OBJ_TYPE_DLIST:
			break;
			
		case OBJ_TYPE_SKELETON:
			draw_skeleton(self, obj);
			break;
	}
}

void n64_object_set_mtx(N64Object* self, const void* mtx) {
	self->mtx = *(const Mtx*)mtx;
}