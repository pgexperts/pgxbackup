/***********************************************************************************************************************************
IO Read Interface Internal

Backend hooks for IoRead drivers (fdRead, bufferRead, limitRead, storage backends, etc.). Drivers fill in only the function pointers
they need: read() is mandatory, eof/open/close/fd/ready are optional. The block flag tells IoRead whether the driver guarantees a
full buffer per call (true for files) or may return short reads (false for sockets); this changes how ioReadInternal() bounds the
driver's input buffer and whether it loops to fill the caller's buffer.
***********************************************************************************************************************************/
#ifndef COMMON_IO_READ_INTERN_H
#define COMMON_IO_READ_INTERN_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
#include "common/io/read.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
typedef struct IoReadInterface
{
    bool block;                                               // Do reads block when buffer is larger than available bytes?

    bool (*eof)(void *driver);
    void (*close)(void *driver);
    bool (*open)(void *driver);
    int (*fd)(const void *driver);
    size_t (*read)(void *driver, Buffer *buffer, bool block);

    // Are there bytes ready to read immediately? There are no guarantees on how much data is available to read but it must be at
    // least one byte. Optionally error when read is not ready.
    bool (*ready)(void *driver, bool error);
} IoReadInterface;

#define ioReadNewP(driver, ...)                                                                                                    \
    ioReadNew(driver, (IoReadInterface){__VA_ARGS__})

FN_EXTERN IoRead *ioReadNew(void *driver, IoReadInterface interface);

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
typedef struct IoReadPub
{
    void *driver;                                                   // Driver object
    IoReadInterface interface;                                      // Driver interface
    IoFilterGroup *filterGroup;                                     // IO filters
    bool eofAll;                                                    // Is the read done (read and filters complete)?

#ifdef DEBUG
    bool opened;                                                    // Has the io been opened?
    bool closed;                                                    // Has the io been closed?
#endif
} IoReadPub;

// Driver for the read object
FN_INLINE_ALWAYS void *
ioReadDriver(const IoRead *const this)
{
    return THIS_PUB(IoRead)->driver;
}

// Interface for the read object
FN_INLINE_ALWAYS const IoReadInterface *
ioReadInterface(const IoRead *const this)
{
    return &THIS_PUB(IoRead)->interface;
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_IO_READ_INTERFACE_TYPE                                                                                        \
    IoReadInterface
#define FUNCTION_LOG_IO_READ_INTERFACE_FORMAT(value, buffer, bufferSize)                                                           \
    objNameToLog(&value, "IoReadInterface", buffer, bufferSize)

#endif
