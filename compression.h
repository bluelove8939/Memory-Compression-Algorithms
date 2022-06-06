#ifndef __COMPRESSION_ALGORITHMS
#define __COMPRESSION_ALGORITHMS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// Parameters for cache line size
#define BYTE_BITWIDTH  8
#define BYTESIZ        1
#define HWORDSIZ       2
#define WORDSIZ        4
#define DWORDSIZ       8
#define CACHE32SIZ     32
#define CACHE64SIZ     64
#define CACHE128SIZ    128

// Parameters for boolean expression
#define FALSE 0
#define TRUE  1
typedef char  Bool;

// Structures for representing memory chunks(blocks)
typedef char     Byte;
typedef char *   ByteArr;
typedef long int ValueBuffer;

typedef struct {
    int size;
    int valid_bitwidth;
    char *body;
} MemoryChunk;

typedef MemoryChunk CacheLine;
typedef MemoryChunk MetaData;

typedef struct {
    CacheLine compressed;
    Bool is_compressed;
    MetaData tag_overhead;
    int tag_overhead_siz;
} CompressionResult;

// Functions for managing ByteArr and ValueBuffer
void set_value(ByteArr arr, ValueBuffer val, int offset, int size) {
    ValueBuffer mask = 1;
    for (int i = 0; i < size * BYTE_BITWIDTH; i++) {
        arr[(i / BYTE_BITWIDTH) + offset] |= (((mask << i) & val) >> ((i / BYTE_BITWIDTH) * BYTE_BITWIDTH));
    }
}

ValueBuffer get_value(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    for (int i = 0; i < size; i++) {
        val |= (arr[offset + i] << (BYTE_BITWIDTH * i));
    }
    return val;
}

// Functions for managing MemoryChunk
MemoryChunk make_memory_chunk(int size, int initial) {
    MemoryChunk result;
    result.size = size;
    result.valid_bitwidth = size * BYTE_BITWIDTH;  // default valid bitwidth = size * 8
    result.body = (char *)malloc(size);
    memset(result.body, initial, size);
    return result;
}

MemoryChunk copy_memory_chunk(MemoryChunk target) {
    MemoryChunk chunk;
    chunk.size = target.size;
    chunk.body = (char *)malloc(target.size);
    memcpy(chunk.body, target.body, chunk.size);
    return chunk;
}

void remove_memory_chunk(MemoryChunk chunk) {
    free(chunk.body);
}

void remove_compressed_result(CompressionResult result) {
    remove_memory_chunk(result.compressed);
}

void print_memory_chunk(MemoryChunk chunk) {
    printf("size: %dBytes\n", chunk.size);

    int *buffer = (int *)malloc(chunk.size);
    memcpy(buffer, chunk.body, chunk.size);
    printf("body: ");
    for (int i = chunk.size / sizeof(int) - 1; i >= 0; i--) {
        printf("%08x ", buffer[i]);
    }
    printf("\n");
    free(buffer);
}

void print_memory_chunk_bitwise(MemoryChunk chunk) {
    Byte buffer, mask;

    printf("valid bitwidth: %dbits", chunk.valid_bitwidth);
    printf("body: ");
    for (int i = chunk.valid_bitwidth-1; i >= 0; i--) {
        buffer = chunk.body[i / BYTE_BITWIDTH];
        printf("%d", (buffer & mask << (i % BYTE_BITWIDTH)) != 0);
    }
    printf("\n");
}

// Functions for BDI algorithms related functions
CacheLine base_plus_delta(CacheLine original);                     // Base + delta algotithm 
CacheLine base_delta_immediate(CacheLine original);                // BDI algorithm
CacheLine bdi_compressing_unit(CacheLine original, int encoding);  // Compressing Unit (CU)

#endif