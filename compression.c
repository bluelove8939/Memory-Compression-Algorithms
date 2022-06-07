#include "compression.h"

/* 
 * Functions for managing ByteArr and ValueBuffer 
 *   ByteArr is literally a structure for byte array representing memory block.
 *   ValueBuffer is 8Byte buffer for reading and writing value to ByteArr.
 * 
 * Functions:
 *   set_value: set value of the byte array
 *   set_value_bitwise: set value of the byte array with bitwise offset and size argument
 *   get_value: get value of the byte array
 *   get_value_bitwise: get value of the byte array with bitwise offset and size argument
 */
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


/* 
 * Functions for managing MemoryChunk
 *   MemoryChunk is a structure for representing memory blocks e.g. cache line and DRAM.
 *   Functions below are for managing those memory blocks.
 * 
 * Functions:
 *   make_memory_chunk: makes an array with its given size and bitwidth
 *   copy_memory_chunk: copy a given memory chunk
 *   remove_memory_chunk: removes memory chunk
 *   remove_compression_result: removes compression result (compressed memory chunk and tag overhead)
 *   remove_decompression_result: removes decompression result (original memory chunk)
 *   print_memory_chunk: print content stored in the memory chunk in hexadecimal form
 *   print_memory_chunk_bitwise: print content stored in the memory chunk in binary form
 *   print_compression_result: prints compression result
 *   print_decompression_result: prints decompression result
 */
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
    if (chunk.size % 4 != 0) {
        for (int i = 0; i < 4 - (chunk.size % 4); i++) {
            printf("--");
        }
    }

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
    printf("type: %s\n", result.compression_type);
    printf("original   size: %dBytes\n", result.original.size);
    printf("compressed size: %dBytes\n", result.compressed.size);
    printf("valid bitwidth:  %dbits\n", result.compressed.valid_bitwidth);
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
    printf("type: %s\n", result.compression_type);
    printf("compressed size: %dBytes\n", result.compressed.size);
    printf("original   size: %dBytes\n", result.original.size);
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
 *   BDI(Base Delta Immediate) algorithm is a compression algorithm usually used to compress
 *   memory blocks e.g. cache line.
 *
 * Functions:
 *   bdi_compression: BDI compression algorithm
 *   bdi_decompression : BDI decompression algorithm
 *   bdi_compressing_unit: actually compresses given cacheline with certain encoding
 * 
 * Note
 *   This algorithm is reference to the paper of PACT12 conference
 *   url: https://users.ece.cmu.edu/~omutlu/pub/bdi-compression_pact12.pdf
 */

CompressionResult bdi_compression(CacheLine original) {
    CompressionResult result;
    MetaData tag_overhead = make_memory_chunk(2, 0);  // 2Bytes of tag overhead with its valid bitwidth of 11bits
    CacheLine compressed;
    ValueBuffer segment_num;

#ifdef VERBOSE
    printf("Compressing with BDI algorithm...\n");
#endif

    result.compression_type = "BDI(Base Delta Immediate)";
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
    printf("Decompressing with BDI algorithm...\n");
    printf("encoding: %d\n", encoding);
    printf("segment pointer: %d\n", segment_num);
#endif

    result.compressed = compressed;
    result.compression_type = "BDI(Base Delta Immediate)";

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
 *   FPC(Frequent Pattern Compression) is an algorithm used to compress memory block by
 *   encoding frequent memory pattern into smaller codeword. 
 * 
 * Functions:
 *   fpc_compression: FPC compression algorithm
 */

CompressionResult fpc_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size * 2, 0);
    MetaData tag_overhead = make_memory_chunk(1, 0);
    ValueBuffer buffer, mask = 1;
    Bool compressed_flag, repeating_flag;
    Bool zeros_flag = FALSE;
    Bool initial, sign_extended_flag;
    char initial_byte;
    int extended_until;
    int pivot = 0;
    int zeros_len = 0;

#ifdef VERBOSE
    printf("Compressing with FPC algorithm...\n");
