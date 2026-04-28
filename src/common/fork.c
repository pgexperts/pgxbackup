/***********************************************************************************************************************************
Fork Handler

forkSafe() is a thin error-checking wrapper around fork(2). forkDetach() is the classic double-fork daemonize sequence used by
async archive-push: setsid() to leave the controlling tty, then fork+exit so the surviving grandchild is no longer a session
leader (and so cannot reacquire a tty), close the std streams, and chdir to / to break any cwd dependency on the caller. The
SIGCHLD ignore between the two forks reaps the intermediate child without a waitpid; it is reset to SIG_DFL afterward so the
detached process can use waitpid() normally on its own children.
***********************************************************************************************************************************/
#include <build.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/debug.h"
#include "common/fork.h"
#include "common/log.h"

/**********************************************************************************************************************************/
FN_EXTERN int
forkSafe(void)
{
    FUNCTION_LOG_VOID(logLevelTrace);

    const int result = fork();

    THROW_ON_SYS_ERROR(result == -1, KernelError, "unable to fork");

    FUNCTION_LOG_RETURN(INT, result);
}

/**********************************************************************************************************************************/
FN_EXTERN void
forkDetach(void)
{
    FUNCTION_LOG_VOID(logLevelTrace);

    // Make this process a group leader so the parent process won't block waiting for it to finish
    THROW_ON_SYS_ERROR(setsid() == -1, KernelError, "unable to create new session group");

    // The process should never receive a SIGHUP but ignore it just in case
    signal(SIGHUP, SIG_IGN);

    // There should be no way the child process can exit first (after the next fork) but just in case ignore SIGCHLD. This means
    // that the child process will automatically be reaped by the kernel should it finish first rather than becoming defunct.
    signal(SIGCHLD, SIG_IGN);

    // Fork again and let the parent process terminate to ensure that we get rid of the session leading process. Only session
    // leaders may get a TTY again.
    if (forkSafe() != 0)
        exit(0);

    // Reset SIGCHLD to the default handler so waitpid() calls in the process will work as expected
    signal(SIGCHLD, SIG_DFL);

    // Change the working directory to / so there is no dependency on the original working directory
    THROW_ON_SYS_ERROR(chdir("/") == -1, PathMissingError, "unable to change directory to '/'");

    // Close standard file descriptors
    THROW_ON_SYS_ERROR(close(STDIN_FILENO) == -1, FileCloseError, "unable to close stdin");
    THROW_ON_SYS_ERROR(close(STDOUT_FILENO) == -1, FileCloseError, "unable to close stdout");
    THROW_ON_SYS_ERROR(close(STDERR_FILENO) == -1, FileCloseError, "unable to close stderr");

    FUNCTION_LOG_RETURN_VOID();
}
