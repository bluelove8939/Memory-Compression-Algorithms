#include "compression.h"
#include "original_bdi_compression.h"  // Including original BDI algorithm

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    CompressionResult bdi_result, fpc_result, bdi_twobase_result, bdi_zr_result, zero_vec_result, zeros_run_result, bdi_ze_result;
    char const *filename = "./data/repeating.bin";
    char const *logfilename = "./logs/comparison_result.txt";
    int filesize, original_size, bdi_size, fpc_size, bdi_twobase_size, bdi_zr_size, zero_vec_size, zeros_run_size, bdi_ze_size;
    int chunksize, iter, maxiter = 500;

#ifdef ORIGINAL_BDI
    int bdi_original_size = 0;
    int bdi_original_result = 0;
#endif

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
    zero_vec_size = 0;
    zeros_run_size = 0;
    bdi_ze_size = 0;
    iter = 0;

    for (int i = 0; (i < filesize) && (iter < maxiter); i += chunksize) {
        chunk = file2memorychunk(filename, i, chunksize);
        
        bdi_result = bdi_compression(chunk);
        fpc_result = fpc_compression(chunk);
        bdi_twobase_result = bdi_twobase_compression(chunk);
        bdi_zr_result = bdi_zr_compression(chunk);
        zero_vec_result = zero_vec_compression(chunk);
        zeros_run_result = zeros_run_compression(chunk);
        bdi_ze_result = bdi_ze_compression(chunk);

        original_size += chunksize;
        bdi_size += bdi_result.compressed.size;
        fpc_size += fpc_result.compressed.size;
        bdi_twobase_size += bdi_twobase_result.compressed.size;
        bdi_zr_size += bdi_zr_result.compressed.size;
        zero_vec_size += zero_vec_result.compressed.size;
        zeros_run_size += zeros_run_result.compressed.size;
        bdi_ze_size += bdi_ze_result.compressed.size;

#ifdef ORIGINAL_BDI
        bdi_original_result = bdi_original_compression(chunk);
        bdi_original_size += bdi_original_result;
#endif

#ifdef VERBOSE
        printf("[SEARCH %d] offset: %dBytes  size: %dBytes\n", i/chunksize, i, chunksize);
        printf("original: ");
        print_memory_chunk(chunk);
        printf("\n");
        printf("bdi size: %dBytes  fpc size: %dBytes  bdi twobase size: %dBytes  bdi zr size: %dBytes  zero vec size: %dBytes  zeros run size: %dBytes  bdi ze size: %dBytes\n", 
                bdi_result.compressed.size, 
                fpc_result.compressed.size,
                bdi_twobase_result.compressed.size,
                bdi_zr_result.compressed.size,
                zero_vec_result.compressed.size,
                zero_vec_result.compressed.size,
                bdi_ze_result.compressed.size);
#endif

#ifdef ORIGINAL_BDI
        printf("original bdi size: %dBytes\n\n", bdi_original_result);
#endif
#ifndef ORIGINAL_BDI
        printf("\n");
#endif

#ifndef VERBOSE
        printf("[ITER %2d] offset: %dBytes  size: %dBytes\n", iter+1, i, chunksize);
#endif

        remove_memory_chunk(chunk);
        remove_compression_result(bdi_result);
        remove_compression_result(fpc_result);
        remove_compression_result(bdi_twobase_result);
        remove_compression_result(bdi_zr_result);
        remove_compression_result(zero_vec_result);
        remove_compression_result(zeros_run_result);
        remove_compression_result(bdi_ze_result);

        iter += 1;
    }

    printf("\n====================\n");
    printf("compression ratio: %.4f(BDI) %.4f(FPC) %.4f(BDI 2B) %.4f(BDI ZR) %.4f(ZERO VEC) %.4f(ZEROS RUN) %.4f(BDI ZE)\n", 
            (double)original_size / bdi_size, 
            (double)original_size / fpc_size, 
            (double)original_size / bdi_twobase_size,
            (double)original_size / bdi_zr_size,
            (double)original_size / zero_vec_size,
            (double)original_size / zeros_run_size,
            (double)original_size / bdi_ze_size);
    printf("original size  (%3dBytes)\n", original_size);
    printf("BDI size       (%3dBytes)\n", bdi_size);
    printf("FPC size       (%3dBytes)\n", fpc_size);
    printf("BDI 2B size    (%3dBytes)\n", bdi_twobase_size);
    printf("BDI ZR size    (%3dBytes)\n", bdi_zr_size);
    printf("ZERO VEC size  (%3dBytes)\n", zero_vec_size);
    printf("ZEROS RUN size (%3dBytes)\n", zeros_run_size);
    printf("BDI ZE size    (%3dBytes)\n", bdi_ze_size);
    printf("====================\n");

#ifdef ORIGINAL_BDI
    printf("\n====================\n");
    printf("compression ratio: %.4f(BDI ORIG)\n", (double)original_size / bdi_original_size);
    printf("BDI ORIG       (%3dBytes)\n", bdi_original_size);
    printf("====================\n");
#endif

    return 0;
}