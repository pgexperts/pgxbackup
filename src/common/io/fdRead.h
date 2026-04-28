/***********************************************************************************************************************************
File Descriptor Read

Read from a file descriptor using the IoRead interface. Configured as a blocking driver (block=true) -- the read loop in fdRead.c
keeps calling read(2) until the caller's buffer is full or EOF is reached. Combine via ioReadFilterGroup() for compression,
checksums, or size counting.
***********************************************************************************************************************************/
#ifndef COMMON_IO_FDREAD_H
#define COMMON_IO_FDREAD_H

#include "common/io/read.h"
#include "common/time.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoRead *ioFdReadNew(const String *name, int fd, TimeMSec timeout);

// Construct and open read fd
FN_INLINE_ALWAYS IoRead *
ioFdReadNewOpen(const String *const name, const int fd, const TimeMSec timeout)
{
    IoRead *const result = ioFdReadNew(name, fd, timeout);
    ioReadOpen(result);
    return result;
}

#endif
