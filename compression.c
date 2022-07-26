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
        arr[(i + offset) / BYTE_BITWIDTH] |= ((val & (mask << i)) >> i) << ((i + offset) % BYTE_BITWIDTH);
    }
}

ValueBuffer get_value(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    for (int i = 0; i < size; i++) {
        val += ((ValueBuffer)arr[offset + i] << (i * BYTE_BITWIDTH));
    }
    if (size == 8) return val;
    return SIGNEX(val, size * BYTE_BITWIDTH - 1);
}

ValueBuffer get_value_bitwise(ByteArr arr, int offset, int size) {
    ValueBuffer val = 0;
    ValueBuffer mask = 1;
    int block_idx, block_bit_idx, output_bit_idx;
    for (int i = 0; i < size; i++) {
        block_idx = (i + offset) / BYTE_BITWIDTH;
        block_bit_idx = (i + offset) % BYTE_BITWIDTH;
        output_bit_idx = i;
        val += ((ValueBuffer)arr[block_idx] & (mask << (block_bit_idx))) >> block_bit_idx << output_bit_idx;
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
    chunk.valid_bitwidth = target.valid_bitwidth;
    chunk.body = (ByteArr)malloc(target.size);
    memcpy(chunk.body, target.body, target.size);
    // printf("copy result - size: %dBytes valid: %dbits content: ", chunk.size, chunk.valid_bitwidth);
    // print_memory_chunk(chunk);
    // printf("\n");
    return chunk;
}

void remove_memory_chunk(MemoryChunk chunk) {
    free(chunk.body);
}

void remove_compression_result(CompressionResult result) {
    remove_memory_chunk(result.compressed);
    remove_memory_chunk(result.tag_overhead);
    result.compressed = NULL;
    result.tag_overhead = NULL;
}

void remove_decompression_result(DecompressionResult result) {
    remove_memory_chunk(result.original);
    result.original = NULL;
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
    // printf("compressed(bitwise)\n");
    // print_memory_chunk_bitwise(result.compressed);
    // printf("\n");
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
 * Functions for reading and writing cacheline file externally
 * 
 * Functions:
 *   file2cacheline: convert binary file to cacheline
 */

MemoryChunk file2memorychunk(char const *filename, int offset, int size) {
#ifdef VERBOSE
    printf("reading file \'%s\'...\n", filename);
    printf("offset: %d  size: %d\n", offset, size);
#endif

    MemoryChunk chunk = make_memory_chunk(size, 0);
    char *buffer = (char *)malloc(size * sizeof(char));
    FILE *fp = fopen(filename, "rb");

    fseek(fp, offset, SEEK_SET);

    if (fp) {
        fread(buffer, size, sizeof(char), fp);
    } else {
#ifdef VERBOSE
        printf("opening file \'%s\' failed\n", filename);
#endif
    }

    memcpy(chunk.body, buffer, size);
    free(buffer);

#ifdef VERBOSE
    printf("reading completed\n");
    printf("chunk: ");
    print_memory_chunk(chunk);
    printf("\n");
#endif

    fclose(fp);
    return chunk;
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
    ValueBuffer base, buffer, mask=1;
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
        k = 4; 
        d = 1;
        break;

    case 4:  // Base8-delta4
        k = 2; 
        d = 1;
        break;

    case 5:  // Base4-delta1
        k = 8; 
        d = 2;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 8; 
        d = 4;
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
            printf("[ITER %2d] base: 0x%02x  buffer: 0x%02x\n", i, 0, original.body[i]);
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
            printf("[ITER %2d] base: 0x%016llx  buffer: 0x%016llx\n", i, base, get_value(original.body, i, 8));
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
        k = 4; 
        d = 1;
        break;

    case 4:  // Base8-delta4
        k = 2; 
        d = 1;
        break;

    case 5:  // Base4-delta1
        k = 8; 
        d = 2;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 8; 
        d = 4;
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
        printf("[ITER %2d] base: 0x%016llx  buffer: 0x%016llx  delta: 0x%016llx\n", i/k, base, buffer, delta);
#endif

        // 2-2. Check whether calculated delta value is sign-extended within d-Bytes
        ValueBuffer byte_mask = 0x00;
        if (d >= 1) byte_mask += 0xff;
        if (d >= 2) byte_mask += 0xff00;
        if (d >= 4) byte_mask += 0xffff0000;
        if (delta != SIGNEX(delta & byte_mask, (d * BYTE_BITWIDTH) - 1)) {
            flag = FALSE;
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
 *   fpc_decompression: FPC decompression algorithm
 */

CompressionResult fpc_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size * 2, 0);
    MetaData tag_overhead = make_memory_chunk(4, 0);
    WordBuffer buffer, mask = 1;
    HwordBuffer lsb, msb;
    Bool compressed_flag, repeating_flag;
    Bool initial;
    ByteBuffer initial_byte;
    int pivot = 0;
    int zeros_len = 0;
    int tag_overhead_width = 0;


#ifdef VERBOSE
    printf("Compressing with FPC algorithm...\n");
#endif

    result.compression_type = "FPC(Frequent Pattern Compression";
    result.original = original;
    

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
            set_value_bitwise(tag_overhead.body, 0, tag_overhead_width, 3);
            set_value_bitwise(compressed.body, zeros_len-1, pivot, 3);
            tag_overhead_width += 3;
            pivot += 3;
            continue;
        }

        compressed_flag = FALSE;
        buffer = get_value(original.body, i, 4);
        i += 4;

        for (int prefix = 1; prefix < 8 && compressed_flag == FALSE; prefix++) {
            switch (prefix) {
            case 1:
#ifdef VERBOSE
                printf("failed\n");
                printf("buffer = 0x%08x\n", buffer);
                printf("prefix 1: ");
#endif
                if (buffer == SIGNEX(buffer & 0b1111, 3)) {  // if LSB 4bits are sign-extended
                    set_value_bitwise(tag_overhead.body, 1, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, buffer, pivot, 4);
                    tag_overhead_width += 3;
                    pivot += 4;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;

            case 2:
#ifdef VERBOSE
                // printf("failed\n");
                printf("failed\n");
                printf("prefix 2: ");
#endif
                if (buffer == (ByteBuffer)(buffer & 0xff)) {  // if LSB 8bits are sign-extended
                    set_value_bitwise(tag_overhead.body, 2, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, buffer, pivot, 8);
                    tag_overhead_width += 3;
                    pivot += 8;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;

            case 3:
#ifdef VERBOSE
                // printf("failed\n");
                printf("failed\n");
                printf("prefix 3: ");
#endif
                if (buffer == (HwordBuffer)(buffer & 0xffff)) {  // if LSB 16bits are sign-extended
                    set_value_bitwise(tag_overhead.body, 3, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, buffer, pivot, 16);
                    tag_overhead_width += 3;
                    pivot += 16;
                    compressed_flag = TRUE;
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;
            
            case 4:
#ifdef VERBOSE
                printf("failed\n");
                printf("prefix 4: ");
#endif
                if ((buffer & 0xffff0000) == 0x0000) {
                    set_value_bitwise(tag_overhead.body, 4, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, buffer & 0x0000ffff, pivot, 16);
                    tag_overhead_width += 3;
                    pivot += 16;
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
                lsb =  buffer & 0xffff;
                msb = (buffer & 0xffff0000) >> (2 * BYTE_BITWIDTH);

                if (lsb == (ByteBuffer)(lsb & 0xff) && msb == (ByteBuffer)(msb & 0xff)) {
                    set_value_bitwise(tag_overhead.body, 5, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, (lsb & 0xff) + (msb & 0xff00), pivot, 16);
                    tag_overhead_width += 3;
                    pivot += 16;
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
                compressed_flag = FALSE;
                if (((ByteBuffer)(buffer & 0x000000ff) == (ByteBuffer)((buffer & 0x0000ff00) >> (BYTESIZ * BYTE_BITWIDTH * 1))) &&
                    ((ByteBuffer)(buffer & 0x000000ff) == (ByteBuffer)((buffer & 0x00ff0000) >> (BYTESIZ * BYTE_BITWIDTH * 2))) &&
                    ((ByteBuffer)(buffer & 0x000000ff) == (ByteBuffer)((buffer & 0xff000000) >> (BYTESIZ * BYTE_BITWIDTH * 3)))) {
                    compressed_flag = TRUE;
                }
                
                if (compressed_flag) {
                    set_value_bitwise(tag_overhead.body, 6, tag_overhead_width, 3);
                    set_value_bitwise(compressed.body, buffer & 0xff, pivot, 8);
                    tag_overhead_width += 3;
                    pivot += 8;
#ifdef VERBOSE
                    printf("succeed\n");
#endif
                }

                break;
            
            case 7:
#ifdef VERBOSE
                printf("failed\n");
                printf("prefix 7: ");
#endif
                set_value_bitwise(tag_overhead.body, 7, tag_overhead_width, 3);
                set_value_bitwise(compressed.body, buffer, pivot, 32);
                tag_overhead_width += 3;
                pivot += 32;
                compressed_flag = TRUE;
#ifdef VERBOSE
                printf("succeed\n");
#endif              
                break;

            default:
                break;
            }
        }
    }

    int compressed_size = ceil((double)pivot / BYTE_BITWIDTH);

    if (compressed_size < original.size) {
        compressed.size = compressed_size;
        compressed.valid_bitwidth = pivot;
        result.compressed = compressed;
        result.is_compressed = TRUE;
        tag_overhead.valid_bitwidth = tag_overhead_width;
        result.tag_overhead = tag_overhead;
    } else {
#ifdef VERBOSE
        printf("compression failed (compressed size: %dBytes)\n", compressed_size);
#endif
        result.compressed = copy_memory_chunk(original);
        result.is_compressed = FALSE;
        tag_overhead.valid_bitwidth = 0;
        result.tag_overhead = tag_overhead;
    }

#ifdef VERBOSE
    printf("compression completed\n");
#endif
    
    return result;
}

DecompressionResult fpc_decompression(CacheLine compressed, MetaData tag_overhead, int original_size) {
    DecompressionResult result;
    CacheLine original = make_memory_chunk(original_size, 0);
    ValueBuffer data_buffer;
    HwordBuffer lsb, msb;
    ByteBuffer tag_buffer;
    int32_t tag_pivot = 0, data_pivot = 0;  // bit size pivot
    int32_t original_cursor = 0;            // byte size cursor

#ifdef VERBOSE
    printf("Decompressing with FPC algorithm...\n");
#endif

    result.compression_type = "FPC(Frequent Pattern Compression";
    result.compressed = compressed;

    while (tag_pivot < (tag_overhead.valid_bitwidth - 1)) {
#ifdef VERBOSE
        printf("tag pivot: %d  data pivot: %d  original cursor: %d\n", tag_pivot, data_pivot, original_cursor);
#endif
        tag_buffer = get_value_bitwise(tag_overhead.body, tag_pivot, 3);
        tag_pivot += 3;

        switch (tag_buffer) {
        case 0:
#ifdef VERBOSE
            printf("prefix 0: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 3);
            data_pivot += 3;
            for (int i = 0; i < data_buffer + 1; i++) {
                set_value(original.body, 0x00, original_cursor, 1);
                original_cursor += 1;
            }
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 1:
#ifdef VERBOSE
            printf("prefix 1: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 4);
            set_value(original.body, SIGNEX(data_buffer, 3), original_cursor, 4);
            data_pivot += 4;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 2:
#ifdef VERBOSE
            printf("prefix 2: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 8);
            printf("%016llx, %lld\n", data_buffer, data_buffer);
            set_value(original.body, SIGNEX(data_buffer, 7), original_cursor, 4);
            data_pivot += 8;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 3:
#ifdef VERBOSE
            printf("prefix 3: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 16);
            set_value(original.body, SIGNEX(data_buffer, 15), original_cursor, 4);
            data_pivot += 16;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 4:
#ifdef VERBOSE
            printf("prefix 4: ");
#endif
            data_buffer = (get_value_bitwise(compressed.body, data_pivot, 16) & 0xffff);
            set_value(original.body, data_buffer, original_cursor, 4);
            data_pivot += 16;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 5:
#ifdef VERBOSE
            printf("prefix 5: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 16);
            lsb = SIGNEX((data_buffer & 0x00ff), 7);
            msb = SIGNEX(((data_buffer & 0xff00) >> BYTE_BITWIDTH), 7);
            set_value(original.body, lsb, original_cursor,     2);
            set_value(original.body, msb, original_cursor + 2, 2);
            data_pivot += 16;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 6:
#ifdef VERBOSE
            printf("prefix 6: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 8);
            set_value(original.body, data_buffer, original_cursor + 0, 1);
            set_value(original.body, data_buffer, original_cursor + 1, 1);
            set_value(original.body, data_buffer, original_cursor + 2, 1);
            set_value(original.body, data_buffer, original_cursor + 3, 1);
            data_pivot += 8;
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;

        case 7:
#ifdef VERBOSE
            printf("prefix 7: ");
#endif
            data_buffer = get_value_bitwise(compressed.body, data_pivot, 32);
            set_value(original.body, data_buffer, original_cursor, 4);
            original_cursor += 4;
#ifdef VERBOSE
            printf("completed\n");
#endif
            break;
        
        default:
            break;
        }
    }

    result.original = original;

    return result;
}


/* 
 * Functions for BDI algorithm with two bases
 *   BDI(Base Delta Immediate) algorithm is a compression algorithm usually used to compress
 *   memory blocks e.g. cache line. In this section, you can check newly designed BDI algorithm
 *   with two bases.
 *
 * Functions:
 *   bdi_twobase_compression: BDI compression algorithm with two bases
 *   bdi_twobase_decompression : BDI decompression algorithm with two bases
 *   bdi_twobase_compressing_unit: actually compresses given cacheline with certain encoding
 * 
 * Note
 *   This algorithm is reference to the paper of PACT12 conference
 *   url: https://users.ece.cmu.edu/~omutlu/pub/bdi-compression_pact12.pdf
 */

CompressionResult bdi_twobase_compression(CacheLine original) {
    CompressionResult result;
    MetaData tag_overhead = make_memory_chunk(2, 0);  // 2Bytes of tag overhead with its valid bitwidth of 11bits
    CacheLine compressed = make_memory_chunk(original.size, 0);  // initialize compressed cacheline with the size of original cacheline
    Bool is_compressed;

#ifdef VERBOSE
    printf("Compressing with BDI algorithm with two bases...\n");
#endif

    result.compression_type = "BDI(Base Delta Immediate) with two bases";
    result.original = original;

    for (int encoding = 0; encoding < 16; encoding++) {
        is_compressed = bdi_twobase_compressing_unit(original, &compressed, &tag_overhead, encoding);
        if (is_compressed == TRUE) {
            result.is_compressed = TRUE;
            break;
        }
    }

    result.compressed = compressed;
    result.tag_overhead = tag_overhead;

    if (result.is_compressed == FALSE) {
        remove_memory_chunk(compressed);
        result.compressed = copy_memory_chunk(original);
        result.tag_overhead.valid_bitwidth = 0;
    }

    return result;
}


Bool bdi_twobase_compressing_unit(CacheLine original, CacheLine *compressed, MemoryChunk *tag_overhead, int encoding) {
    ValueBuffer buffer, base, delta, mask;
    int k, d, segment_num, compressed_size;
    Bool is_compressed = FALSE;

    switch (encoding) {
    case 0:  // Zeros (encoding 0)
#ifdef VERBOSE
        printf("encoding %d: ", encoding);
#endif
        is_compressed = TRUE;
        for (int i = 0; i < original.size; i++) {
            buffer = get_value(original.body, i, 1);
            if (buffer != 0) {
                is_compressed = FALSE;
                break;
            }
        }

        if (is_compressed) {
#ifdef VERBOSE
            printf("succeed\n");
#endif
            compressed->size = 1;
            compressed->valid_bitwidth = 8;
            set_value_bitwise(tag_overhead->body, 0, encoding, 4);
            set_value_bitwise(tag_overhead->body, 1, 4, 7);
            tag_overhead->valid_bitwidth = 11;
            return TRUE;
        }
#ifdef VERBOSE
        else 
            printf("failed\n");
#endif

        return FALSE;
    
    case 1:  // Repeating values (encoding 1)
#ifdef VERBOSE
        printf("encoding %d: ", encoding);
#endif
        is_compressed = TRUE;
        base = get_value(original.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
            buffer = get_value(original.body, i, 8);
            if (buffer != base) {
                is_compressed = FALSE;
                break;
            }
        }

        if (is_compressed) {
            compressed->size = 8;
            compressed->valid_bitwidth = 64;
            set_value(compressed->body, base, 0, 8);
            set_value_bitwise(tag_overhead->body, 0, encoding, 4);
            set_value_bitwise(tag_overhead->body, 1, 4, 7);
            tag_overhead->valid_bitwidth = 11;
#ifdef VERBOSE
            printf("succeed\n");
#endif
        }
#ifdef VERBOSE
        else
            printf("failed\n");
#endif

        return FALSE;

    case 2:  // Base8-delta1 (encoding 2)
        k = 8;
        d = 1;

        break;

    case 3:  // Base8-delta2 (encoding 3)
        k = 4;
        d = 1;

        break;

    case 4:  // Base8-delta4 (encoding 4)
        k = 2;
        d = 1;

        break;

    case 5:  // Base4-delta1 (encoding 5)
        k = 8;
        d = 2;

        break;
    
    case 6:  // Base4-delta2 (encoding 6)
        k = 4;
        d = 2;

        break;

    case 7:  // Base2-delta1 (encoding 7)
        k = 8;
        d = 4;

        break;
    
    default:
        k = 8;
        d = 1;

        break;
    }

#ifdef VERBOSE
    printf("encoding %d (Base%d-Delta%d)\n", encoding, k, d);
#endif

    compressed_size = k;
    is_compressed = TRUE;
    mask = 0;

    if (d >= 1) mask += 0xff;
    if (d >= 2) mask += 0xff00;
    if (d >= 4) mask += 0xffff0000;

    base = get_value(original.body, 0, k);
    set_value(compressed->body, base, 0, k);

    for (int i = 0, j = 0; i < original.size; i += k) {
        buffer = get_value(original.body, i, k);
        delta = buffer - base;
        if (k < 8) delta = SIGNEX(delta, (k * BYTE_BITWIDTH - 1));
#ifdef VERBOSE
        printf("[ITER %2d] base: 0x%016llx  buffer: 0x%016llx  delta: 0x%016llx\n", i/k, base, buffer, delta);
#endif
        if (SIGNEX(delta & mask, (d * BYTE_BITWIDTH) - 1) == delta) {
#ifdef VERBOSE
            printf(">>> delta is %dByte sign extended\n", d);
#endif
            set_value(compressed->body, delta, k + (j * d), d);
        } else if (SIGNEX(buffer & mask, d * (BYTE_BITWIDTH) - 1) == buffer) {
#ifdef VERBOSE
            printf(">>> delta is not %dByte sign extended but buffer is %dByte sign extended\n", d, d);
#endif
            set_value(compressed->body, buffer, k + (j * d), d);
            set_value_bitwise(tag_overhead->body, 1, 11 + j, 1);
        } else {
            is_compressed = FALSE;
            break;
        }

        compressed_size += d;
        j += 1;
    }

    if (is_compressed == TRUE) {
        compressed->size = compressed_size;
        compressed->valid_bitwidth = compressed_size * 8;
        tag_overhead->valid_bitwidth = 11 + (original.size / k);
#ifdef VERBOSE
        printf("encoding %d succeed (size: %dBytes)\n", encoding, compressed->size);
#endif
    }
#ifdef VERBOSE
    else {
        printf("encoding %d failed\n", encoding);
    }
#endif

    return is_compressed;
}

DecompressionResult bdi_twobase_decompression(CacheLine compressed, MetaData tag_overhead, int original_size) {
    CacheLine original = make_memory_chunk(original_size, 0);
    DecompressionResult result;
    ValueBuffer base, buffer, mask=1;
    int k, d;
    int encoding = get_value_bitwise(tag_overhead.body, 0, 4);
    int segment_num = get_value_bitwise(tag_overhead.body, 4, 7);
    int selection, sel_mask = 1;

#ifdef VERBOSE
    printf("Decompressing with BDI algorithm...\n");
    printf("encoding: %d\n", encoding);
    printf("segment pointer: %d\n", segment_num);
#endif

    result.compressed = compressed;
    result.compression_type = "BDI(Base Delta Immediate) with two bases";

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
        k = 4; 
        d = 1;
        break;

    case 4:  // Base8-delta4
        k = 2; 
        d = 1;
        break;

    case 5:  // Base4-delta1
        k = 8; 
        d = 2;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 8; 
        d = 4;
        break;
    
    default:
        result.original = copy_memory_chunk(compressed);
        remove_memory_chunk(original);
        result.is_decompressed = FALSE;
        return result;
    }

    base = get_value(compressed.body, 0, k);
    selection = get_value_bitwise(tag_overhead.body, 11, original_size / k);

    for (int i = 0; i < original.size / k; i++) {
        buffer = get_value(compressed.body, k + (d*i), d);
        if (selection & (sel_mask << i))
            set_value(original.body, SIGNEX(buffer, (d * BYTE_BITWIDTH)-1), i * k, k);
        else
            set_value(original.body, SIGNEX(buffer, (d * BYTE_BITWIDTH)-1) + base, i * k, k);
    }
    result.original = original;
    result.is_decompressed = TRUE;
    return result;
}


/* 
 * Functions for BDI algorithm with zeros run detection
 *   BDI(Base Delta Immediate) algorithm is a compression algorithm usually used to compress
 *   memory blocks e.g. cache line. In this section, you can check newly designed BDI algorithm
 *   with zeros run(ZR).
 *
 * Functions:
 *   bdi_zr_compression: BDI compression algorithm with zeros run detection
 *   bdi_zr_decompression : BDI decompression algorithm with zeros run detection
 *   bdi_zr_compressing_unit: actually compresses given cacheline with certain encoding
 * 
 * Note
 *   This algorithm is reference to the paper of PACT12 conference
 *   url: https://users.ece.cmu.edu/~omutlu/pub/bdi-compression_pact12.pdf
 */

CompressionResult bdi_zr_compression(CacheLine original) {
    CompressionResult result;
    MetaData shifting;  // Maximum size of shifting bits are 16Bytes
    MetaData tag_overhead = make_memory_chunk(2, 0);  // 2Bytes of tag overhead with its valid bitwidth of 11bits
    CacheLine compressed = make_memory_chunk(original.size, 0);  // initialize compressed cacheline with the size of original cacheline
    Bool is_compressed;

#ifdef VERBOSE
    printf("Compressing with BDI algorithm with zeros run...\n");
#endif

    result.compression_type = "BDI(Base Delta Immediate) with zeros run";
    result.original = original;

    for (int encoding = 0; encoding < 8; encoding++) {
        shifting = bdi_zr_detector(original, encoding);
#ifdef VERBOSE
        printf("[INITIAL] compressing with encoding %d\n", encoding);
        printf("shifting: ");
        print_memory_chunk_bitwise(shifting);
        printf("\n");
#endif
        is_compressed = bdi_zr_compressing_unit(original, &compressed, &shifting, &tag_overhead, encoding);
        if (is_compressed) break;
        remove_memory_chunk(shifting);
    }

    result.compressed = compressed;
    result.tag_overhead = tag_overhead;

    if (result.is_compressed == FALSE) {
        remove_memory_chunk(compressed);
        result.compressed = copy_memory_chunk(original);
        result.tag_overhead.valid_bitwidth = 0;
    }

    return result;
}

MemoryChunk bdi_zr_detector(CacheLine original, int encoding) {
    MemoryChunk shifting = make_memory_chunk(16, 0);
    ValueBuffer base, buffer;
    int k, zeros_cnt, shift_block_size;
    int offset = 0;

    switch (encoding) {
    case 0:
    case 1:
        shifting.size = 0;
        shifting.valid_bitwidth = 0;
        return shifting;

    case 2:
    case 5:
    case 7:
        k = 8;
        break;
    
    case 3:
    case 6:
        k = 4;
        break;

    case 4:
        k = 2;
        break;
    
    default:
        break;
    }

    shift_block_size = (int)log2((double)k) + 1;
    shifting.size = (int)((original.size / k) * ((double)shift_block_size / BYTE_BITWIDTH));
    shifting.valid_bitwidth = shifting.size * BYTE_BITWIDTH;

    base = get_value(original.body, 0, k);
    zeros_cnt = 0;

    for (zeros_cnt = 0; zeros_cnt < k; zeros_cnt++) {
        if ((base & ((ValueBuffer)0xff << (zeros_cnt * BYTE_BITWIDTH)))) {
            break;
        }
    }

    if (zeros_cnt > 0) {
        set_value_bitwise(shifting.body, 1, 0, 1);
        set_value_bitwise(shifting.body, zeros_cnt-1, 1, shift_block_size-1);
    }

    offset += shift_block_size;

    for (int i = k; i < original.size; i += k) {
        buffer = get_value(original.body, i, k);

        for (zeros_cnt = 0; zeros_cnt < k; zeros_cnt++) {
            if ((buffer & ((ValueBuffer)0xff << (zeros_cnt * BYTE_BITWIDTH)))) {
                break;
            } 
        }

        if (zeros_cnt > 0) {
            set_value_bitwise(shifting.body, 1, offset, 1);
            set_value_bitwise(shifting.body, zeros_cnt-1, offset+1, shift_block_size-1);
        }

        // printf("[TEST] buffer: 0x%016llx  zeros_cnt: %d  offset: %d\n", buffer, zeros_cnt, offset);

        offset += shift_block_size;
    }

    return shifting;
}

Bool bdi_zr_compressing_unit(CacheLine original, CacheLine *compressed, MemoryChunk *shifting, MemoryChunk *tag_overhead, int encoding) {
    ValueBuffer base, buffer, delta, mask;
    MemoryChunk extendinfo;
    int extendinfo_offset;
    int k, d, zeros_cnt, zeros_cnt_bitwidth, offset, shift_bit, compressed_size;

    switch (encoding) {
    case 0:
        for (int i = 0; i < original.size; i += 1) {
            if (original.body[i] != 0)
                return FALSE;
        }

        compressed->body[0] = 0;
        compressed->size = 1;
        compressed->valid_bitwidth = 8;
        return TRUE;

    case 1:
        base = get_value(original.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
            buffer = get_value(original.body, i, 8);
            if (base != buffer)
                return FALSE;
        }

        set_value(compressed->body, base, 0, 8);
        compressed->size = 8;
        compressed->valid_bitwidth = 8;
        return TRUE;
    
    case 2:
        k = 8;
        d = 1;
        zeros_cnt_bitwidth = 3;
        break;

    case 3:
        k = 4;
        d = 1;
        zeros_cnt_bitwidth = 2;
        break;

    case 4:
        k = 2;
        d = 1;
        zeros_cnt_bitwidth = 1;
        break;

    case 5:
        k = 8;
        d = 2;
        zeros_cnt_bitwidth = 3;
        break;

    case 6:
        k = 4;
        d = 2;
        zeros_cnt_bitwidth = 2;
        break;

    case 7:
        k = 8;
        d = 4;
        zeros_cnt_bitwidth = 3;
        break;
    
    default:
        break;
    }

    offset = 0;
    compressed_size = shifting->size;
    extendinfo = make_memory_chunk(ceil((double)original.size / (k * BYTE_BITWIDTH)), 0);
    mask = 0;

    if (d >= 1) mask += 0xff;
    if (d >= 2) mask += 0xff00;
    if (d >= 4) mask += 0xffff0000;

    base = get_value(original.body, 0, k);
    shift_bit = get_value_bitwise(shifting->body, 0, 1);

#ifdef VERBOSE
    printf("[ITER 0] base: 0x%016llx  shift_bit: %d\n", base, shift_bit != 0);
#endif

    if (shift_bit != 0) {
        zeros_cnt = get_value_bitwise(shifting->body, 1, zeros_cnt_bitwidth);
        base = base >> ((zeros_cnt + 1) * BYTE_BITWIDTH);
#ifdef VERBOSE
    printf(">>> shift base %dBytes -> base: 0x%016llx\n", zeros_cnt+1, base);
    printf(">>> current compressed size: %dBytes  shifting offset: %dbits\n", compressed_size, offset);
#endif
        set_value(compressed->body, base, compressed_size, k);
        compressed_size += k;
    }

    offset += 1 + zeros_cnt_bitwidth;
    extendinfo_offset = 1;

    for (int i = k; i < original.size; i += k) {
        buffer = get_value(original.body, i, k);
        shift_bit = get_value_bitwise(shifting->body, offset, 1);
#ifdef VERBOSE
        printf("[ITER %d] buffer: 0x%016llx  shift_bit: %d\n", i/k, buffer, shift_bit != 0);
#endif

        if (shift_bit != 0) {
            zeros_cnt = get_value_bitwise(shifting->body, offset+1, zeros_cnt_bitwidth);
            if (zeros_cnt == (k - 1)) {
#ifdef VERBOSE
                printf(">>> do not save delta due to zero value\n");
                printf(">>> current compressed size: %dBytes  shifting offset: %dbits\n", compressed_size, offset);
#endif
                offset += zeros_cnt_bitwidth + 1;
                continue;
            }
            buffer = buffer >> ((zeros_cnt + 1) * BYTE_BITWIDTH);
#ifdef VERBOSE
            printf(">>> shift buffer %dBytes -> base: 0x%016llx\n", zeros_cnt+1, base);
            printf(">>> current compressed size: %dBytes  shifting offset: %dbits\n", compressed_size, offset);
#endif
        }

        offset += zeros_cnt_bitwidth + 1;
        delta = buffer - base;

#ifdef VERBOSE
        printf("base: 0x%016llx  buffer: 0x%016llx  delta: 0x%016llx  extended: 0x%016llx\n", base, buffer, delta, SIGNEX(delta & mask, (d * BYTE_BITWIDTH) - 1));
#endif
        if ((delta & (~mask)) == 0) {
            set_value(compressed->body, delta, compressed_size, d);
            compressed_size += d;
#ifdef VERBOSE
            printf(">>> current compressed size: %dBytes  shifting offset: %dbits (extended with zero pad)\n", compressed_size, offset);
#endif
        } else if ((delta & (~mask)) == (~mask)) {
            set_value_bitwise(extendinfo.body, 1, extendinfo_offset, 1);
            set_value(compressed->body, delta, compressed_size, d);
            compressed_size += d;
#ifdef VERBOSE
            printf(">>> current compressed size: %dBytes  shifting offset: %dbits (extended with 1)\n", compressed_size, offset);
#endif
        } else {
#ifdef VERBOSE
            printf("failed\n");
#endif
            return FALSE;
        }

        extendinfo_offset += 1;
    }

    // copy extension information to compressed array
    for (int i = 0; i < extendinfo.size; i++)
        set_value(compressed->body, extendinfo.body[i], i + compressed_size, 1);
    compressed_size += extendinfo.size;
    remove_memory_chunk(extendinfo);

    // copy shifting information to compressed array
    for (int i = 0; i < shifting->size; i++) 
        set_value(compressed->body, shifting->body[i], i, 1);

    compressed->size = compressed_size;
    compressed->valid_bitwidth = compressed_size * BYTE_BITWIDTH;

    set_value_bitwise(tag_overhead->body, encoding, 0, 4);
    set_value_bitwise(tag_overhead->body, ceil((double)compressed_size / 8), 4, 7);
    tag_overhead->size = 2;
    tag_overhead->valid_bitwidth = 11;

#ifdef VERBOSE
    printf("succeed\n");
#endif
    return TRUE;
}


/* 
 * Other algorithms on test
 *   1. Zero Vector Compression
 *      This algorithm includes 1bit index for extracting zero byte (expecting good compression ratio on sparse dataset)
 *   2. Zeros Run Compression
 *      This algorithm is zeros run compression algorithm in FPC
 *
 * Functions:
 *   zero_vec_compression: zero vector compression
 *   zeros_run_compression: zeros run compression
 */

CompressionResult zero_vec_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed;
    const double threshold = 0.5;
    int zero_cnt, index, offset = original.size;  // in bits

#ifdef VERBOSE
    printf("Compressing with zero vector algorithm...\n");
#endif

    result.compression_type = "Zero Vector algorithm";
    result.original = original;

    zero_cnt = 0;
    for (index = 0; index < original.size; index++) {
        if (original.body[index] == 0x00)
            zero_cnt++;
    }

    if (((double)zero_cnt / original.size) < threshold) {
#ifdef VERBOSE
        printf("failed due to insufficient sparcity of the memory chunk\n");
#endif  
        compressed = copy_memory_chunk(original);
#ifdef VERBOSE
        printf("compressed size: %dBytes\n", compressed.size);
#endif
        result.is_compressed = FALSE;
        result.compressed = compressed;
        result.tag_overhead = make_memory_chunk(1, 0);
        result.tag_overhead.size = 0;
        result.tag_overhead.valid_bitwidth = 0;

        return result;
    }

    compressed = make_memory_chunk(original.size, 0);

    // generating bit indexes
    for (index = 0; index < original.size; index++) {
#ifdef VERBOSE
        printf("original.body[%2d] = 0x%02x (is_zero: %5s, offset: %2d, index: %2d)\n", index, 
                                                               original.body[index], 
                                                               original.body[index] == 0 ? "true" : "false",
                                                               offset, index);
#endif
        if (original.body[index] != 0x00) {
            set_value_bitwise(compressed.body, 1, index, 1);
            set_value_bitwise(compressed.body, original.body[index], offset, BYTE_BITWIDTH);
            offset += BYTE_BITWIDTH;
        }
    }

    compressed.size = offset / BYTE_BITWIDTH;
    compressed.valid_bitwidth =  offset;
    result.compressed = compressed;
    result.is_compressed = TRUE;
    // result.tag_overhead = make_memory_chunk(1, 0);
    // result.tag_overhead.size = 0;
    // result.tag_overhead.valid_bitwidth = 0;

#ifdef VERBOSE
    printf("succeed\n");
#endif

    return result;
}

CompressionResult zeros_run_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size, 0);
    ValueBuffer buffer;
    int offset = 0, zeros_cnt = 0;
    Bool flag = TRUE;

#ifdef VERBOSE
    printf("Compressing with zeros run algorithm...\n");
#endif

    result.compression_type = "Zeros Run algorithm";
    result.original = original;

    for (int i = 0; (i < original.size) && flag; i++) {
        buffer = get_value(original.body, i, 1);
#ifdef VERBOSE
        printf("[ITER %2d] buffer: 0x%08x offset: %3d zeros_cnt: %d\n", i, (ByteBuffer)buffer, offset, zeros_cnt);
#endif

        if (buffer == 0) {
            zeros_cnt += 1;
        } else {
            if (zeros_cnt != 0) {
                if ((original.size * BYTE_BITWIDTH) - offset < 4) {
                    flag = FALSE;
                    break;
                }

                set_value_bitwise(compressed.body, 1, offset, 1);
                set_value_bitwise(compressed.body, zeros_cnt-1, offset+1, 3);
                offset += 4;
                zeros_cnt = 0;
            }

            if ((original.size * BYTE_BITWIDTH) - offset < (BYTE_BITWIDTH + 1)) {
                flag = FALSE;
                break;
            }

            set_value_bitwise(compressed.body, buffer, offset+1, BYTE_BITWIDTH);
            offset += BYTE_BITWIDTH+1;
        }

        if (zeros_cnt == 8) {
            if ((original.size * BYTE_BITWIDTH) - offset < 4) {
                flag = FALSE;
                break;
            }

            set_value_bitwise(compressed.body, 1, offset, 1);
            set_value_bitwise(compressed.body, 7, offset+1, 3);
            offset += 4;
            zeros_cnt = 0;
        }
    }

    if (zeros_cnt != 0) {
        if ((original.size * BYTE_BITWIDTH) - offset < 4) {
            flag = FALSE;
        } else {
            set_value_bitwise(compressed.body, 1, offset, 1);
            set_value_bitwise(compressed.body, zeros_cnt-1, offset, 3);
            offset += 4;
            zeros_cnt = 0;
        }
    }

    if (flag == FALSE) {
#ifdef VERBOSE
        printf("failed (compression increases the size)\n");
#endif

        remove_memory_chunk(compressed);
        result.compressed = copy_memory_chunk(original);
        result.is_compressed = FALSE;
        result.tag_overhead = make_memory_chunk(1, 0);
        result.tag_overhead.size = 0;
        result.tag_overhead.valid_bitwidth = 0;

        return result;
    }

#ifdef VERBOSE
    printf("succeed (offset: %d)\n", offset);
#endif

    compressed.size = ceil((double)offset / BYTE_BITWIDTH);
    compressed.valid_bitwidth = offset;
    result.compressed = compressed;
    result.tag_overhead = make_memory_chunk(1, 0);
    result.tag_overhead.size = 0;
    result.tag_overhead.valid_bitwidth = 0;
    result.is_compressed = TRUE;

    return result;
}


/* 
 * Functions for BDI algorithm with zero base detection
 *   BDI(Base Delta Immediate) algorithm is a compression algorithm usually used to compress
 *   memory blocks e.g. cache line. In this section, you can check newly designed BDI algorithm
 *   with zero base encoding. This algorithm encodes zero data blocks into 1bit encoding
 *
 * Functions:
 *   bdi_ze_compression: BDI compression algorithm with zero base encoding
 *   bdi_ze_compressing_unit: Compressing unit for BDI algorithm with zero base encoding
 * 
 * Note
 *   This algorithm is reference to the paper of PACT12 conference
 *   url: https://users.ece.cmu.edu/~omutlu/pub/bdi-compression_pact12.pdf
 */

CompressionResult bdi_ze_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size, 0);
    MetaData tag_overhead = make_memory_chunk(2, 0);
    Bool is_compressed;

#ifdef VERBOSE
    printf("Compressing with BDI algorithm with zero base encoding...\n");
#endif

    result.compression_type = "BDI(Base Delta Immediate) with zero base encoding";
    result.original = original;

    for (int encoding = 0; encoding < 8; encoding++) {
#ifdef VERBOSE
        printf("[INITIAL] compressing with encoding %d\n", encoding);
#endif
        is_compressed = bdi_ze_compressing_unit(original, &compressed, &tag_overhead, encoding);
        if (is_compressed) break;
    }

    result.compressed = compressed;
    result.tag_overhead = tag_overhead;

    if (is_compressed == FALSE) {
        remove_memory_chunk(compressed);
        result.compressed = copy_memory_chunk(original);
        result.tag_overhead.valid_bitwidth = 0;
    }

    return result;
}

Bool bdi_ze_compressing_unit(CacheLine original, CacheLine *compressed, MetaData *tag_overhead, int encoding) {
    ValueBuffer base, buffer, delta, mask;
    int k, d;
    int compressed_siz = 0;
    Bool initial, flag = TRUE;
    MetaData zero_base_encoding;
    int zero_base_encoding_siz;

    // 0. Select mode by given encoding (MUX)
    switch (encoding) {
    case 0:  // Zero values
#ifdef VERBOSE
        printf("Zeros compression (encoding: %d)\n", encoding);
#endif
        for (int i = 0; i < original.size; i++) {
#ifdef VERBOSE
            printf("[ITER %2d] base: 0x%02x  buffer: 0x%02x\n", i, 0, original.body[i]);
#endif
            if (original.body[i] != 0) {
#ifdef VERBOSE
                printf("iteration terminated (not compressible)\n");
#endif
                return FALSE;
            }
        }
#ifdef VERBOSE
        printf("compression completed\n");
#endif
        compressed->size = 1;
        set_value_bitwise(tag_overhead->body, 0, 0, 4);
        set_value_bitwise(tag_overhead->body, 1, 4, 7);
        return TRUE;
    
    case 1:  // Repeated values
#ifdef VERBOSE
        printf("Repeated values compression (encoding: %d)\n", encoding);
#endif
        base = get_value(original.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
#ifdef VERBOSE
            printf("[ITER %2d] base: 0x%016llx  buffer: 0x%016llx\n", i, base, get_value(original.body, i, 8));
#endif
            if (get_value(original.body, i, 8) != base) {
#ifdef VERBOSE
                printf("iteration terminated (not compressible)\n");
#endif
                return FALSE;
            }
        }
#ifdef VERBOSE
        printf("compression completed\n");
#endif
        set_value(compressed->body, base, 0, 8);
        set_value_bitwise(tag_overhead->body, 1, 0, 4);
        set_value_bitwise(tag_overhead->body, 1, 4, 7);
        compressed->size = 8;
        tag_overhead->size = 11;
        
        return TRUE;

    case 2:  // Base8-delta1
        k = 8; 
        d = 1;
        break;
    
    case 3:  // Base8-delta2
        k = 4; 
        d = 1;
        break;

    case 4:  // Base8-delta4
        k = 2; 
        d = 1;
        break;

    case 5:  // Base4-delta1
        k = 8; 
        d = 2;
        break;

    case 6:  // Base4-delta2
        k = 4; 
        d = 2;
        break;
    
    case 7:  // Base2-delta1
        k = 8; 
        d = 4;
        break;
    
    default:
        k = 8; 
        d = 1;
        break;
    }

#ifdef VERBOSE
    printf("Base%d-Delta%d (encoding: %d)\n", k, d, encoding);
#endif

    zero_base_encoding_siz = ceil((double)original.size / k);
    zero_base_encoding = make_memory_chunk(zero_base_encoding_siz, 0);
    compressed_siz = zero_base_encoding_siz;

    ValueBuffer byte_mask = 0x00;
    if (d >= 1) byte_mask += 0xff;
    if (d >= 2) byte_mask += 0xff00;
    if (d >= 4) byte_mask += 0xffff0000;

    base = get_value(original.body, 0, k);
    set_value(compressed->body, base, compressed_siz, k);
    compressed_siz += k;

    for (int i = k; i < original.size; i += k) {
        buffer = get_value(original.body, i, k);
#ifdef VERBOSE
        printf("[ITER %2d] base: 0x%016llx  buffer: 0x%016llx\n", i/k, base, buffer);
#endif

        if (buffer == 0x00) {
#ifdef VERBOSE
            printf("zero buffer encoded\n");
#endif
            set_value_bitwise(zero_base_encoding.body, 0, i / k, 1);
            continue;
        } else {
            set_value_bitwise(zero_base_encoding.body, 1, i / k, 1);
        }

        delta = buffer - base;
#ifdef VERBOSE
        printf("delta: 0x%016llx\n", delta);
#endif

        if (delta != SIGNEX(delta & byte_mask, (d * BYTE_BITWIDTH) - 1)) {
#ifdef VERBOSE
            printf("compression failed (sign extension failure)\n");
#endif
            return FALSE;
        }

        set_value(compressed->body, delta, compressed_siz, d);
        compressed_siz += d;
    }

#ifdef VERBOSE
    printf("compression succeed\n");
#endif

    compressed->size = compressed_siz;
    set_value_bitwise(tag_overhead->body, encoding, 0, 4);
    set_value_bitwise(tag_overhead->body, ceil((double)compressed_siz / 8), 4, 7);

    return TRUE;
}  