#include <z64viewer.h>

typedef struct drawParams
{
	MemFile zScene;
	MemFile zRoom;
} DrawParams;

static void draw(void *udata)
{
	DrawParams *params = udata;
	
	z64viewer_scene(params->zScene.data);
	z64viewer_room(params->zRoom.data);
}

int main(void)
{
	DrawParams params = {0};
	// void* zobj = loadfile("object_link_boy.zobj", 0);
	// void* gameplay_keep = loadfile("gameplay_keep.zobj", 0);
	params.zScene = MemFile_Initialize();
	params.zRoom = MemFile_Initialize();
	
	MemFile_LoadFile(&params.zScene, "scene.zscene");
	MemFile_LoadFile(&params.zRoom, "room_0.zmap");
	
	if (z64viewer_init("z64viewer"))
		return 0;
	
	// render loop
	// -----------
	while (!z64viewer_shouldWindowClose())
	{
		z64viewer_update();
		z64viewer_draw(draw, &params);
	}
	
	z64viewer_terminate();
}

