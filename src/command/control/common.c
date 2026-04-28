/***********************************************************************************************************************************
Common Handler for Control Commands

Stop file machinery shared by start/stop. A stop file (one per stanza, plus an "all" stop file) is checked at the start of
operations that should not run while the operator has paused them. Tested by lockStopTest(); created/removed by cmdStart and
cmdStop. The stop file is separate from the per-command lock so it persists across processes and survives a reboot.
***********************************************************************************************************************************/
#include <build.h>

#include "command/control/common.h"
#include "common/debug.h"
#include "config/config.h"
#include "storage/helper.h"

/**********************************************************************************************************************************/
FN_EXTERN String *
lockStopFileName(const String *const stanza)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, stanza);
    FUNCTION_TEST_END();

    FUNCTION_TEST_RETURN(
        STRING, strNewFmt("%s/%s" STOP_FILE_EXT, strZ(cfgOptionStr(cfgOptLockPath)), stanza != NULL ? strZ(stanza) : "all"));
}

/**********************************************************************************************************************************/
FN_EXTERN void
lockStopTest(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Check the current stanza (if any)
        if (cfgOptionTest(cfgOptStanza))
        {
            if (storageExistsP(storageLocal(), lockStopFileName(cfgOptionStr(cfgOptStanza))))
                THROW_FMT(StopError, "stop file exists for stanza %s", strZ(cfgOptionDisplay(cfgOptStanza)));
        }

        // Check all stanzas
        if (storageExistsP(storageLocal(), lockStopFileName(NULL)))
            THROW(StopError, "stop file exists for all stanzas");
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
