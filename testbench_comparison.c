#include "compression.h"

int main(int argc, char const *argv[]) {
    char const *filename = "./data/repeating.bin";

    if (argc == 2) {
        filename = argv[1];
    }

    CacheLine original = file2memorychunk(filename, 0, CACHE64SIZ);

    CompressionResult bdi_result = bdi_compression(original);
    CompressionResult fpc_result = fpc_compression(original);

    printf("====================\n");
    printf("compression ratio: %.4f(BDI) %.4f(FPC)\n", (double)original.size / bdi_result.compressed.size, (double)original.size / fpc_result.compressed.size);
    printf("original   (%3dBytes): ", original.size);
    print_memory_chunk(original);
    printf("\n");
    printf("BDI result (%3dBytes): ", bdi_result.compressed.size);
    print_memory_chunk(bdi_result.compressed);
    printf("\n");
    printf("FPC result (%3dBytes): ", fpc_result.compressed.size);
    print_memory_chunk(fpc_result.compressed);
    printf("\n====================\n");

    remove_memory_chunk(original);
    remove_compression_result(bdi_result);
    remove_compression_result(fpc_result);

    return 0;
}