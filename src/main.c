/***********************************************************************************************************************************
Main

Entry point for the pgxbackup binary. Performs one-time initialization (storage backend registry, command timing, statistics,
exit/signal handlers), parses the configuration, and dispatches to a command handler. The same binary serves four distinct command
roles (main/async/local/remote); when invoked as a worker process by the parallel dispatcher (cfgCmdRoleLocal/Remote) it skips the
user-facing command switch and instead becomes a ProtocolServer reading requests from stdin and writing responses to stdout.
***********************************************************************************************************************************/
#include <build.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "command/annotate/annotate.h"
#include "command/archive/get/get.h"
#include "command/archive/push/push.h"
#include "command/backup/backup.h"
#include "command/check/check.h"
#include "command/command.h"
#include "command/control/start.h"
#include "command/control/stop.h"
#include "command/exit.h"
#include "command/expire/expire.h"
#include "command/help/help.h"
#include "command/info/info.h"
#include "command/local/local.h"
#include "command/lock.h"
#include "command/manifest/manifest.h"
#include "command/remote/remote.h"
#include "command/repo/get.h"
#include "command/repo/ls.h"
#include "command/repo/put.h"
#include "command/repo/rm.h"
#include "command/restore/restore.h"
#include "command/server/ping.h"
#include "command/server/server.h"
#include "command/stanza/create.h"
#include "command/stanza/delete.h"
#include "command/stanza/upgrade.h"
#include "command/verify/verify.h"
#include "common/debug.h"
#include "common/io/fdRead.h"
#include "common/io/fdWrite.h"
#include "common/stat.h"
#include "config/config.h"
#include "config/load.h"
#include "postgres/interface.h"
#include "protocol/helper.h"
#include "storage/azure/helper.h"
#include "storage/cifs/helper.h"
#include "storage/gcs/helper.h"
#include "storage/helper.h"
#include "storage/s3/helper.h"
#include "storage/sftp/helper.h"
#include "version.h"

/***********************************************************************************************************************************
Include automatically generated help data
***********************************************************************************************************************************/
#include "command/help/help.auto.c.inc"

