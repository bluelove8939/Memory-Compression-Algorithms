#include "compression.h"


#define COMPRESSION_ONLY  // comment this line not to check decompression algorithm


int main(int argc, char const *argv[]) {
    // Testbench for BDI algorithm
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    set_value(original.body, 0x00000022, 0,  4);
    set_value(original.body, 0x00002200, 4,  4);
    set_value(original.body, 0x00000000, 8,  4);
    set_value(original.body, 0x00000000, 12, 4);

    CompressionResult result = bdi_ze_compression(original);
    print_compression_result(result);
    printf("\n");

#ifndef COMPRESSION_ONLY
    DecompressionResult dec_result = bdi_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);
    printf("\n");

    remove_decompression_result(dec_result);
#endif
    remove_compression_result(result);
    remove_memory_chunk(original);

    return 0;
}