// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fuse.h>

#include "pages.h"
#include "util.h"
#include "bitmap.h"

//typedef struct fStats {
//    char* fPath;
//    mode_t mode;
//    int st_size;
//    int index;
//    int links;
//} fStats;


const int PAGE_COUNT = 256;
const int NUFS_SIZE  = 4096 * 256; // 1MB

static int   pages_fd   = -1;
//pages_base is pointer to allocated memory so range from page_base to page_base + 1MB
static void* pages_base =  0;

//pbm is array of 8 ints each int stores 32 pages for a total of 32*8 or 256 pages
//when bit is 1 page is taken, when bit is 0 page is free
static int* pbm;
static fStats* stats;
static int *size = 0;

void
print_stats()
{
	for (int ii = 0; ii < *size; ii++){
		printf("%d: %s - %d\n", ii, stats[ii].fPath, stats[ii].index);
	}
}

int
make_file(const char *path, mode_t mode){
    
    int ii = alloc_page();
    //add to total number of files
    *size += 1;
    stats[*size-1].st_size = 0;
    stats[*size-1].links = 1;
    stats[*size-1].index = ii;
    stats[*size-1].mode = mode;
    stats[*size-1].ptr = 0;

    strcpy(stats[*size-1].fPath, path);
    printf(" Allocated slot =  %d \n" , ii);
    return ii;
}

int
get_file_info(const char *path, struct stat *st){
    //check every file struct to see if a file corresponds to the given path
    //iterate through every file in stats to find desired file 

    for (int ii = 0; ii < *size; ii ++){
        if (stats[ii].index != -1){
            if (strcmp(path, stats[ii].fPath) == 0){
                st->st_mode = stats[ii].mode;
                st->st_size = stats[ii].st_size;
                st->st_uid = getuid();
                st->st_nlink = stats[ii].links;
                return 0;
            }
        }
    }

    if (strcmp(path, "/") == 0) {
        st->st_mode = 040755; // directory
        st->st_size = 0;
        st->st_uid = getuid();
        st->st_nlink = 1;
        if (*size >= 1) {

        } else {
            make_file(path, 040755);
        }
        return 0;
    } else {
        return -ENOENT;
    }

}


int
list_files(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset){
    struct stat st;
    for (int ii = 0; ii < *size; ii ++){
        //if file name contains root path
        if (strstr(stats[ii].fPath, path) != NULL){
            if (stats[ii].index != -1){
                if (strcmp(path, stats[ii].fPath) == 0){
                } else {
                    char* str = stats[ii].fPath + strlen(path);
                    //if there is still a / in path name then it should not be returned
                    if (strstr(str, "/") != NULL){
                        str = str + 1;
                        if (strstr(str, "/") != NULL){
                        } else {
                            int rv = 0; //get_file_info(stats[ii].fPath, &st);
                            assert(rv == 0);
                            filler(buf, str, &st , 0);
                        }
                        
                    } else {
                        int rv = get_file_info(stats[ii].fPath, &st);
                        assert(rv == 0);
                        filler(buf, str, &st , 0);
                    }
                }
            }
       }
    }
    return 0;

}

int
get_index(const char *path)
{
	for (int ii = 0; ii < *size; ii ++){
        	if (strcmp(stats[ii].fPath, path) == 0){
			return ii;
		}
	}
	return -1;
}

void
add_pages(int nums, int index) 
{
	struct fStats * curr = &stats[index];
	for(int ii = 0; ii <= nums; ii++){
		printf("Adding: %d/%d\n", ii, nums);
		if(curr->ptr == 0){
			int newPtr = make_file(curr->fPath, curr->mode); 
			curr->ptr = newPtr;
			print_stats();
			printf("%d\n", newPtr);
			curr = &stats[newPtr+1];
		}
		else{
			curr = &stats[curr->ptr+1];
		}
	}

}


int
write_small(const char *path, const char *buf,size_t s, off_t offset){
    for (int ii = 0; ii < *size; ii ++){
        if (strcmp(stats[ii].fPath, path) == 0){
            stats[ii].st_size += s;
	    
            void* dest =  (void *) pages_get_page(stats[ii].index) + offset;
            printf("%p\n", dest);
	    memcpy(dest, buf, s);
            char* pchar = (char*) dest;
            break;
        }
    }

    return s;

}

int
write_file(const char *path, const char *buf,size_t s, off_t offset){

	if((s+offset) <= 4096){
		return write_small(path, buf, s, offset);
	}
	int numToAdd = ((s+offset)/4096);
	int byteLeft =s;
	int keeper = s;
	int statNum = get_index(path);
	struct fStats first = stats[statNum];
	first.st_size += s;
	struct fStats* curr = &first;
	add_pages(numToAdd, statNum);
	while(byteLeft > 0){
		if(offset > 4096){
			offset -= 4096;
			curr = &stats[curr->ptr];
			continue;
		}
		int pgn = curr->index;
		if (s+offset > 4096) {
			s = 4096-offset;
		}
		void * des = (void*) pages_get_page(pgn);
		printf("%p\n", des);
		offset = 0;
		memcpy(des, buf, s);
		byteLeft -= s;
		s = byteLeft;
		curr = &stats[curr->ptr];
	}
	
    	return keeper;

}

