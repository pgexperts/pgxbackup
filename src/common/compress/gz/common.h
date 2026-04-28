/***********************************************************************************************************************************
Gz Common
***********************************************************************************************************************************/
#ifndef COMMON_COMPRESS_GZ_COMMON_H
#define COMMON_COMPRESS_GZ_COMMON_H

/***********************************************************************************************************************************
Gz extension
***********************************************************************************************************************************/
#define GZ_EXT                                                      "gz"

/***********************************************************************************************************************************
Constants

WINDOW_BITS = 15 selects the maximum (32KB) sliding window. WANT_GZ = 16 is added to the windowBits parameter of zlib's
inflateInit2/deflateInit2 to request the gzip wrapper (with magic header and CRC32) instead of the bare deflate or zlib stream.
When the filter is constructed with raw=true, WANT_GZ is omitted so the resulting stream is plain deflate -- this is required
when the gzip framing is provided (or expected) by an outer layer.
***********************************************************************************************************************************/
#define WINDOW_BITS                                                 15
#define WANT_GZ                                                     16

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Process gz errors
FN_EXTERN int gzError(int error);

#endif
