#include "directory.h"
#include "pages.h"
#include "inode.h"
#include "bitmap.h"
#include "util.h"

#include <errno.h>

void
init_directory(int inum)
{
	inode* node = get_inode(inum);
	node->refs = 0;
	node->mode = 040755;
	node->size = 0;
	node->blocks[0] = alloc_page();
	for(int i = 1; i < INODE_MAX_DIRECT_BLOCKS; ++i)
		node->blocks[i] = -1;
	node->iptr = -1;
	void* page = pages_get_page(node->blocks[0]);
	memset(page, 0, PAGE_SIZE);
}

int
find_entry(int directory_inum, const char* entry_name)
{
	if(directory_inum < 0 || directory_inum >= INODE_COUNT)
		return -1;

	inode* node = get_inode(directory_inum);
	if(!(node->mode & 040000))
		return -1;

	void* entry_bitfield = pages_get_page(node->blocks[0]);
	directory_entry* entry = entry_bitfield + bits_to_bytes(DIR_ENTRY_COUNT);
	for(int i = 0; i < DIR_ENTRY_COUNT; ++i, ++entry)
	{
		if(bitmap_get(entry_bitfield, i) && streq(entry->name, entry_name))
			return entry->inum;
	}
	return -1;
}

directory_entry*
get_entry(int directory_inum, int entry_num)
{
	if(directory_inum < 0 || directory_inum >= INODE_COUNT)
		return 0;
	if(entry_num < 0 || entry_num > DIR_ENTRY_COUNT)
		return 0;

	inode* node = get_inode(directory_inum);
	if(!(node->mode & 040000))
		return 0;

	void* entry_bitfield = pages_get_page(node->blocks[0]);
	directory_entry* entry_base = entry_bitfield + bits_to_bytes(DIR_ENTRY_COUNT);
	if(bitmap_get(entry_bitfield, entry_num))
		return entry_base + entry_num;
	else
		return 0;
}

int
get_entry_count(int directory_inum)
{
	if(directory_inum < 0 || directory_inum >= INODE_COUNT)
		return -1;
	inode* node = get_inode(directory_inum);
	if(!(node->mode & 040000))
		return -1;
	
	int sum = 0;
	void* entry_bitfield = pages_get_page(node->blocks[0]);
	directory_entry* entry_base = entry_bitfield + bits_to_bytes(DIR_ENTRY_COUNT);
	for(int i = 0; i < DIR_ENTRY_COUNT; ++i)
	{
		if(bitmap_get(entry_bitfield, i))
			++sum;
	}
	return sum;
}

int
add_entry(int directory_inum, const char* name, int entry_inum)
{
	inode* dir = get_inode(directory_inum);
	void* entry_bitfield = pages_get_page(dir->blocks[0]);
	directory_entry* entry = entry_bitfield + bits_to_bytes(DIR_ENTRY_COUNT);
	for(int i = 0; i < DIR_ENTRY_COUNT; ++i, ++entry)
	{
		if(!bitmap_get(entry_bitfield, i))
		{
			bitmap_put(entry_bitfield, i, 1);
			strncpy(entry->name, name, DIR_ENTRY_NAME_LENGTH - 1);
			entry->name[DIR_ENTRY_NAME_LENGTH - 1] = '\0';
			memcpy(entry->name, name, min(strlen(name) + 1, DIR_ENTRY_NAME_LENGTH - 1));
			entry->inum = entry_inum;
			get_inode(entry_inum)->refs++;
			return i; 
		}
	}
	return -EDQUOT;
}

void
free_entry(int directory_inum, const char* name)
{
	inode* dir = get_inode(directory_inum);
	void* entry_bitfield = pages_get_page(dir->blocks[0]);
	directory_entry* entry = entry_bitfield + bits_to_bytes(DIR_ENTRY_COUNT);
	for(int i = 0; i < DIR_ENTRY_COUNT; ++i, ++entry)
	{
		if(bitmap_get(entry_bitfield, i) && streq(name, entry->name))
		{
			bitmap_put(entry_bitfield, i, 0);
			inode* node = get_inode(entry->inum);
			node->refs--;
			if(node->refs == 0)
				free_inode(entry->inum);
			return;
		}
	}
}