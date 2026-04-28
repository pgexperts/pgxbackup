/***********************************************************************************************************************************
Buffer IO Write

Write to a Buffer object using the IoWrite interface. The destination buffer is borrowed and grows as bytes are appended. Useful
for capturing the output of a filter pipeline in memory (e.g. building a compressed/encrypted payload to ship over the protocol).
***********************************************************************************************************************************/
#ifndef COMMON_IO_BUFFERWRITE_H
#define COMMON_IO_BUFFERWRITE_H

#include "common/io/write.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoWrite *ioBufferWriteNew(Buffer *buffer);

// Construct and open buffer write
FN_INLINE_ALWAYS IoWrite *
ioBufferWriteNewOpen(Buffer *const buffer)
{
    IoWrite *const result = ioBufferWriteNew(buffer);
    ioWriteOpen(result);
    return result;
}

#endif
