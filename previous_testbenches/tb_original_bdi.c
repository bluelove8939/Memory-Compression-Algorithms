#include "original_bdi_compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    char const *filename = "./data/repeating.bin";
    char const *logfilename = "./logs/comparison_result.txt";
    int filesize, original_size, compressed_size, result;
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

    original_size = 0;
    compressed_size = 0;

    for (int i = 0; (i < filesize) && (iter < maxiter); i += chunksize) {
        chunk = file2memorychunk(filename, i, chunksize);
        result = bdi_original_compression(chunk);

        original_size += chunksize;
        compressed_size += result;

#ifdef VERBOSE
        printf("[SEARCH %d] offset: %dBytes  size: %dBytes\n", i/chunksize, i, chunksize);
        printf("original: ");
        print_memory_chunk(chunk);
        printf("\n");
        printf("current compressed %dBytes\n\n", result);
#endif

#ifndef VERBOSE
        printf("[ITER %2d] offset: %dBytes  size: %dBytes\n", iter+1, i, chunksize);
#endif

        remove_memory_chunk(chunk);

        iter += 1;
    }

    printf("\n====================\n");
    printf("compression ratio: %.4f(BDI)\n", 
            (double)original_size / compressed_size);
    printf("original size   (%3dBytes)\n", original_size);
    printf("compressed size (%3dBytes)\n", compressed_size);
    printf("====================\n");

    return 0;
}