#include "compression.h"

// Functions for managing ByteArr and ValueBuffer
void set_value(ByteArr arr, ValueBuffer val, int offset, int size) {
    ValueBuffer mask = 1;
    for (int i = 0; i < size * BYTE_BITWIDTH; i++) {
        arr[(i / BYTE_BITWIDTH) + offset] |= ((((mask << i) & val) >> ((i / BYTE_BITWIDTH) * BYTE_BITWIDTH)));
    }
}

void set_value_bitwise(ByteArr arr, ValueBuffer val, int offset, int size) {
    ValueBuffer mask = 1;
    for (int i = 0; i < size; i++) {
        arr[(i + offset) / BYTE_BITWIDTH] |= ((mask << i) & val) << (((offset + i) % 8) - i);
    }
}

ValueBuffer get_value(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    for (int i = 0; i < size; i++) {
        val |= ((ValueBuffer)arr[offset + i] << (BYTE_BITWIDTH * i));
    }
    return val;
}

ValueBuffer get_value_bitwise(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    ValueBuffer mask = 1;
    for (int i = offset; i < offset + size; i++) {
        val |= ((ValueBuffer)arr[i / BYTE_BITWIDTH] & (mask << (i % BYTE_BITWIDTH))) >> (i % BYTE_BITWIDTH) - (i-offset);
    }
    return val;
}


// Functions for managing MemoryChunk
MemoryChunk make_memory_chunk(int size, int initial) {
    MemoryChunk result;
    result.size = size;
    result.valid_bitwidth = size * BYTE_BITWIDTH;  // default valid bitwidth = size * 8
    result.body = (char *)malloc(size);
    memset(result.body, initial, size);
    return result;
}

MemoryChunk copy_memory_chunk(MemoryChunk target) {
    MemoryChunk chunk;
    chunk.size = target.size;
    chunk.body = (char *)malloc(target.size);
    memcpy(chunk.body, target.body, chunk.size);
    return chunk;
}

void remove_memory_chunk(MemoryChunk chunk) {
    free(chunk.body);
}

void remove_compression_result(CompressionResult result) {
    remove_memory_chunk(result.compressed);
    remove_memory_chunk(result.tag_overhead);
}

void remove_decompression_result(DecompressionResult result) {
    remove_memory_chunk(result.original);
}

void print_memory_chunk(MemoryChunk chunk) {
    for (int i = chunk.size-1; i >= 0; i--) {
        printf("%02x", chunk.body[i] & 0xff);
        if (i % 4 == 0) printf(" ");
    }
}

void print_memory_chunk_bitwise(MemoryChunk chunk) {
    Byte buffer, mask = 1;

    for (int i = chunk.valid_bitwidth-1; i >= 0; i--) {
        buffer = chunk.body[i / BYTE_BITWIDTH];
        printf("%d", (buffer & mask << (i % BYTE_BITWIDTH)) != 0);
    }
}

void print_compression_result(CompressionResult result) {
    Byte buffer, mask = 1;

    printf("======= Compression Results =======\n");
    printf("original size:     %dBytes\n", result.original.size);
    printf("compressed size:   %dBytes\n", result.compressed.size);
    printf("compression ratio: %.4f\n", (double)result.original.size / result.compressed.size);
    printf("original:   ");
    print_memory_chunk(result.original);
    printf("\n");
    printf("compressed: ");
    print_memory_chunk(result.compressed);
    printf("\n");
    printf("tag overhead: ");
    print_memory_chunk_bitwise(result.tag_overhead);
    printf(" (%dbits)\n", result.tag_overhead.valid_bitwidth);
    printf("is compressed: %s\n", result.is_compressed ? "true" : "false");
    printf("===================================\n");
}

void print_decompression_result(DecompressionResult result) {
    Byte buffer, mask = 1;

    printf("====== Decompression Results ======\n");
    printf("compressed size:   %dBytes\n", result.compressed.size);
    printf("original size:     %dBytes\n", result.original.size);
    printf("compressed: ");
    print_memory_chunk(result.compressed);
    printf("\n");
    if (result.is_decompressed) {
        printf("original:   ");
        print_memory_chunk(result.original);
        printf("\n");
    }
    printf("is decompressed: %s\n", result.is_decompressed ? "true" : "false");
    printf("===================================\n");
}


/* 
 * Functions for BDI algorithm
 *   bdi_compression: BDI compression algorithm
 *   bdi_decompression : BDI decompression algorithm
 *   bdi_compressing_unit: actually compresses given cacheline with certain encoding
 * 
 * Note
 *   This code snippet is written with its reference to the paper from PACT12 conference
 *   url: https://users.ece.cmu.edu/~omutlu/pub/bdi-compression_pact12.pdf
 */

