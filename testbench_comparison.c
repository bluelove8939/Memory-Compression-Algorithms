#include "compression.h"

int main(int argc, char const *argv[]) {
    CacheLine original = file2memorychunk("./data/test.bin", 0, CACHE64SIZ);

    CompressionResult result = bdi_compression(original);
    print_compression_result(result);
    printf("\n");

    DecompressionResult dec_result = bdi_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);
    printf("\n");

    remove_memory_chunk(original);
    remove_compression_result(result);
    remove_decompression_result(dec_result);

    return 0;
}