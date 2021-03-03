// based on cs3650 starter code

#ifndef PAGES_H
#define PAGES_H

#include <stdio.h>

typedef struct fStats {
    char fPath[50];
    mode_t mode;
    int st_size;
    int index;
    int links;
    char symLink[50];
    int ptr;
} fStats;

void pages_init(const char* path);
void pages_free();
void* pages_get_page(int pnum);
int alloc_page();
void free_page(int pnum);
int get_file_info(const char *path, struct stat *st); 
int make_file(const char *path, mode_t mode);
int list_files(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset);
int write_file(const char *path, const char *buf,size_t s, off_t offset);
int read_file(const char *path, char *buf,size_t s,off_t offset);
int rm_file(const char *path);
int change_name(const char *from, const char *to);
int make_hard_link(const char *from, const char *to);
int rm_dir(const char *path);
int make_file_sym(const char* from, const char* to);
char* get_file_sym(const char* from);

#endif
