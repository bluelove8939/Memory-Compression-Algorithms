#ifndef BDI_ZEROVEC
#define BDI_ZEROVEC

#include "compression.h"

// Verbose parameter
// #define VERBOSE  // Comment this line not to display debug messages

CompressionResult bdi_zv_compression(CacheLine original);
Bool bdi_zv_compressing_unit(CacheLine original, CacheLine *compressed, MetaData *tag_overhead, int encoding);

#endif