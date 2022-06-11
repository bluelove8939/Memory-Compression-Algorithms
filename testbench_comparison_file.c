#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    CompressionResult bdi_result, fpc_result;
    char const *filename = "./data/repeating.bin";
    char const *logfilename = "comparison_result.txt";
    int filesize, originalsize, bdisize, fpcsize;
    int chunksize, iter, maxiter = 500;

    // freopen(logfilename,"w",stdout);

    if (argc > 2) {
        filename = argv[1];
        chunksize = atoi(argv[2]);
    } else {
        fprintf(stderr, "[ERROR] Insufficient number of line arguments (filename and memory chunk size is required\n");
        exit(-1);
    }

    if (argc > 3)
        maxiter = atoi(argv[3]);

    FILE *fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);

    originalsize = 0;
    bdisize = 0;
    fpcsize = 0;
    iter = 0;

    for (int i = 0; (i < filesize) && (iter < maxiter); i += chunksize) {
        chunk = file2memorychunk(filename, i, chunksize);
        
        bdi_result = bdi_compression(chunk);
        fpc_result = fpc_compression(chunk);

        originalsize += chunksize;
        bdisize += bdi_result.compressed.size;
        fpcsize += fpc_result.compressed.size;

#ifdef VERBOSE
        printf("offset: %dBytes  size: %dBytes\n", i, chunksize);
        printf("original: ");
        print_memory_chunk(chunk);
        printf("\n");
        printf("bdi size: %dBytes  fpc size: %dBytes\n\n", bdi_result.compressed.size, fpc_result.compressed.size);
#endif

        remove_memory_chunk(chunk);
        remove_compression_result(bdi_result);
        remove_compression_result(fpc_result);

        iter += 1;
    }

    printf("====================\n");
    printf("compression ratio: %.4f(BDI) %.4f(FPC)\n", (double)originalsize / bdisize, (double)originalsize / fpcsize);
    printf("original size (%3dBytes)\n", originalsize);
    printf("BDI size      (%3dBytes)\n", bdisize);
    printf("FPC size      (%3dBytes)\n", fpcsize);
    printf("====================\n");

    return 0;
}