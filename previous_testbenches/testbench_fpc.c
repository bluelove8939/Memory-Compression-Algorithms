#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    // Testbench for FPC algorithm
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    set_value(original.body, 0x00000000, 0,  4);  // prefix 0
    set_value(original.body, 0x00000002, 4,  4);  // prefix 1
    set_value(original.body, 0x0000007c, 8,  4);  // prefix 2
    set_value(original.body, 0x00003918, 12, 4);  // prefix 3
    set_value(original.body, 0x0000ffbb, 16, 4);  // prefix 4
    set_value(original.body, 0xfffb0045, 20, 4);  // prefix 5
    set_value(original.body, 0xfcfcfcfc, 24, 4);  // prefix 6
    set_value(original.body, 0x92817382, 28, 4);  // prefix 7

    CompressionResult result = fpc_compression(original);
    print_compression_result(result);
    printf("\n");

    DecompressionResult dec_result = fpc_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);
    printf("\n");

    remove_memory_chunk(original);
    remove_compression_result(result);
    remove_decompression_result(dec_result);

    return 0;
}