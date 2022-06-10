#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    CompressionResult bdi_result, fpc_result;
    char const *filename = "./data/repeating.bin";
    char const *logfilename = "comparison_result.txt";
    int filesize, bdisize, fpcsize;
    int chunksize;

    // freopen(logfilename,"w",stdout);

    if (argc > 1) filename = argv[1];
    if (argc > 2) chunksize = atoi(argv[2]);

    FILE *fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    fclose(fp);

    bdisize = 0;
    fpcsize = 0;

    for (int i = 0; i < filesize; i += chunksize) {
#ifdef VERBOSE
        printf("offset: %dBytes  size: %dBytes\n", i, chunksize);
#endif
        chunk = file2memorychunk(filename, i, chunksize);
        
        bdi_result = bdi_compression(chunk);
        fpc_result = fpc_compression(chunk);

        bdisize += bdi_result.compressed.size;
        fpcsize += fpc_result.compressed.size;

        remove_memory_chunk(chunk);
        remove_compression_result(bdi_result);
        remove_compression_result(fpc_result);
    }

    printf("====================\n");
    printf("compression ratio: %.4f(BDI) %.4f(FPC)\n", (double)filesize / bdisize, (double)filesize / fpcsize);
    printf("file size  (%3dBytes)\n", filesize);
    printf("BDI size   (%3dBytes)\n", bdisize);
    printf("FPC size   (%3dBytes)\n", fpcsize);
    printf("====================\n");

    return 0;
}