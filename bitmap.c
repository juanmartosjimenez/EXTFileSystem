#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "bitmap.h"
#include "bit-ops.h"

int 
bitmap_get(int* bm, int ii){
    if (TestBit(bm, ii) !=0 ){
        return 1;
    } else {
        return 0;
    }
}

int*
bitmap_put(int* bm, int ii, int vv){
    if (vv == 0){
        ClearBit(bm, ii);
    } else {
        SetBit(bm, ii);
    }
    bitmap_print(bm);
}


int*
bitmap_print(int* bm){
    for (int ii = 0; ii < 256; ii ++ ){
        printf("%d", bitmap_get(bm, ii));
    }
    printf("\n");
}



