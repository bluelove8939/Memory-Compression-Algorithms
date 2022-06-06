#include "compression.h"

int main(int argc, char const *argv[]) {
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    *(original.body+1) = 36;

    CompressionResult result = base_delta_immediate(original);
    print_compression_result(result);
    remove_compression_result(result);
    remove_memory_chunk(original);

    return 0;
}