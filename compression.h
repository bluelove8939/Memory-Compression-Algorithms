#ifndef __COMPRESSION_HEADER__
#define __COMPRESSION_HEADER__

#include <stdio.h>
#include <stdlib.h>

#define BYTESIZ   8
#define HWORDSIZ  16
#define WORDSIZ   32
#define DWORDSIZ  64

typedef struct _CacheLine {
    int size;
    int count;
    char *body;
} CacheLine;

CacheLine make_cache_line(int size, int count) {
    CacheLine cache_line;
    cache_line.size = size;
    cache_line.count = count;
    cache_line.body = (char *)malloc(size * count);
    return cache_line;
}

void remove_cache_line(CacheLine *target) {
    free(target->body);
}

#endif