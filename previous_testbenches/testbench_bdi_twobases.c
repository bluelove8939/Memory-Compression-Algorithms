#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    // Testbench for BDI algorithm
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    set_value(original.body, 0xff, 0, 1);
    set_value(original.body, 0xff, 1, 1);
    set_value(original.body, 0x00, 2, 1);
    set_value(original.body, 0x00, 3, 1);

    CompressionResult result = bdi_twobase_compression(original);
    print_compression_result(result);
    printf("\n");

    DecompressionResult dec_result = bdi_twobase_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);
    printf("\n");

    remove_decompression_result(dec_result);
    remove_compression_result(result);
    remove_memory_chunk(original);

    return 0;
}