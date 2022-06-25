#ifndef ORIGINAL_BDI
#define ORIGINAL_BDI

#include "compression.h"

// Verbose parameter
#define VERBOSE  // Comment this line not to display debug messages

typedef ValueBuffer* BufferArr;

unsigned long long my_llabs(long long x);
unsigned my_abs(int x);

BufferArr convertBuffer2Array(ByteArr arr, unsigned size, unsigned step);
Bool isZeroPackable(BufferArr values, unsigned size);
Bool isSameValuePackable(BufferArr values, unsigned size);
unsigned multBaseCompression (BufferArr values, unsigned size, unsigned blimit, unsigned bsize);

unsigned bdi_original_compression(CacheLine original);  // original BDI compression algorithm just showing compressed size

#endif