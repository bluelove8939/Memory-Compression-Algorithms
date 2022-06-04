#include "compression.h"

CompressionResult base_plus_delta(CacheLine original) {
    CompressionResult result;
}

CompressionResult base_delta_immediate(CacheLine original) {

}

CompressionResult bdi_compressing_unit(CacheLine original, int k, int d) {
    void *splitted;

    switch (k) {
        case 8:
            splitted = (signed long int *)original;
            break;
        
        case 4:
            splitted = (signed int *)original;
            break;

        case 2:
            splitted = (signed short *)original;
            break;
        
        default:
            break;
    }
        

}

int main(int argc, char const *argv[]) {
    CacheLine original = (CacheLine)malloc(CACHE64SIZ);
    memset(original, 0, CACHE64SIZ);
    original[1] = 1;
    printf("size: %d\n", sizeof(original));
    printf("size: %d\n", CACHE64SIZ);

    return 0;
}
