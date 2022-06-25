#include "bdi_zerovec.h"


CompressionResult bdi_zv_compression(CacheLine original) {
    CompressionResult result;
    CacheLine compressed = make_memory_chunk(original.size, 0);
    CacheLine tag_overhead = make_memory_chunk(2, 0);
    Bool is_compressed;

    for (int encoding = 0; encoding < 8; encoding++) {
        is_compressed = bdi_zv_compressing_unit(original, &compressed, &tag_overhead, encoding);

        if (is_compressed) {
            result.compressed = compressed;
            result.tag_overhead = tag_overhead;
            result.is_compressed = TRUE;
            return result;
        }
    }

    result.compressed = original;
    result.tag_overhead = tag_overhead;
    remove_memory_chunk(compressed);
    result.is_compressed = FALSE;

    return result;
}


Bool bdi_zv_compressing_unit(CacheLine original, CacheLine *compressed, MetaData *tag_overhead, int encoding) {
    ValueBuffer base, buffer, delta, mask;
    int k, d, compressed_size;

    switch (encoding) {
    case 0:
        for (int i = 0; i < original.size; i += 1) {
            if (original.body[i] != 0)
                return FALSE;
        }

        compressed->body[0] = 0;
        compressed->size = 1;
        compressed->valid_bitwidth = 8;
        set_value_bitwise(tag_overhead->body, encoding, 0, 4);
        set_value_bitwise(tag_overhead->body, 1, 4, 7);
        tag_overhead->valid_bitwidth = 11;
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
        compressed->valid_bitwidth = 64;
        set_value_bitwise(tag_overhead->body, encoding, 0, 4);
        set_value_bitwise(tag_overhead->body, 1, 4, 7);
        tag_overhead->valid_bitwidth = 11;
        return TRUE;
    
    case 2:
        k = 8;
        d = 1;
        break;

    case 3:
        k = 4;
        d = 1;
        break;

    case 4:
        k = 2;
        d = 1;
        break;

    case 5:
        k = 8;
        d = 2;
        break;

    case 6:
        k = 4;
        d = 2;
        break;

    case 7:
        k = 8;
        d = 4;
        break;
    
    default:
        break;
    }

    
}