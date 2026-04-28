/***********************************************************************************************************************************
Io Server Interface

Create sessions for protocol servers. For example, a TLS server can be created with tlsServerNew() and then new TLS sessions can be
accepted with ioServerAccept().

IoServer is the listening end of the IoClient/IoServer pair. ioServerAccept() takes an *existing* IoSession (typically the raw
socket session that just accepted a connection) and wraps/upgrades it -- e.g. the TLS server takes a plain socket session and
returns a TLS-encrypted session on top of it.
***********************************************************************************************************************************/
#ifndef COMMON_IO_SERVER_H
#define COMMON_IO_SERVER_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct IoServer IoServer;

#include "common/io/server.intern.h"
#include "common/io/session.h"
#include "common/type/object.h"

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
typedef struct IoServerPub
{
    void *driver;                                                   // Driver object
    const IoServerInterface *interface;                             // Driver interface
} IoServerPub;

// Name that identifies the server
FN_INLINE_ALWAYS const String *
ioServerName(const IoServer *const this)
{
    return THIS_PUB(IoServer)->interface->name(THIS_PUB(IoServer)->driver);
}

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Move to a new parent mem context
FN_INLINE_ALWAYS IoServer *
ioServerMove(IoServer *const this, MemContext *const parentNew)
{
    return objMove(this, parentNew);
}

// Open session
FN_INLINE_ALWAYS IoSession *
ioServerAccept(IoServer *const this, IoSession *const session)
{
    return THIS_PUB(IoServer)->interface->accept(THIS_PUB(IoServer)->driver, session);
}

/***********************************************************************************************************************************
Destructor
***********************************************************************************************************************************/
FN_INLINE_ALWAYS void
ioServerFree(IoServer *const this)
{
    objFree(this);
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
FN_EXTERN void ioServerToLog(const IoServer *this, StringStatic *debugLog);

#define FUNCTION_LOG_IO_SERVER_TYPE                                                                                                \
    IoServer *
#define FUNCTION_LOG_IO_SERVER_FORMAT(value, buffer, bufferSize)                                                                   \
    FUNCTION_LOG_OBJECT_FORMAT(value, ioServerToLog, buffer, bufferSize)

#endif