int
read_small(const char *path, char *buf,size_t s,off_t offset){
	    print_stats();
//	    printf("%d, %ld\n", s, offset);
	    int ii = get_index(path);
	    printf("%d\n", stats[ii].index);
	    void* src = (void *) pages_get_page(stats[ii].index) - offset;
	    printf("%p\n", src);
            memcpy(buf, src, s);
           

       
    return s;
}

int
read_file(const char *path, char *buf,size_t s,off_t offset){
	if(s <= 4096 && offset < 4096){
		printf("------\n");
	    return read_small(path, buf, s, offset);
	}
	char* tempBuf;
	int byteLeft = s;
	int keeper = s;
	int statNum = get_index(path);
	printf("%d\n", statNum);
	struct fStats* curr = &stats[statNum];
	while(byteLeft > 0 && curr->ptr != 0){	
		if(offset >= 4096){
			offset -= 4096;
			if(curr->ptr == 0){
				printf("error\n");
			}
			curr = &stats[curr->ptr];
			continue;
		}
		int pgn = curr->index;
		if(s+offset > 4096){
			s = 4096 - offset;
		}
		void* src = (void*)pages_get_page(pgn) + offset;
		offset = 0;
		memcpy(buf, src, s);
		byteLeft -= s;
		s = byteLeft;
		curr = &stats[curr->ptr];
	}

	return keeper;



}

int rm_file(const char *path){
    for (int ii = 0; ii < *size; ii ++){
        if (strcmp(stats[ii].fPath, path) == 0){
            if (stats[ii].links > 1){
                stats[ii].links -= 1;
            } else {
                //put 0 in bitmap since that page is now empty
                bitmap_put(pbm, stats[ii].index, 0);
                stats[ii].index = -1;
                stats[ii].links = 0;
            }
            break;
        }
    }
    return 0;
}

int 
rm_dir(const char *path){
    for (int ii = 0; ii < *size; ii++){
        if (strcmp(stats[ii].fPath, path) == 0){
            if (stats[ii].links > 1){
                stats[ii].links -=1;
            } else {
                bitmap_put(pbm, stats[ii].index, 0);
                stats[ii].index = -1;
                stats[ii].links = 0;
            }
            break;
        }
    }
    return 0;
}

int change_name(const char *from, const char *to){
    int rv = -1;
    for (int ii = 0; ii < *size; ii ++){
        if (strcmp(stats[ii].fPath, from) == 0 && stats[ii].index != -1){
            strcpy(stats[ii].fPath, to);
            rv = 0;
            break;
        }
    }
    return rv;
}

int
make_hard_link(const char *from, const char *to)
{
    int rv = -1;
    for (int ii = 0; ii < *size; ii ++){
        if (strcmp(stats[ii].fPath, from) == 0){
            //increment size but use same data as from file
            *size += 1;
            stats[*size-1].st_size = stats[ii].st_size;
            stats[*size-1].index = stats[ii].index;
            stats[*size-1].mode = stats[ii].mode;
            strcpy(stats[*size-1].fPath, to);
            stats[*size-1].links = 1;
            stats[ii].links += 1;
            rv = 0;
            break;
        }
    }
    return rv;
}

char*
get_file_sym(const char* from){
    for (int ii = 0; ii < *size; ii ++){
        if (strcmp(from,stats[ii].fPath) == 0 && stats[ii].index != -1){
            return stats[ii].symLink;
        }
    }
}

int
make_file_sym(const char* from, const char* to){
    make_file(to, S_IFLNK);
    strcpy(stats[*size-1].symLink, from);
}

void
pages_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);

    //pbm = get_pages_bitmap();
    pbm = (int*) pages_get_page(0);
    //fstat points to pbm + size of pbm array 256 bits or 32 bytes + 4 bytes for int that stores its size
    size = (void*) pages_base + 32;

    //printf("size value  %d \n", *((int*) pages_base + 8));
    //printf("size value  %p \n", ((int*) pages_base + 8));
    //printf("size value  %p \n", ((void*) pages_base + 32));
    //printf("size value  %p \n", size);
    //printf("size value  %d \n", *size);

    stats = (void*) pages_get_page(0) + 36;


    //first part of first page stores the bitmap of occupied pages
    //second part of first page stores array of file stat for occupied pages 
    bitmap_put(pbm, 0, 1);
}

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}


int
alloc_page()
{

    for (int ii = 0; ii < PAGE_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii)) {
            bitmap_put(pbm, ii, 1);
            printf("+ alloc_page() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void
free_page(int pnum)
{
    printf("+ free_page(%d)\n", pnum);
    bitmap_put(pbm, pnum, 0);
}

