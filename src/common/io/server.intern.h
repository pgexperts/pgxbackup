/***********************************************************************************************************************************
Io Server Interface Internal

Vtable populated by protocol-specific server drivers. accept() takes an existing inner IoSession (from a lower layer such as the
socket server) and returns a new IoSession wrapping it -- this is what allows TLS to layer on top of a plain socket without the
caller knowing which transport is which.
***********************************************************************************************************************************/
#ifndef COMMON_IO_SERVER_INTERN_H
#define COMMON_IO_SERVER_INTERN_H

#include "common/io/server.h"
#include "common/io/session.h"
#include "common/type/string.h"

/***********************************************************************************************************************************
Interface
***********************************************************************************************************************************/
typedef struct IoServerInterface
{
    // Type used to identify the server
    StringId type;

    // Server name, usually address:port or some other unique identifier
    const String *(*name)(void *driver);

    // Accept a session
    IoSession *(*accept)(void *driver, IoSession *session);

    // Driver log function
    void (*toLog)(const void *driver, StringStatic *debugLog);
} IoServerInterface;

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoServer *ioServerNew(void *driver, const IoServerInterface *interface);

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_IO_SERVER_INTERFACE_TYPE                                                                                      \
    IoServerInterface *
#define FUNCTION_LOG_IO_SERVER_INTERFACE_FORMAT(value, buffer, bufferSize)                                                         \
    objNameToLog(&value, "IoServerInterface", buffer, bufferSize)

#endif
