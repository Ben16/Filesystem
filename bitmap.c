#include "bitmap.h"

int
bitmap_get(void* bm, int ii)
{
	return (int)(*((char*)(bm + ii/8)) & (1 << (7 - ii%8)));
}

void bitmap_put(void* bm, int ii, int vv)
{
	if(vv)
		*((char*)(bm + ii/8)) |= (1 << (7 - ii%8));
	else
		*((char*)(bm + ii/8)) &= ~(1 << (7 - ii%8));
}

void bitmap_print(void* bm, int size)
{

}