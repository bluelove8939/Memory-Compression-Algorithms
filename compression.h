#ifndef COMPRESSION_ALGORITHMS
#define COMPRESSION_ALGORITHMS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages

// Parameters for cache line size
#define BYTE_BITWIDTH  8
#define BYTESIZ        1
#define HWORDSIZ       2
#define WORDSIZ        4
#define DWORDSIZ       8
#define CACHE32SIZ     32   // 32Bytes cacheline
#define CACHE64SIZ     64   // 64Bytes cacheline
#define CACHE128SIZ    128  // 128Bytes cacheline

// Sign extension macro
#define SIGNEX(v, sb) ((v) | (((v) & (1 << (sb))) ? ~((1 << (sb))-1) : 0))

// Boolean expression
#define FALSE 0
#define TRUE  1
typedef char  Bool;

// Structures for representing memory chunks(blocks)
typedef unsigned char           Byte;         // 1Byte (8bit)
typedef unsigned char *  ByteArr;      // Byte array
typedef long long int    ValueBuffer;  // 8Bytes

typedef struct {
    int size;            // byte size
    int valid_bitwidth;  // valid bitwidth
    ByteArr body;        // actual memory block
} MemoryChunk;

typedef MemoryChunk CacheLine;  // memory block representing cacheline
typedef MemoryChunk MetaData;   // memory block represinting compression overheads (e.g. tag overhead)

typedef struct {
    char *compression_type;  // name of compression algorithm
    CacheLine original;      // original cacheline (given)
    CacheLine compressed;    // compressed cacheline (induced)
    Bool is_compressed;      // flag identifying whether the given cacheline is compressed
    MetaData tag_overhead;   // tag overhead (e.g. encoding type, segment pointer ...)
} CompressionResult;

typedef struct {
    char *compression_type;  // name of decompression algorithm
    CacheLine original;      // original cacheline (induced)
    CacheLine compressed;    // compressed cacheline (given)
    Bool is_decompressed;    // flag identifying whether the given cacheline is decompressed
} DecompressionResult;

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
void remove_decompression_result(DecompressionResult result);
void print_memory_chunk(MemoryChunk chunk);
void print_memory_chunk_bitwise(MemoryChunk chunk);
void print_compression_result(CompressionResult result);
void print_decompression_result(DecompressionResult result);

// Functions for BDI(Base Delta Immediate) algorithm
CompressionResult bdi_compression(CacheLine original);                                                  // BDI compression algorithm
DecompressionResult bdi_decompression(CacheLine compressed, MetaData tag_overhead, int original_size);  // BDI decompression algorithm
CacheLine bdi_compressing_unit(CacheLine original, int encoding);                                       // Compressing Unit (CU)

// Functions for FPC(Frequent Pattern Compression) algorithm
CompressionResult fpc_compression(CacheLine original);  // FPC compression algorithm

#endif