CompressionResult bdi_compression(CacheLine original) {
    CompressionResult result;
    MetaData tag_overhead = make_memory_chunk(2, 0);  // 2Bytes of tag overhead with its valid bitwidth of 11bits
    CacheLine compressed;
    ValueBuffer segment_num;

    result.original = original;

    for (int encoding = 0; encoding < 8; encoding++) {
        compressed = bdi_compressing_unit(original, encoding);
        if (compressed.size < original.size) {
            result.compressed = compressed;
            result.is_compressed = TRUE;
            segment_num = ceil((double)compressed.size / BYTE_BITWIDTH);
            set_value_bitwise(tag_overhead.body, encoding, 0, 4);     // encoding        (0-3 bits)
            set_value_bitwise(tag_overhead.body, segment_num, 4, 7);  // segment pointer (4-11bits)
            tag_overhead.valid_bitwidth = 11;   // tag_overhead = {encoding(4bits), segment_pointer(7bits)}
            result.tag_overhead = tag_overhead;
            return result;
        }
    }

    result.compressed = copy_memory_chunk(original);
    result.is_compressed = FALSE;
    set_value_bitwise(tag_overhead.body, 15, 0, 4);
    segment_num = ceil((double)original.size / BYTE_BITWIDTH);
    set_value_bitwise(tag_overhead.body, segment_num, 4, 7);
    tag_overhead.valid_bitwidth = 11;
    result.tag_overhead = tag_overhead;
    return result;
}

DecompressionResult bdi_decompression(CacheLine compressed, MetaData tag_overhead, int original_size) {
    CacheLine original = make_memory_chunk(original_size, 0);
    DecompressionResult result;
    ValueBuffer base, buffer;
    int k, d;
    int encoding = get_value_bitwise(tag_overhead.body, 0, 4);
    int segment_num = get_value_bitwise(tag_overhead.body, 4, 7);

#ifdef VERBOSE
    printf("encoding: %d\n", encoding);
    printf("segment pointer: %d\n", segment_num);
#endif

    result.compressed = compressed;

    switch (encoding) {
    case 0:  // Zero values
#ifdef VERBOSE
        printf("Zeros compression (encoding: %d)\n", encoding);
#endif
        for (int i = 0; i < original.size; i++) {
            set_value(original.body, 0, i, 1);
        }
        result.original = original;
        result.is_decompressed = TRUE;
#ifdef VERBOSE
        printf("decompression completed\n");
#endif
        return result;
    
    case 1:  // Repeated values
#ifdef VERBOSE
        printf("Repeated values compression (encoding: %d)\n", encoding);
#endif
        base = get_value(compressed.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
            set_value(original.body, base, i, 8);
        }
        result.original = original;
        result.is_decompressed = TRUE;
#ifdef VERBOSE
        printf("compression completed\n");
#endif
        return result;

    case 2:  // Base8-delta1
        k = 8; 
        d = 1;
        break;
    
    case 3:  // Base8-delta2
        k = 8; 
        d = 2;
        break;

    case 4:  // Base8-delta4
        k = 8; 
        d = 4;
        break;

    case 5:  // Base4-delta1
        k = 4; 
        d = 1;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 2; 
        d = 1;
        break;
    
    default:
        result.original = original;
        result.is_decompressed = FALSE;
        return result;
    }

    base = get_value(compressed.body, 0, k);
    for (int i = 0; i < original.size / k; i++) {
        buffer = get_value(compressed.body, k + (d*i), d);
        set_value(original.body, buffer + base, i * k, k);
    }
    result.original = original;
    result.is_decompressed = TRUE;
    return result;
}

