#ifndef __COMPRESSION_ALGORITHMS
#define __COMPRESSION_ALGORITHMS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// Verbose?
#define VERBOSE  // Comment this line not to display debug messages

// Parameters for cache line size
#define BYTE_BITWIDTH  8
#define BYTESIZ        1
#define HWORDSIZ       2
#define WORDSIZ        4
#define DWORDSIZ       8
#define CACHE32SIZ     32
#define CACHE64SIZ     64
#define CACHE128SIZ    128

// Boolean expression
#define FALSE 0
#define TRUE  1
typedef char  Bool;

// Structures for representing memory chunks(blocks)
typedef char           Byte;         // 1Byte (8bit)
typedef char *         ByteArr;      // Byte array
typedef long long int  ValueBuffer;  // 8Bytes

typedef struct {
    int size;
    int valid_bitwidth;
    char *body;
} MemoryChunk;

typedef MemoryChunk CacheLine;
typedef MemoryChunk MetaData;

typedef struct {
    CacheLine original;
    CacheLine compressed;
    Bool is_compressed;
    MetaData tag_overhead;
} CompressionResult;

// Functions for managing ByteArr and ValueBuffer
void set_value(ByteArr arr, ValueBuffer val, int offset, int size) {
    ValueBuffer mask = 1;
    for (int i = 0; i < size * BYTE_BITWIDTH; i++) {
        arr[(i / BYTE_BITWIDTH) + offset] |= ((((mask << i) & val) >> ((i / BYTE_BITWIDTH) * BYTE_BITWIDTH)));
    }
}

void set_value_bitwise(ByteArr arr, ValueBuffer val, int offset, int size) {
    ValueBuffer mask = 1;
    for (int i = 0; i < size; i++) {
        arr[(i + offset) / BYTE_BITWIDTH] |= ((mask << i) & val) << (((offset + i) % 8) - i);
    }
}

ValueBuffer get_value(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    for (int i = 0; i < size; i++) {
        val |= ((ValueBuffer)arr[offset + i] << (BYTE_BITWIDTH * i));
    }
    return val;
}

ValueBuffer get_value_bitwise(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    ValueBuffer mask = 1;
    for (int i = offset; i < offset + size; i++) {
        val |= ((ValueBuffer)arr[i / BYTE_BITWIDTH] & (mask << (i % BYTE_BITWIDTH))) << ((i / BYTE_BITWIDTH) * BYTE_BITWIDTH);
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

void remove_compression_result(CompressionResult result) {
    remove_memory_chunk(result.compressed);
    remove_memory_chunk(result.tag_overhead);
}

void print_memory_chunk(MemoryChunk chunk) {
    for (int i = chunk.size-1; i >= 0; i--) {
        printf("%02x", chunk.body[i] & 0xff);
        if (i % 4 == 0) printf(" ");
    }
}

void print_memory_chunk_bitwise(MemoryChunk chunk) {
    Byte buffer, mask = 1;

    for (int i = chunk.valid_bitwidth-1; i >= 0; i--) {
        buffer = chunk.body[i / BYTE_BITWIDTH];
        printf("%d", (buffer & mask << (i % BYTE_BITWIDTH)) != 0);
    }
}

void print_compression_result(CompressionResult result) {
    Byte buffer, mask = 1;

    printf("======= Compressed Cache Line =======\n");
    printf("original size: %dBytes\n", result.original.size);
    printf("original: ");
    print_memory_chunk(result.original);
    printf("\n");
    printf("compressed size: %dBytes\n", result.compressed.size);
    printf("compressed: ");
    print_memory_chunk(result.compressed);
    printf("\n");
    printf("tag overhead bitwidth: %dbits\n", result.tag_overhead.valid_bitwidth);
    printf("tag overhead: ");
    print_memory_chunk_bitwise(result.tag_overhead);
    printf("\n=====================================\n");
}

// Functions for BDI algorithms related functions
CompressionResult base_plus_delta(CacheLine original);                     // Base + delta algotithm 
CompressionResult base_delta_immediate(CacheLine original);                // BDI algorithm
CacheLine bdi_compressing_unit(CacheLine original, int encoding);  // Compressing Unit (CU)

#endif