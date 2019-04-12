#include "bitmap.h"

int
bitmap_get(void* bm, int ii)
{
	return (int)(*((char*)(bm + ii/8)) & (1 << (7 - ii)));
}

void bitmap_put(void* bm, int ii, int vv)
{
	if(vv)
		*((char*)(bm + ii/8)) |= (1 << (7 - ii));
	else
		*((char*)(bm + ii/8)) &= ~(1 << (7 - ii));
}

void bitmap_print(void* bm, int size)
{

}