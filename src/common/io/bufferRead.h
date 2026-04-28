/***********************************************************************************************************************************
Buffer IO Read

Read from a Buffer object using the IoRead interface. The buffer is borrowed (not copied) -- the caller must keep it alive for the
lifetime of the IoRead. Useful for in-memory pipelines, e.g. running checksum/compression filters over a buffer without involving
any file descriptors.
***********************************************************************************************************************************/
#ifndef COMMON_IO_BUFFERREAD_H
#define COMMON_IO_BUFFERREAD_H

#include "common/io/read.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoRead *ioBufferReadNew(const Buffer *buffer);

// Construct and open buffer read
FN_INLINE_ALWAYS IoRead *
ioBufferReadNewOpen(const Buffer *const buffer)
{
    IoRead *const result = ioBufferReadNew(buffer);
    ioReadOpen(result);
    return result;
}

#endif
