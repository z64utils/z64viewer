#include <zroom.h>
#include <bigendian.h>
#include <n64.h>

#include <stdint.h>

/*
 *
 * private
 *
 */
static void doMeshHeader0(void *header, int layer)
{
	uint8_t *h8 = header;
	uint8_t num = h8[1];
	uint32_t *first = n64_virt2phys(u32r(h8 + 4));
	uint32_t *last = n64_virt2phys(u32r(h8 + 8));
	
	while (first < last && num--)
	{
		void *opa = n64_virt2phys(u32r(first + 0));
		void *xlu = n64_virt2phys(u32r(first + 1));
		
		if (layer == 0)
			n64_draw(opa);
		else
			n64_draw(xlu);
		
		first += 2; /* stride */
	}
}
static void doMeshHeader1(void *header, int layer)
{
	/* TODO static background type for prerenders */
}
static void doMeshHeader2(void *header, int layer)
{
	uint8_t *h8 = header;
	uint8_t num = h8[1];
	uint32_t *first = n64_virt2phys(u32r(h8 + 4));
	uint32_t *last = n64_virt2phys(u32r(h8 + 8));
	
	while (first < last && num--)
	{
		void *opa = n64_virt2phys(u32r(first + 2));
		void *xlu = n64_virt2phys(u32r(first + 3));
		
		/* TODO primitives visualizing culling information
		 * might be useful for debugging eventually
		 */
		
		if (layer == 0)
			n64_draw(opa);
		else
			n64_draw(xlu);
		
		first += 4; /* stride */
	}
}
static void doMeshHeader(void *header, int layer)
{
	uint8_t *header8 = header;
	uint8_t type = *header8;
	
	switch (type)
	{
		case 0:
			doMeshHeader0(header, layer);
			break;
		
		case 1:
			doMeshHeader1(header, layer);
			break;
		
		case 2:
			doMeshHeader2(header, layer);
			break;
	}
}

/*
 *
 * public
 *
 */
void zroom_draw(void *room)
{
	uint8_t *room8;
	
	/* rooms go in segment 0x03 */
	n64_set_segment(0x03, room);
	
	/* step through each header command until 'end header' command reached */
	for (room8 = room; *room8 != 0x14; room8 += 8)
	{
		switch (*room8)
		{
			case 0x0A:
			{
				void *p = n64_virt2phys(u32r(room8 + 4));
				int i;
				
				for (i = 0; i < 2; ++i)
				{
					n64_set_onlyZmode(ZMODE_ALL);
					doMeshHeader(p, i);
				}
				break;
			}
		}
	}
}

