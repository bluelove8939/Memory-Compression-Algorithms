#include "compression.h"

int main(int argc, char const *argv[]) {
    // Testbench for BDI algorithm
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);

    CompressionResult result = cpack_compression(original);
    print_compression_result(result);
    printf("\n");

    DecompressionResult dec_result = cpack_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);
    printf("\n");

    remove_decompression_result(dec_result);
    remove_compression_result(result);
    remove_memory_chunk(original);

    return 0;
}