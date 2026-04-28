/***********************************************************************************************************************************
Io Session Interface Internal

Vtable for protocol-specific session drivers. ioRead's ignoreUnexpectedEof flag is here (rather than on the IoRead returned from
the session) because it is a session-level policy: TLS is required to send a close_notify before EOF, but some peers omit it and
the caller may want to tolerate that for compatibility.
***********************************************************************************************************************************/
#ifndef COMMON_IO_SESSION_INTERN_H
#define COMMON_IO_SESSION_INTERN_H

#include "common/io/session.h"
#include "common/io/write.h"

/***********************************************************************************************************************************
Interface
***********************************************************************************************************************************/
typedef struct IoSessionInterface
{
    // Type used to identify the session
    StringId type;

    // Close the session
    void (*close)(void *driver);

    // Session file descriptor, if any
    int (*fd)(void *driver);

    // IoRead interface for the session
    IoRead *(*ioRead)(void *driver, bool ignoreUnexpectedEof);

    // IoWrite interface for the session
    IoWrite *(*ioWrite)(void *driver);

    // Session role
    IoSessionRole (*role)(const void *driver);

    // Driver log function
    void (*toLog)(const void *driver, StringStatic *debugLog);
} IoSessionInterface;

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoSession *ioSessionNew(void *driver, const IoSessionInterface *interface);

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
// Has the session been authenticated?
FN_EXTERN void ioSessionAuthenticatedSet(IoSession *this, bool authenticated);

// Set the peer name
FN_EXTERN void ioSessionPeerNameSet(IoSession *this, const String *peerName);

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_IO_SESSION_INTERFACE_TYPE                                                                                     \
    IoSessionInterface *
#define FUNCTION_LOG_IO_SESSION_INTERFACE_FORMAT(value, buffer, bufferSize)                                                        \
    objNameToLog(&value, "IoSessionInterface", buffer, bufferSize)

#endif
