#ifndef COMPRESSION_ALGORITHMS
#define COMPRESSION_ALGORITHMS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// Verbose?
// #define VERBOSE  // Comment this line not to display debug messages

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
void set_value(ByteArr arr, ValueBuffer val, int offset, int size);
void set_value_bitwise(ByteArr arr, ValueBuffer val, int offset, int size);
ValueBuffer get_value(ByteArr arr, int offset, int size);
ValueBuffer get_value_bitwise(ByteArr arr, int offset, int size);

// Functions for managing MemoryChunk
MemoryChunk make_memory_chunk(int size, int initial);
MemoryChunk copy_memory_chunk(MemoryChunk target);
void remove_memory_chunk(MemoryChunk chunk);
void remove_compression_result(CompressionResult result);
void print_memory_chunk(MemoryChunk chunk);
void print_memory_chunk_bitwise(MemoryChunk chunk);
void print_compression_result(CompressionResult result);

// Functions for BDI algorithms related functions
CompressionResult base_plus_delta(CacheLine original);             // Base + delta algotithm 
CompressionResult base_delta_immediate(CacheLine original);        // BDI algorithm
CacheLine bdi_compressing_unit(CacheLine original, int encoding);  // Compressing Unit (CU)

#endif