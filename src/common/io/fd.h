/***********************************************************************************************************************************
File Descriptor Functions

Helpers around poll() that wait for a file descriptor to become readable or writable within a deadline. Used by fdRead/fdWrite and
the socket/tls backends to implement the IoRead/IoWrite ready hooks and to bound blocking operations by ioTimeoutMs().
***********************************************************************************************************************************/
#ifndef COMMON_IO_FD_H
#define COMMON_IO_FD_H

#include "common/time.h"

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Wait until the file descriptor is ready to read/write or timeout
FN_EXTERN bool fdReady(int fd, bool read, bool write, TimeMSec timeout);

// Wait until the file descriptor is ready to read or timeout
FN_INLINE_ALWAYS bool
fdReadyRead(const int fd, const TimeMSec timeout)
{
    return fdReady(fd, true, false, timeout);
}

// Wait until the file descriptor is ready to write or timeout
FN_INLINE_ALWAYS bool
fdReadyWrite(const int fd, const TimeMSec timeout)
{
    return fdReady(fd, false, true, timeout);
}

#endif
