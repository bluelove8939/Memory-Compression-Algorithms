#include "compression.h"

int main(int argc, char const *argv[]) {
    // // Testbench for BDI algorithm
    // CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    // *(original.body+1) = 36;

    // CompressionResult result = bdi_compression(original);
    // print_compression_result(result);
    // printf("\n");

    // DecompressionResult dec_result = bdi_decompression(result.compressed, result.tag_overhead, result.original.size);
    // print_decompression_result(dec_result);
    // printf("\n");

    // remove_decompression_result(dec_result);
    // remove_compression_result(result);
    // remove_memory_chunk(original);


    // Testbench for FPC algorithm
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    set_value(original.body, 0x00, 0, 4);
    set_value(original.body, 0x02, 4, 4);

    CompressionResult result = fpc_compression(original);
    print_compression_result(result);
    printf("\n");

    return 0;
}