#include <stdio.h>
#include <stdlib.h>
#include "compression.h"


CacheLine base_plus_delta(CacheLine original) {
    CacheLine compressed = make_cache_line(original.size);
    return compressed;
}


int main(int argc, char *argv[]) {
    CacheLine original;
    original.size = BYTESIZ;
    printf("original: size(%d) count(%d)", original.size);
}