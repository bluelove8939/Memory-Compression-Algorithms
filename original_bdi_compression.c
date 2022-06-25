#include "original_bdi_compression.h"

unsigned long long my_llabs(long long x) {
   unsigned long long t = x >> 63;
   return (x ^ t) - t;
}

unsigned my_abs(int x) {
   unsigned t = x >> 31;
   return (x ^ t) - t;
}

BufferArr convertBuffer2Array(ByteArr arr, unsigned size, unsigned step) {
    BufferArr values = (BufferArr)malloc(sizeof(ValueBuffer) * (size / step));
    
    for (int i = 0; i < size / step; i++) {
        values[i] = 0;    // Initialize all elements to zero.
    }

    for (int i = 0; i < size; i += step) {
        for (int j = 0; j < step; j++) {
            values[i / step] += (ValueBuffer)((Byte)arr[i + j]) << (BYTE_BITWIDTH * j);
        }
    }

    return values;
}

Bool isZeroPackable(BufferArr values, unsigned size) {
    int is_packable = TRUE;

    for (int i = 0; i < size; i++) {
        if( values[i] != 0){
            is_packable = FALSE;
            break;
        }
    }

    return is_packable;
}

Bool isSameValuePackable(BufferArr values, unsigned size) {
    int is_packable = TRUE;

    for (int i = 0; i < size; i++) {
        if( values[0] != values[i]){
            is_packable = FALSE;
            break;
        }
    }

    return is_packable;
}

unsigned multBaseCompression (BufferArr values, unsigned size, unsigned blimit, unsigned bsize) {
    unsigned long long limit = 0;
    unsigned BASES = 2;

    //define the appropriate size for the mask
    switch(blimit){
        case 1:
            limit = 0xFF;
            break;
        case 2:
            limit = 0xFFFF;
            break;
        case 4:
            limit = 0xFFFFFFFF;
            break;
        default:
            limit = 0xff;
    }

    // finding bases: # BASES
    ValueBuffer mbases[64];
    unsigned baseCount = 1;
    mbases[0] = 0;

    for (int i = 0; i < size; i++) {
        for(int j = 0; j <  baseCount; j++){
            if (my_llabs((long long int)(mbases[j] - values[i])) > limit) {
                mbases[baseCount++] = values[i];  
            }
        }

        if (baseCount >= BASES)
            break;
    }

    // find how many elements can be compressed with mbases
    unsigned compCount = 0;
    for (int i = 0; i < size; i++) {
        //ol covered = 0;
        for(int j = 0; j <  baseCount; j++){
            if( my_llabs((long long int)(mbases[j] -  values[i])) <= limit ){
                compCount++;
                break;
            }
        }
    }
    
    unsigned mCompSize = blimit * compCount + bsize * BASES + (size - compCount) * bsize;
    if(compCount < size)
        return size * bsize;
    return mCompSize;
}

unsigned bdi_original_compression(CacheLine original) {
    BufferArr values;
    unsigned bestCSize = original.size;
    unsigned currCSize = original.size;

    // Base8 compressions including zero packing and repeating values
    values = convertBuffer2Array(original.body, original.size, 8);

    if (isZeroPackable(values, original.size / 8))
        bestCSize = 1;
    if (isSameValuePackable(values, original.size / 8))
        currCSize = 8;
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;

    currCSize = multBaseCompression(values, original.size / 8, 1, 8);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    currCSize = multBaseCompression(values, original.size / 8, 2, 8);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    currCSize =  multBaseCompression(values, original.size / 8, 4, 8);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    free(values);

    // Base4 compression including repeated values
    values = convertBuffer2Array(original.body, original.size, 4);
    if( isSameValuePackable(values, original.size / 4))
        currCSize = 4;
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    currCSize = multBaseCompression(values, original.size / 4, 1, 4);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    currCSize = multBaseCompression(values, original.size / 4, 2, 4);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    free(values);

    // Base2 compression including repeated values
    values = convertBuffer2Array(original.body, original.size, 2);
    currCSize = multBaseCompression(values, original.size / 2, 1, 2);
    bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
    free(values);

    return bestCSize;
}