CacheLine bdi_compressing_unit(CacheLine original, int encoding) {
    ValueBuffer base = 0;
    ValueBuffer buffer, delta, mask;
    int k, d;
    int compressed_siz = 0;
    Bool initial, flag = TRUE;
    CacheLine result;

    // 0. Select mode by given encoding (MUX)
    switch (encoding) {
    case 0:  // Zero values
#ifdef VERBOSE
        printf("Zeros compression (encoding: %d)\n", encoding);
#endif
        for (int i = 0; i < original.size; i++) {
#ifdef VERBOSE
            printf("[ITER %2d] base: 0x%016x  buffer: 0x%016x\n", i, 0, original.body[i]);
#endif
            if (original.body[i] != 0) {
#ifdef VERBOSE
                printf("iteration terminated (not compressible)\n");
#endif
                result = copy_memory_chunk(original);
                return result;
            }
        }
#ifdef VERBOSE
        printf("compression completed\n");
#endif
        result = make_memory_chunk(1, 0);
        return result;
    
    case 1:  // Repeated values
#ifdef VERBOSE
        printf("Repeated values compression (encoding: %d)\n", encoding);
#endif
        base = get_value(original.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
#ifdef VERBOSE
            printf("[ITER %2d] base: 0x%016x  buffer: 0x%016x\n", i, base, get_value(original.body, i, 8));
#endif
            if (get_value(original.body, i, 8) != base) {
#ifdef VERBOSE
                printf("iteration terminated (not compressible)\n");
#endif
                result = copy_memory_chunk(original);
                return result;
            }
        }
#ifdef VERBOSE
        printf("compression completed\n");
#endif
        result = make_memory_chunk(8, 0);
        set_value(result.body, base, 0, 8);
        return result;

    case 2:  // Base8-delta1
        k = 8; 
        d = 1;
        break;
    
    case 3:  // Base8-delta2
        k = 8; 
        d = 2;
        break;

    case 4:  // Base8-delta4
        k = 8; 
        d = 4;
        break;

    case 5:  // Base4-delta1
        k = 4; 
        d = 1;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 2; 
        d = 1;
        break;
    
    default:
        k = 8; 
        d = 1;
        break;
    }

#ifdef VERBOSE
    printf("Base%d-Delta%d compression (encoding: %d)\n", k, d, encoding);
#endif
    result = make_memory_chunk(original.size, 0);

    // 1. Find out base value
    base = get_value(original.body, 0, k);
    set_value(result.body, base, 0, k);
    compressed_siz = k;

    // 2. Calculate delta values and check if this cache line is compressible
    for (int i = 0; i < original.size; i += k) {
        // 2-1. Split byte array into k-byte array and extract each value
        buffer = get_value(original.body, i, k);
        delta = buffer - base;

#ifdef VERBOSE
        printf("[ITER %2d] base: 0x%016x  buffer: 0x%016x  delta: 0x%016x\n", i/k, base, buffer, delta);
#endif

        // 2-2. Check whether calculated delta value is sign-extended within d-Bytes
        mask = 1;
        initial = (delta & mask << (sizeof(delta) * BYTE_BITWIDTH - 1)) != 0;
        
        for (int j = sizeof(delta) * 8 - 1; j >= d * 8 - 1; j--) {
            if (initial != ((delta & mask << j) != 0)) {
                flag = FALSE;
                break;
            }
        }

        // 2-3 Copy shrinked delta value to compressed memory block
        if (flag == FALSE) {
#ifdef VERBOSE
            printf("iteration terminated (not compressible)\n");
#endif
            remove_memory_chunk(result);
            result = copy_memory_chunk(original);
            return result;
        } else {
            mask = 1;
            set_value(result.body, delta, compressed_siz, d);
            compressed_siz += d;
        }
    }

#ifdef VERBOSE
    printf("compression completed\n");
#endif

    result.size = compressed_siz;
    return result;
}


/* 
 * Functions for FPC algorithm
 *   fpc_compression: FPC compression algorithm
 */

CompressionResult fpc_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size, 0);
    ValueBuffer buffer, mask = 1;
    Bool is_compressible = TRUE;
    Bool zeros_flag = FALSE;
    Bool initial, sign_extended_flag;
    int extended_until;
    int pivot = 0;
    int zeros_len = 0;

    for (int i = 0; i < original.size; i += 4) {
        buffer = get_value(original.body, i, 4);
        if (zeros_flag) {
            if (buffer == 0 && zeros_len < 2) {
                zeros_len++;
            } else {
                set_value_bitwise(compressed.body, 0, pivot, 3);
                set_value_bitwise(compressed.body, zeros_len * WORDSIZ - 1, pivot+3, 3);
                pivot += 6;
            }
        }

        for (int prefix = 1; prefix < 8; prefix++) {
            switch (prefix) {
            case 1:
            case 2:
            case 3:
                for (extended_until = WORDSIZ * BYTE_BITWIDTH - 1; extended_until >= 0; extended_until--) {
                    if (initial != ((buffer & mask << extended_until) != 0))
                        break;
                }

                if (extended_until < 4) {  // if LSB 4bits are sign-extended
                    set_value_bitwise(compressed.body, 1, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 4);
                    pivot += 7;
                } else if (extended_until < 8) {  // if LSB 8bits are sign-extended
                    set_value_bitwise(compressed.body, 2, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 8);
                    pivot += 11;
                } else if (extended_until < 16) {  // if LSB 16bits are sign-extended
                    set_value_bitwise(compressed.body, 3, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 16);
                    pivot += 19;
                }

                break;
            
            case 4:
                break;

            case 5:
                break;

            case 6:
                break;
            
            case 7:
                break;

            default:
                break;
            }
            
        }
    }

    return result;
}