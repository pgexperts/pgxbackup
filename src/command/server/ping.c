/***********************************************************************************************************************************
Server Ping Command

Liveness probe for the TLS server. Connects without certificate verification (any CA, any peer name) and issues a single NOOP
through the protocol layer. A successful round-trip means the server is up and TLS is responsive; the connection is then
closed. Used by monitoring tools and during testing to confirm the server is reachable without exercising any of the
auth-protected commands.
***********************************************************************************************************************************/
#include <build.h>

#include "command/server/ping.h"
#include "common/debug.h"
#include "common/io/socket/client.h"
#include "common/io/tls/client.h"
#include "config/config.h"
#include "protocol/client.h"
#include "protocol/helper.h"

FN_EXTERN void
cmdServerPing(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Check for user-specified host
        const String *host = cfgOptionStr(cfgOptTlsServerAddress);
        const StringList *const commandParam = cfgCommandParam();

        if (strLstSize(commandParam) == 1)
            host = strLstGet(commandParam, 0);
        else if (strLstSize(commandParam) > 1)
            THROW(ParamInvalidError, "extra parameters found");

        // Connect to server without any verification
        const TimeMSec timeout = cfgOptionUInt64(cfgOptIoTimeout);

        IoClient *const tlsClient = tlsClientNewP(
            sckClientNew(host, cfgOptionUInt(cfgOptTlsServerPort), timeout, timeout), host, timeout, timeout, false);
        IoSession *const tlsSession = ioClientOpen(tlsClient);

        // Send ping
        ProtocolClient *const protocolClient = protocolClientNew(
            strNewFmt(PROTOCOL_SERVICE_REMOTE " socket protocol on '%s'", strZ(host)), PROTOCOL_SERVICE_REMOTE_STR,
            ioSessionIoReadP(tlsSession), ioSessionIoWrite(tlsSession));
        protocolClientNoExit(protocolClient);
        protocolClientNoOp(protocolClient);
        protocolClientFree(protocolClient);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
