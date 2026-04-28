/***********************************************************************************************************************************
IO Functions

Top-level helpers and process-wide configuration for the IO layer. The IO layer is composed of three abstractions: IoRead/IoWrite
(byte streams over files, sockets, buffers, etc.), IoFilter/IoFilterGroup (composable byte transforms such as compression,
encryption, hashing, and size counting), and IoClient/IoServer/IoSession (connection-oriented protocol drivers built on top of an
IoRead and IoWrite pair). The buffer size and timeout below are global tuning knobs that all of these layers consult.
***********************************************************************************************************************************/
#ifndef COMMON_IO_IO_H
#define COMMON_IO_IO_H

#include <stddef.h>

#include <common/io/read.h>
#include <common/io/write.h>
#include <common/time.h>

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Copy data from source to destination (both must be open and neither will be closed)
typedef struct IoCopyParam
{
    VAR_PARAM_HEADER;
    const Variant *limit;                                           // Limit bytes to copy from source
} IoCopyParam;

#define ioCopyP(source, destination, ...)                                                                                          \
    ioCopy(source, destination, (IoCopyParam){VAR_PARAM_INIT, __VA_ARGS__})

FN_EXTERN void ioCopy(IoRead *source, IoWrite *destination, IoCopyParam param);

// Read all IO into a buffer
FN_EXTERN Buffer *ioReadBuf(IoRead *read);

// Read all IO but don't store it. Useful for calculating checksums, size, etc.
FN_EXTERN bool ioReadDrain(IoRead *read);

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
// Buffer size
FN_EXTERN size_t ioBufferSize(void);
FN_EXTERN void ioBufferSizeSet(size_t bufferSize);

// I/O timeout in milliseconds. Used to timeout on connections and read/write operations. Note that an *entire* read/write operation
// does not need to take place within this timeout but at least some progress needs to be made, even if it is only a byte.
FN_EXTERN TimeMSec ioTimeoutMs(void);
FN_EXTERN void ioTimeoutMsSet(TimeMSec timeout);

#endif
