#include "inode.h"
#include "bitmap.h"
#include "util.h"

#define FIRST_PAGE_INODE_COUNT 168

int
alloc_inode()
{
	void* ibm = get_inode_bitmap();

	for (int ii = 1; ii < INODE_COUNT; ++ii)
	{
		if (!bitmap_get(ibm, ii))
		{
	    		bitmap_put(ibm, ii, 1);
	    		printf("+ alloc_inode() -> %d\n", ii);
	    		return ii;
		}
	}

	return -1;
}

void
free_inode(int inum)
{
	printf("+ free_inode(%d)\n", inum);
	inode* node = get_inode(inum);
	if(node)
	{
		for(int i = 0; i < INODE_MAX_DIRECT_BLOCKS; ++i)
		{
			if(node->blocks[i] != -1)
				free_page(node->blocks[i]);
		}
		if(node->iptr != -1)
				free_page(node->iptr);
	}
	void* ibm = get_inode_bitmap();
	bitmap_put(ibm, inum, 0);
}

inode*
get_inode(int inum)
{
	if(inum < 0 || inum >= INODE_COUNT)
		return 0;
	else if(inum < FIRST_PAGE_INODE_COUNT)
		return (inode*)(get_inode_bitmap() + INODE_COUNT) + inum;
	else
		return (inode*)pages_get_page(1) + inum - FIRST_PAGE_INODE_COUNT;
}

void
init_file(int inum)
{
	inode* node = get_inode(inum);
	node->refs = 0;
	node->mode = 0100644;
	node->size = 0;
	for(int i = 0; i < INODE_MAX_DIRECT_BLOCKS; ++i)
		node->blocks[i] = -1;
	node->iptr = -1;
}