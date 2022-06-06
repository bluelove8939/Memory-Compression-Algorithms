#include "compression.h"

int main(int argc, char const *argv[]) {
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    *(original.body+1) = 36;

    CompressionResult result = bdi_compression(original);
    print_compression_result(result);

    DecompressionResult dec_result = bdi_decompression(result.compressed, result.tag_overhead, result.original.size);
    print_decompression_result(dec_result);

    remove_decompression_result(dec_result);
    remove_compression_result(result);
    remove_memory_chunk(original);

    return 0;
}