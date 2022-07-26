#include <stdio.h>
#include <string.h>

#include "compression.h"
#include "bdi_zerovec.h"

// Verbose parameter
// #define VERBOSE  // Comment this line not to display debug messages

// Number of algorithms in test
#define ALGO_NUM         8
#define FILENAME_BUFSIZ  2048


int main(int argc, char const *argv[]) {
    MemoryChunk chunk;
    CompressionResult result;
    int chunksize, iter, maxiter = 500, filesize;

    char datafilename[FILENAME_BUFSIZ];
    char const *filename;
    char const *logfilename = "./logs/comparison.csv";

    char *algo_names[ALGO_NUM] = {"BDI", "FPC", "BDI 2B", "BDI+ZR", "ZeroVec", "ZerosRun", "BDI+ZE", "BDI+ZV"};
    int   algo_sizes[ALGO_NUM];
    int   original_size;

    CompressionResult (*algo_funcs[ALGO_NUM]) (CacheLine original) = {
        bdi_compression,          // BDI
        fpc_compression,          // FPC
        bdi_twobase_compression,  // BDI with two bases
        bdi_zr_compression,       // BDI with zeros run
        zero_vec_compression,     // Zero Vector
        zeros_run_compression,    // Zeros Run
        bdi_ze_compression,       // BDI with zero encoding
        bdi_zv_compression,       // BDI with zero vector
    };

    if (argc > 2) {
        filename = argv[1];
        chunksize = atoi(argv[2]);
    } else {
        fprintf(stderr, "[ERROR] Insufficient number of line arguments (filename and memory chunk size is required\n");
        exit(-1);
    }

    if (argc > 3)
        maxiter = atoi(argv[3]);
    if (argc > 4)
        logfilename = argv[4];

    FILE *filelistfp = fopen(filename, "rt");
    FILE *logfilefp = fopen(logfilename, "wt");

    fprintf(logfilefp, "%s", "Layer Name");
    for (int i = 0 ; i < ALGO_NUM; i++) {
        fprintf(logfilefp, ",%s", algo_names[i]);
    }
    fprintf(logfilefp, "\n");

    while (fgets(datafilename, FILENAME_BUFSIZ-1, filelistfp)) {
        if (datafilename[strlen(datafilename)-1] == '\n')
            datafilename[strlen(datafilename)-1] = 0;

        FILE *fp = fopen(datafilename, "rb");
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fclose(fp);

        printf("Reading %s (filesize: %dBytes)\n", datafilename, filesize);

        for (int i = 0; i < ALGO_NUM; i++)
            algo_sizes[i] = 0;

        iter = 0;
        original_size = 0;

        printf("debugging\n");

        for (int i = 0; (i < filesize) && (iter < maxiter); i += chunksize) {
            chunk = file2memorychunk(datafilename, i, chunksize);
#ifdef VERBOSE
            printf("original: ");
            print_memory_chunk(chunk);
            printf("\n");
#endif
            for (int j = 0; j < ALGO_NUM; j++) {
                result = algo_funcs[j](chunk);
                algo_sizes[j] += result.compressed.size;
#ifdef VERBOSE
                printf("%8s size: %dBytes  result: ", algo_names[j], result.compressed.size);
                print_memory_chunk(result.compressed);
                printf("\n");
#endif
                remove_compression_result(result);
            }
#ifdef VERBOSE
            printf("\n");
#endif
#ifndef VERBOSE
            printf("\r[ITER %2d] offset: %dBytes  size: %dBytes", iter+1, i, chunksize);
#endif
            printf("debugging\n");
            remove_memory_chunk(chunk);
            iter += 1;
            original_size += chunksize;
        }

        printf("\ncompression ratio: ");
        fprintf(logfilefp, "%s", datafilename);
        for (int i = 0; i < ALGO_NUM; i++) {
            printf("%.4f(%s) ", (double)original_size / algo_sizes[i], algo_names[i]);
            fprintf(logfilefp, ",%.4f", (double)original_size / algo_sizes[i]);
        }
        printf("\n\n");
        fprintf(logfilefp, "\n");
    }

    fclose(filelistfp);
    fclose(logfilefp);

    return 0;
}