// #include <z64.h>
//
// void SkelAnime_DrawFlexLimbOpa(GlobalContext* globalCtx, s32 limbIndex, void** skeleton, Vec3s* jointTable,
//     OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg,
//     Mtx** limbMatricies) {
// 	StandardLimb* limb;
// 	Gfx* newDList;
// 	Gfx* limbDList;
// 	Vec3f pos;
// 	Vec3s rot;
//
// 	OPEN_DISPS(globalCtx->state.gfxCtx, "../z_skelanime.c", 1214);
//
// 	Matrix_Push();
//
// 	limb = (StandardLimb*)SEGMENTED_TO_VIRTUAL(skeleton[limbIndex]);
// 	limbIndex++;
// 	rot = jointTable[limbIndex];
//
// 	pos.x = limb->jointPos.x;
// 	pos.y = limb->jointPos.y;
// 	pos.z = limb->jointPos.z;
//
// 	newDList = limbDList = limb->dList;
//
// 	if ((overrideLimbDraw == NULL) || !overrideLimbDraw(globalCtx, limbIndex, &newDList, &pos, &rot, arg)) {
// 		Matrix_TranslateRotateZYX(&pos, &rot);
// 		if (newDList != NULL) {
// 			Matrix_ToMtx(*limbMatricies, "../z_skelanime.c", 1242);
// 			gSPMatrix(POLY_OPA_DISP++, *limbMatricies, G_MTX_LOAD);
// 			gSPDisplayList(POLY_OPA_DISP++, newDList);
// 			(*limbMatricies)++;
// 		} else if (limbDList != NULL) {
// 			Matrix_ToMtx(*limbMatricies, "../z_skelanime.c", 1249);
// 			(*limbMatricies)++;
// 		}
// 	}
//
// 	if (postLimbDraw != NULL) {
// 		postLimbDraw(globalCtx, limbIndex, &limbDList, &rot, arg);
// 	}
//
// 	if (limb->child != LIMB_DONE) {
// 		SkelAnime_DrawFlexLimbOpa(
// 			globalCtx,
// 			limb->child,
// 			skeleton,
// 			jointTable,
// 			overrideLimbDraw,
// 			postLimbDraw,
// 			arg,
// 			limbMatricies
// 		);
// 	}
//
// 	Matrix_Pop();
//
// 	if (limb->sibling != LIMB_DONE) {
// 		SkelAnime_DrawFlexLimbOpa(
// 			globalCtx,
// 			limb->sibling,
// 			skeleton,
// 			jointTable,
// 			overrideLimbDraw,
// 			postLimbDraw,
// 			arg,
// 			limbMatricies
// 		);
// 	}
// 	CLOSE_DISPS(globalCtx->state.gfxCtx, "../z_skelanime.c", 1265);
// }
//
// void SkelAnime_DrawFlexOpa(GlobalContext* globalCtx, void** skeleton, Vec3s* jointTable, s32 dListCount,
//     OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg) {
// 	StandardLimb* rootLimb;
// 	s32 pad;
// 	Gfx* newDList;
// 	Gfx* limbDList;
// 	Vec3f pos;
// 	Vec3s rot;
// 	Mtx* mtx = Graph_Alloc(globalCtx->state.gfxCtx, dListCount * sizeof(Mtx));
//
// 	if (skeleton == NULL) {
// 		osSyncPrintf(VT_FGCOL(RED));
// 		osSyncPrintf("Si2_draw_SV():skelがNULLです。\n"); // "skel is NULL."
// 		osSyncPrintf(VT_RST);
//
// 		return;
// 	}
//
// 	OPEN_DISPS(globalCtx->state.gfxCtx, "../z_skelanime.c", 1294);
//
// 	gSPSegment(POLY_OPA_DISP++, 0xD, mtx);
//
// 	Matrix_Push();
//
// 	rootLimb = SEGMENTED_TO_VIRTUAL(skeleton[0]);
//
// 	pos.x = jointTable[0].x;
// 	pos.y = jointTable[0].y;
// 	pos.z = jointTable[0].z;
//
// 	rot = jointTable[1];
//
// 	newDList = limbDList = rootLimb->dList;
//
// 	if ((overrideLimbDraw == NULL) || !overrideLimbDraw(globalCtx, 1, &newDList, &pos, &rot, arg)) {
// 		Matrix_TranslateRotateZYX(&pos, &rot);
// 		if (newDList != NULL) {
// 			Matrix_ToMtx(mtx, "../z_skelanime.c", 1327);
// 			gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_LOAD);
// 			gSPDisplayList(POLY_OPA_DISP++, newDList);
// 			mtx++;
// 		} else if (limbDList != NULL) {
// 			Matrix_ToMtx(mtx, "../z_skelanime.c", 1334);
// 			mtx++;
// 		}
// 	}
//
// 	if (postLimbDraw != NULL) {
// 		postLimbDraw(globalCtx, 1, &limbDList, &rot, arg);
// 	}
//
// 	if (rootLimb->child != LIMB_DONE) {
// 		SkelAnime_DrawFlexLimbOpa(
// 			globalCtx,
// 			rootLimb->child,
// 			skeleton,
// 			jointTable,
// 			overrideLimbDraw,
// 			postLimbDraw,
// 			arg,
// 			&mtx
// 		);
// 	}
//
// 	Matrix_Pop();
// 	CLOSE_DISPS(globalCtx->state.gfxCtx, "../z_skelanime.c", 1347);
// }