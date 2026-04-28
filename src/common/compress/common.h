/***********************************************************************************************************************************
Compression Common

Pack-encoded constructor parameter helpers shared by every compression backend so that filters can be reconstructed on the far
side of the protocol layer.
***********************************************************************************************************************************/
#ifndef COMMON_COMPRESS_COMMON_H
#define COMMON_COMPRESS_COMMON_H

#include "common/type/pack.h"

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Build compress param list
FN_EXTERN Pack *compressParamList(int level, bool raw);

// Build decompress param list
FN_EXTERN Pack *decompressParamList(bool raw);

#endif
