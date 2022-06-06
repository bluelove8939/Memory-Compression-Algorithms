#include "compression.h"

CacheLine base_plus_delta(CacheLine original) {
    CompressionResult result;
}

CacheLine base_delta_immediate(CacheLine original) {
    CompressionResult result;
    CacheLine compressed;

    for (int encoding = 0; encoding < 8; encoding++) {
        compressed = bdi_compressing_unit(original, encoding);
        if (compressed.size < original.size) {
            result.compressed = compressed;
            result.is_compressed = TRUE;
        }
    }
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
        for (int i = 0; i < original.size; i++) {
            if (original.body[i] != 0) {
                result = copy_memory_chunk(original);
                return result;
            }
        }
        result = make_memory_chunk(1, 0);
        return result;
    
    case 1:  // Repeating values
        base = get_value(original.body, 0, 8);
        for (int i = 0; i < original.size; i += 8) {
            if (get_value(original.body, i, 8) != base) {
                result = copy_memory_chunk(original);
                return result;
            }
        }
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
        printf("[ITER %2d] base: %08x  buffer: %08x  delta: %08x\n", i/k, base, buffer, delta);

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
            printf("iteration terminated (not compressible)\n");
            remove_memory_chunk(result);
            result = copy_memory_chunk(original);
            return result;
        } else {
            mask = 1;
            set_value(result.body, delta, compressed_siz, d);
            compressed_siz += d;
        }
    }

    result.size = compressed_siz;
    return result;
}

int main(int argc, char const *argv[]) {
    CacheLine original = make_memory_chunk(CACHE64SIZ, 0);
    *(original.body+1) = 36;
    print_memory_chunk(original);
    
    int encoding = 3;
    CacheLine compressed = bdi_compressing_unit(original, encoding);
    print_memory_chunk(compressed);

    remove_memory_chunk(compressed);
    remove_memory_chunk(original);

    return 0;
}