#endif

    result.compression_type = "FPC(Frequent Pattern Compression";
    result.original = original;
    tag_overhead.valid_bitwidth = 0;
    result.tag_overhead = tag_overhead;

    for (int i = 0; i < original.size;) {
#ifdef VERBOSE
        printf("[ITER] cursor position: %d  pivot: %d\n", i, pivot);
        printf("prefix 0 (zero run): ");
#endif
        for (zeros_len = 0; (get_value(original.body, i + zeros_len, 1) & 0xff) == 0x00 && zeros_len < 8; zeros_len++) {}
        if (zeros_len > 0) {
#ifdef VERBOSE
            printf("succeed (len: %d)\n", zeros_len);
#endif
            i += zeros_len;
            set_value_bitwise(compressed.body, 0, pivot, 3);
            set_value_bitwise(compressed.body, zeros_len-1, pivot+3, 3);
            pivot += 6;
            continue;
        }

        compressed_flag = FALSE;
        buffer = get_value(original.body, i, 4);
        i += 4;

        for (int prefix = 1; prefix < 8 && compressed_flag == FALSE; prefix++) {
            switch (prefix) {
            case 1:
            case 2:
            case 3:
#ifdef VERBOSE
                printf("failed\n");
                printf("buffer = 0x%016x\n", buffer);
                printf("prefix 1 or 2 or 3: ");
#endif
                initial = ((buffer & mask << WORDSIZ * BYTE_BITWIDTH - 1) != 0);
                for (extended_until = WORDSIZ * BYTE_BITWIDTH - 1; extended_until >= 0; extended_until--) {
                    if (initial != ((buffer & mask << extended_until) != 0))
                        break;
                }

                if (extended_until < 4) {  // if LSB 4bits are sign-extended
                    set_value_bitwise(compressed.body, 1, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 4);
                    pivot += 7;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed (prefix 1)\n");
#endif
                } else if (extended_until < 8) {  // if LSB 8bits are sign-extended
                    set_value_bitwise(compressed.body, 2, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 8);
                    pivot += 11;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed (prefix 2)\n");
#endif
                } else if (extended_until < 16) {  // if LSB 16bits are sign-extended
                    set_value_bitwise(compressed.body, 3, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 16);
                    pivot += 19;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed (prefix 3)\n");
#endif
                }

                break;
            
            case 4:
#ifdef VERBOSE
                printf("failed\n");
                printf("prefix 4: ");
#endif
                initial = ((buffer & mask) != 0);
                for (extended_until = 0; extended_until < WORDSIZ * BYTE_BITWIDTH; extended_until++) {
                    if (initial != ((buffer & mask << extended_until) != 0))
                        break;
                }

                if (extended_until >= 16) {  // if MSB 16bits are sign-extended
                    set_value_bitwise(compressed.body, 4, pivot, 3);
                    set_value_bitwise(compressed.body, buffer, pivot+3, 16);
                    pivot += 19;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;

            case 5:
#ifdef VERBOSE
                printf("failed\n");
                printf("prefix 5: ");
#endif
                if (buffer & 0xffff == (buffer & 0xffff0000) >> (HWORDSIZ * BYTE_BITWIDTH)) {
                    set_value_bitwise(compressed.body, 5, pivot, 3);
                    set_value_bitwise(compressed.body, buffer & 0xffff, pivot+3, 16);
                    pivot += 19;
                    compressed_flag = TRUE;
                }

                if (compressed_flag) {
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;

            case 6:
#ifdef VERBOSE
                printf("failed\n");
                printf("prefix 6: ");
#endif
                repeating_flag = TRUE;
                initial_byte = buffer & 0xff;
                for (int j = 0; j < 4; j++) {
                    if (buffer & (0xff << (j * BYTE_BITWIDTH)) >> (j * BYTE_BITWIDTH) != (ValueBuffer)initial_byte) {
                        repeating_flag = FALSE;
                    }
                }
                
                if (repeating_flag) {
                    set_value_bitwise(compressed.body, 6, pivot, 3);
                    set_value_bitwise(compressed.body, (ValueBuffer)initial_byte, pivot+3, 8);
                    pivot += 11;
                    compressed_flag = TRUE;
                }

                if (compressed_flag) {
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;
            
            case 7:
#ifdef VERBOSE
                printf("failed\n");
                printf("failed to compressed buffer value 0x%08x", buffer);
#endif
                set_value_bitwise(compressed.body, 7, pivot, 3);
                set_value_bitwise(compressed.body, buffer, pivot+3, 32);
                pivot += 35;
                compressed_flag = TRUE;
                
                break;

            default:
                break;
            }
            
        }
    }

    int compressed_size = ceil((double)pivot / BYTE_BITWIDTH);

    if (pivot < original.size) {
        compressed.size = compressed_size;
        compressed.valid_bitwidth = pivot;
        result.compressed = compressed;
        result.is_compressed = TRUE;
    } else {
        result.compressed = copy_memory_chunk(original);
        result.is_compressed = FALSE;
    }

#ifdef VERBOSE
    printf("compression completed\n");
#endif
    
    return result;
}