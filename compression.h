#ifndef __COMPRESSION_ALGORITHMS
#define __COMPRESSION_ALGORITHMS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Parameters for cache line size
#define BYTESIZ      1
#define HWORDSIZ     2
#define WORDSIZ      4
#define DWORDSIZ     8
#define CACHE32SIZ   32
#define CACHE64SIZ   64
#define CACHE128SIZ  128
typedef int* CacheLine;
typedef int* MetaData;

// Parameters for boolean expression
#define FALSE 0
#define TRUE  1
typedef int  Bool;

typedef struct {
    CacheLine compressed;
    Bool is_compressed;
    MetaData tag_overhead;
    int tag_overhead_siz;
} CompressionResult;

// BDI algorithms related functions
CompressionResult base_plus_delta(CacheLine original);       // Base + delta algotithm 
CompressionResult base_delta_immediate(CacheLine original);  // BDI algorithm
CompressionResult bdi_compressing_unit(CacheLine original, int k, int d);  // Compressing Unit (CU)

#endif