int
main(int argListSize, const char *argList[])
{
    // Set stack trace and mem context error cleanup handlers. These run during error unwinding -- registering them before any
    // other init means a failure in cmdInit/statInit/exitInit/cfgLoad still gets a clean backtrace and contexts get freed.
    static const ErrorHandlerFunction errorHandlerList[] = {stackTraceClean, memContextClean};
    errorHandlerSet(errorHandlerList, LENGTH_OF(errorHandlerList));

    // Register the optional cloud/network storage backends. Posix is always available and is registered separately by
    // storageHelperInit(). SFTP is conditional on libssh2; the rest compile in unconditionally but only activate when configured.
    // STORAGE_END_HELPER is the sentinel that terminates the list.
    static const StorageHelper storageHelperList[] =
    {
        STORAGE_AZURE_HELPER,
        STORAGE_CIFS_HELPER,
        STORAGE_GCS_HELPER,
        STORAGE_S3_HELPER,
#ifdef HAVE_LIBSSH2
        STORAGE_SFTP_HELPER,
#endif
        STORAGE_END_HELPER
    };

    storageHelperInit(storageHelperList);

    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(INT, argListSize);
        FUNCTION_LOG_PARAM(CHARPY, argList);
    FUNCTION_LOG_END();

    // Initialize command with the start time. Must run before cfgLoad so the elapsed-time message at the end of the command
    // captures the full duration, not just time spent after config parsing.
    cmdInit();

    // Initialize statistics collector
    statInit();

    // Initialize exit handler. Installs signal handlers for SIGHUP/SIGINT/SIGTERM and ignores SIGPIPE (writes return EPIPE
    // instead). Done before cfgLoad so a SIGINT during config parsing exits cleanly through exitSafe().
    exitInit();

    // Process commands
    volatile int result = 0;
    volatile bool error = false;

    TRY_BEGIN()
    {
        // Load the configuration
        // -------------------------------------------------------------------------------------------------------------------------
        cfgLoad((unsigned int)argListSize, argList);
        const ConfigCommandRole commandRole = cfgCommandRole();

        // Main/async commands
        // -------------------------------------------------------------------------------------------------------------------------
        // Main is the user-facing role; async is a detached worker spawned for archive-get/archive-push to return control to
        // PostgreSQL's archive_command quickly while work continues in the background. Both share the same per-command dispatch
        // table below; the few commands that distinguish (archive-get/archive-push) branch on commandRole inline.
        if (commandRole == cfgCmdRoleMain || commandRole == cfgCmdRoleAsync)
        {
            switch (cfgCommandHelp() ? cfgCmdHelp : cfgCommand())
            {
                // Annotate command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdAnnotate:
                    cmdAnnotate();
                    break;

                // Archive get command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdArchiveGet:
                {
                    if (commandRole == cfgCmdRoleAsync)
                        cmdArchiveGetAsync();
                    else
                        result = cmdArchiveGet();

                    break;
                }

                // Archive push command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdArchivePush:
                {
                    if (commandRole == cfgCmdRoleAsync)
                        cmdArchivePushAsync();
                    else
                        cmdArchivePush();

                    break;
                }

                // Backup command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdBackup:
                {
                    // Run backup
                    cmdBackup();

                    // Auto-expire chains directly off backup so the user sees a single command. cmdEnd/cfgCommandSet/cmdBegin
                    // re-frame the log output as if "expire" had been invoked separately, and cmdLockWriteP clears the percent
                    // complete left over from the backup so a parallel `info` query does not show stale progress.
                    if (cfgOptionBool(cfgOptExpireAuto))
                    {
                        // Switch to expire command
                        cmdEnd(0, NULL);
                        cfgCommandSet(cfgCmdExpire, cfgCmdRoleMain);
                        cfgLoadLogFile();
                        cmdBegin();

                        // Null out any backup percent complete value in the backup lock file
                        cmdLockWriteP();

                        // Run expire
                        cmdExpire();
                    }

                    break;
                }

                // Check command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdCheck:
                    cmdCheck();
                    break;

                // Expire command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdExpire:
                    cmdExpire();
                    break;

                // Info command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdInfo:
                    cmdInfo();
                    break;

                // Manifest command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdManifest:
                    cmdManifest();
                    break;

                // Repository get file command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdRepoGet:
                    result = cmdStorageGet();
                    break;

                // Repository list paths/files command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdRepoLs:
                    cmdStorageList();
                    break;

                // Repository put file command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdRepoPut:
                    cmdStoragePut();
                    break;

                // Repository remove paths/files command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdRepoRm:
                    cmdStorageRemove();
                    break;

                // Server command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdServer:
                    cmdServer((unsigned int)argListSize, argList);
                    break;

                // Server ping command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdServerPing:
                    cmdServerPing();
                    break;

                // Restore command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdRestore:
                    cmdRestore();
                    break;

                // Stanza create command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdStanzaCreate:
                    cmdStanzaCreate();
                    break;

                // Stanza delete command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdStanzaDelete:
                    cmdStanzaDelete();
                    break;

                // Stanza upgrade command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdStanzaUpgrade:
                    cmdStanzaUpgrade();
                    break;

                // Start command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdStart:
                    cmdStart();
                    break;

                // Stop command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdStop:
                    cmdStop();
                    break;

                // Verify command
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdVerify:
                    cmdVerify();
                    break;

                // Help/version commands
                // -----------------------------------------------------------------------------------------------------------------
                case cfgCmdHelp:
                case cfgCmdVersion:
                    cmdHelp(BUF(helpData, sizeof(helpData)));
                    break;
            }
        }
        // Local/remote commands
        // -------------------------------------------------------------------------------------------------------------------------
        // These roles are not invoked by users; they are spawned by the parallel dispatcher (locals on the same host, remotes
        // over SSH or TLS). The process inherits stdin/stdout as the protocol pipe and becomes a ProtocolServer that loops on
        // requests until it receives an exit message. The command-specific switch above is bypassed; cmdLocal/cmdRemote install
        // their own handler tables for the binary protocol.
        else
        {
            ASSERT(commandRole == cfgCmdRoleLocal || commandRole == cfgCmdRoleRemote);

            const String *const service = commandRole == cfgCmdRoleLocal ? PROTOCOL_SERVICE_LOCAL_STR : PROTOCOL_SERVICE_REMOTE_STR;
            const String *const name = strNewFmt("%s-%s", strZ(service), strZ(cfgOptionDisplay(cfgOptProcess)));
            const TimeMSec timeout = cfgOptionUInt64(cfgOptProtocolTimeout);
            ProtocolServer *const server = protocolServerNew(
                name, service, ioFdReadNewOpen(name, STDIN_FILENO, timeout), ioFdWriteNewOpen(name, STDOUT_FILENO, timeout));

            if (commandRole == cfgCmdRoleLocal)
                cmdLocal(server);
            else
                cmdRemote(server);
        }
    }
    CATCH_FATAL()
    {
        error = true;
        result = exitSafe(result, true, 0);
    }
    TRY_END();

    // Free protocol objects
    protocolFree();

    FUNCTION_LOG_RETURN(INT, error ? result : exitSafe(result, false, 0));
}
