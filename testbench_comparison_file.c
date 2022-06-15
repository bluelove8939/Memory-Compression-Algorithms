#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    CompressionResult bdi_result, fpc_result, bdi_twobase_result, bdi_zr_result;
    char const *filename = "./data/repeating.bin";
    char const *logfilename = "comparison_result.txt";
    int filesize, original_size, bdi_size, fpc_size, bdi_twobase_size, bdi_zr_size;
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
    bdi_size = 0;
    fpc_size = 0;
    bdi_twobase_size = 0;
    bdi_zr_size = 0;
    iter = 0;

    for (int i = 0; (i < filesize) && (iter < maxiter); i += chunksize) {
        chunk = file2memorychunk(filename, i, chunksize);
        
        bdi_result = bdi_compression(chunk);
        fpc_result = fpc_compression(chunk);
        bdi_twobase_result = bdi_twobase_compression(chunk);
        bdi_zr_result = bdi_zr_compression(chunk);

        original_size += chunksize;
        bdi_size += bdi_result.compressed.size;
        fpc_size += fpc_result.compressed.size;
        bdi_twobase_size += bdi_twobase_result.compressed.size;
        bdi_zr_size += bdi_zr_result.compressed.size;

#ifdef VERBOSE
        printf("offset: %dBytes  size: %dBytes\n", i, chunksize);
        printf("original: ");
        print_memory_chunk(chunk);
        printf("\n");
        printf("bdi size: %dBytes  fpc size: %dBytes  bdi twobase size: %dBytes  bdi zr size: %dBytes\n\n", 
                bdi_result.compressed.size, 
                fpc_result.compressed.size,
                bdi_twobase_result.compressed.size,
                bdi_zr_result.compressed.size);
#endif

        remove_memory_chunk(chunk);
        remove_compression_result(bdi_result);
        remove_compression_result(fpc_result);
        remove_compression_result(bdi_twobase_result);
        remove_compression_result(bdi_zr_result);

        iter += 1;
    }

    printf("====================\n");
    printf("compression ratio: %.4f(BDI) %.4f(FPC) %.4f(BDI 2B) %.4f(BDI ZR)\n", 
            (double)original_size / bdi_size, 
            (double)original_size / fpc_size, 
            (double)original_size / bdi_twobase_size,
            (double)original_size / bdi_zr_size);
    printf("original size (%3dBytes)\n", original_size);
    printf("BDI size      (%3dBytes)\n", bdi_size);
    printf("FPC size      (%3dBytes)\n", fpc_size);
    printf("BDI 2B size   (%3dBytes)\n", bdi_twobase_size);
    printf("BDI ZR size   (%3dBytes)\n", bdi_zr_size);
    printf("====================\n");

    return 0;
}