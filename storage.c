#include "storage.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "bitmap.h"
#include "util.h"

#include <errno.h>
#include <assert.h>
#include <libgen.h>

int working_dir = 0;

int
find(const char* path, int dir)
{
	if(streq(path, "/"))
		return 0;
	else if(streq(path, "."))
		return dir;
	else
	{
		int len = strlen(path) + 1;
		char copy1[len];
		memcpy(copy1, path, sizeof(char)*len);
		char* basen = basename(copy1);
		char copy2[len];
		memcpy(copy2, path, sizeof(char)*len);
		char* dirn = dirname(copy2);
		return find_entry(find(dirn, dir), basen);
	}
}

void
storage_init(const char* path)
{
	pages_init(path);
	if(!get_inode(0)->mode)
	{
		bitmap_put(get_inode_bitmap(), 0, 1);
		init_directory(0);
	}
}

int 
storage_stat_inode(int inum, struct stat* st)
{
	inode* node = get_inode(inum);
	if(!node)
		return -ENOENT;
	st->st_mode = node->mode;
	st->st_size = node->size;
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_nlink = node->refs;
	st->st_ino = inum;
	return 0;
}

int 
storage_stat_fp(const char* path, struct stat* st)
{
	return storage_stat_inode(find(path, working_dir), st);
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
	int inum = find(path, working_dir);
	inode* node = get_inode(inum);
	if(!node || !(node->mode & 0100000))
		return -EINVAL;
	if(offset > node->size)
		return 0;

	int* page_index;
	if(node->size > INODE_MAX_DIRECT_BLOCKS * PAGE_SIZE)
		page_index = (int*)pages_get_page(node->iptr);
	else
		page_index = node->blocks;
	
	int remaining_actual_size = node->size - offset;
	size_t remaining_desired_size = size;
	page_index += offset / PAGE_SIZE;
	offset = offset % PAGE_SIZE;

	int to_read;
	while(remaining_desired_size > 0 && remaining_actual_size > 0)
	{
		to_read = min(remaining_desired_size, min(remaining_actual_size, PAGE_SIZE - offset));
		memcpy(buf, pages_get_page(*page_index) + offset, to_read);
		++page_index;
		offset = 0;
		remaining_desired_size -= to_read;
		remaining_actual_size -= to_read;
		buf += to_read;
	}
	
	return (int)(size - remaining_desired_size);
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
	int inum = find(path, working_dir);
	inode* node = get_inode(inum);
	if(node)
	{
		int new_size = max(offset + size, node->size);
		storage_truncate(path, new_size);
	}
	else
	{
		storage_truncate(path, size);
		inum = find(path, working_dir);
		node = get_inode(inum);
	}

	int* page_index;
	if(node->size > INODE_MAX_DIRECT_BLOCKS * PAGE_SIZE)
		page_index = (int*)pages_get_page(node->iptr);
	else
		page_index = node->blocks;

	size_t remaining_size = size;
	page_index += offset / PAGE_SIZE;
	offset = offset % PAGE_SIZE;

	int to_write;
	while(remaining_size > 0)
	{
		to_write = min(remaining_size, PAGE_SIZE - offset);
		memcpy(pages_get_page(*page_index) + offset, buf, to_write);
		++page_index;
		offset = 0;
		remaining_size -= to_write;
		buf += to_write;
	}
	return size;
}

int
storage_truncate(const char *path, off_t size)
{
	int inum = find(path, working_dir);
	inode* node = get_inode(inum);
	if(node)
	{
		if(!(node->mode & 0100000))
			return -EINVAL;
		int old_page_count = bytes_to_pages(node->size);
		node->size = size;
		int new_page_count = bytes_to_pages(node->size);
		if(old_page_count > new_page_count && old_page_count <= INODE_MAX_DIRECT_BLOCKS)
		{
			for(int i = new_page_count; i < old_page_count; ++i)
				free_page(node->blocks[i]);
		}
		else if(old_page_count > INODE_MAX_DIRECT_BLOCKS && new_page_count <= INODE_MAX_DIRECT_BLOCKS)
		{
			int* pnum = pages_get_page(node->iptr);
			for(int i = 0; i < new_page_count; ++i, ++pnum)
				node->blocks[i] = *pnum;
			for(int i = new_page_count; i < old_page_count; ++i, ++pnum)
				free_page(*pnum);
			free_page(node->iptr);
		}
		else if(new_page_count > INODE_MAX_DIRECT_BLOCKS && old_page_count > new_page_count)
		{
			int* pnum = (int*)pages_get_page(node->iptr) + new_page_count;
			for(int i = new_page_count; i < old_page_count; ++i, ++pnum)
				free_page(*pnum);
		}
		else if(old_page_count < new_page_count && new_page_count <= INODE_MAX_DIRECT_BLOCKS)
		{
			for(int i = old_page_count; i < new_page_count; ++i)
				node->blocks[i] = alloc_page();
		}
		else if(old_page_count <= INODE_MAX_DIRECT_BLOCKS && new_page_count > INODE_MAX_DIRECT_BLOCKS)
		{
			node->iptr = alloc_page();
			int* pnum = pages_get_page(node->iptr);
			for(int i = 0; i < old_page_count; ++i, ++pnum)
				*pnum = node->blocks[i];
			for(int i = old_page_count; i < new_page_count; ++i, ++pnum)
				*pnum = alloc_page();
		}
		else if(old_page_count < new_page_count && old_page_count > INODE_MAX_DIRECT_BLOCKS)
		{
			int* pnum = (int*)pages_get_page(node->iptr) + old_page_count;
			for(int i = old_page_count; i < new_page_count; ++i, ++pnum)
				*pnum = alloc_page();
		}
	}
	else
	{
		alloc_inode();
	}
}

int 
storage_mknod(const char* path, int mode)
{
	int len = strlen(path) + 1;
	char copy1[len];
	memcpy(copy1, path, sizeof(char)*len);
	char* dir = dirname(copy1);
	char copy2[len];
	memcpy(copy2, path, sizeof(char)*len);
	char* base = basename(copy2);
	int parent = find(dir, working_dir);

	int inum = alloc_inode();
	if(mode & 040000)
		init_directory(inum);
	else if(mode & 0100000)
		init_file(inum);
	return (add_entry(parent, base, inum) >= 0) ? 0 : -EACCES;
}

int
storage_unlink(const char* path)
{
	int len = strlen(path) + 1;
	char copy1[len];
	memcpy(copy1, path, sizeof(char)*len);
	char* dir = dirname(copy1);
	int parent = find(dir, working_dir);

	len = strlen(path) + 1;
	char copy2[len];
	memcpy(copy2, path, sizeof(char)*len);
	
	free_entry(parent, basename(copy2));
	return 0;
}

int
storage_link(const char* from, const char* to)
{
	int len = strlen(to) + 1;
	char copy2[len];
	memcpy(copy2, to, sizeof(char)*len);
	char* newDir = dirname(copy2);
	int newParent = find(newDir, working_dir);
	if(newParent < 0)
		return -ENOENT;

	int inum = find(from, working_dir);
	if(inum < 0)
		return -ENOENT;
	len = strlen(to) + 1;
	char copy3[len];
	memcpy(copy3, to, sizeof(char)*len);
	add_entry(newParent, basename(copy3), inum);
	return 0;
}

int
storage_rename(const char* from, const char* to)
{
	storage_link(from, to);
	storage_unlink(from);
}

int    storage_set_time(const char* path, const struct timespec ts[2]);