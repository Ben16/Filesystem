// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_ENTRY_NAME_LENGTH 	48
#define DIR_ENTRY_COUNT	 	63

typedef struct directory_entry {
    char name[DIR_ENTRY_NAME_LENGTH];
    int  inum;
    char _padding[12];
} directory_entry;

void init_directory(int inum);
int find_entry(int directory_inum, const char* entry_name);
directory_entry* get_entry(int directory_inum, int entry_num);
int add_entry(int directory_inum, const char* name, int entry_inum);
void free_entry(int directory_inum, const char* name);

#endif

