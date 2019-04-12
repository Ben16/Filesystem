// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "pages.h"

#define INODE_MAX_DIRECT_BLOCKS 2
#define INODE_COUNT 255

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int blocks[INODE_MAX_DIRECT_BLOCKS]; // direct pointers
    int iptr; // single indirect pointer
} inode;

void print_inode(inode* node);
int alloc_inode();
void free_inode(int inum);
inode* get_inode(int inum);
void init_file(int inum);
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);

#endif
