/***********************************************************************************************************************************
IO Write Interface

Objects that write to some IO destination (file, socket, etc.) are implemented using this interface. All objects are required to
implement IoWriteProcess and can optionally implement IoWriteOpen or IoWriteClose. IoWriteOpen and IoWriteClose can be used to
allocate/open or deallocate/free resources. An example of an IoWrite object is IoBufferWrite.

Lifecycle and ordering invariants:
  1. ioFilterGroupAdd() on the write's filter group -- must happen before ioWriteOpen().
  2. ioWriteOpen() -- locks in the filter chain.
  3. ioWrite() / ioWriteLine() / ioWriteStr() / ioWriteVarIntU64() -- any number of times.
  4. ioWriteClose() -- flushes filters with NULL input until they all report done, then closes the driver.

Bytes flow opposite to the read side: caller's buffer -> filter[0] -> filter[1] -> ... -> filter[N-1] -> driver. ioWriteFlush() and
ioWriteSeek() are only valid when *no* filters have been added (asserted via filterGroupSet) because filters may hold back bytes
that would then end up at the wrong offset.
***********************************************************************************************************************************/
#ifndef COMMON_IO_WRITE_H
#define COMMON_IO_WRITE_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct IoWrite IoWrite;

#include "common/io/filter/group.h"
#include "common/io/write.intern.h"
#include "common/type/buffer.h"
#include "common/type/object.h"

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
// Filter group. Filters must be set before open and cannot be reset
FN_INLINE_ALWAYS IoFilterGroup *
ioWriteFilterGroup(IoWrite *const this)
{
    return THIS_PUB(IoWrite)->filterGroup;
}

// File descriptor for the write object. Not all write objects have a file descriptor and -1 will be returned in that case.
FN_EXTERN int ioWriteFd(const IoWrite *this);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Open the IO
FN_EXTERN void ioWriteOpen(IoWrite *this);

// Write data to IO and process filters
FN_EXTERN void ioWrite(IoWrite *this, const Buffer *buffer);

// Write linefeed-terminated buffer
FN_EXTERN void ioWriteLine(IoWrite *this, const Buffer *buffer);

// Can bytes be written immediately? There are no guarantees on how much data can be written but it must be at least one byte.
typedef struct IoWriteReadyParam
{
    VAR_PARAM_HEADER;
    bool error;                                                     // Error when write not ready
} IoWriteReadyParam;

#define ioWriteReadyP(this, ...)                                                                                                   \
    ioWriteReady(this, (IoWriteReadyParam){VAR_PARAM_INIT, __VA_ARGS__})

FN_EXTERN bool ioWriteReady(IoWrite *this, IoWriteReadyParam param);

// Write string
FN_EXTERN void ioWriteStr(IoWrite *this, const String *string);

// Write linefeed-terminated string
FN_EXTERN void ioWriteStrLine(IoWrite *this, const String *string);

// Write varint-128 encoding
FN_EXTERN void ioWriteVarIntU64(IoWrite *this, uint64_t value);

// Flush any data in the output buffer. This does not end writing and will not work if filters are present (asserted) because a
// filter may legitimately hold back bytes until more input arrives or until close-time flushing.
FN_EXTERN void ioWriteFlush(IoWrite *this);

// Seek to specified position relative to beginning of write. Forces a flush first so any buffered output lands at the *old*
// position rather than after the seek. Not valid with filters for the same reason as ioWriteFlush().
FN_EXTERN void ioWriteSeek(IoWrite *this, uint64_t position);

// Close the IO and write any additional data that has not been written yet. Drives the filter chain with NULL inputs until every
// filter reports done -- this is what gives compression and encryption filters a chance to emit their trailing blocks.
FN_EXTERN void ioWriteClose(IoWrite *this);

/***********************************************************************************************************************************
Destructor
***********************************************************************************************************************************/
FN_INLINE_ALWAYS void
ioWriteFree(IoWrite *const this)
{
    objFree(this);
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_IO_WRITE_TYPE                                                                                                 \
    IoWrite *
#define FUNCTION_LOG_IO_WRITE_FORMAT(value, buffer, bufferSize)                                                                    \
    objNameToLog(value, "IoWrite", buffer, bufferSize)

#endif
