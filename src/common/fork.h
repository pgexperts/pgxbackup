/***********************************************************************************************************************************
Fork Handler

forkSafe() is fork(2) with an error throw on failure. forkDetach() performs a textbook double-fork detach with std-stream close
and chdir to /, used by async archive-push to spawn an independent worker that survives the original invocation.
***********************************************************************************************************************************/
#ifndef COMMON_FORK_H
#define COMMON_FORK_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Fork a new process and throw an error if it fails
FN_EXTERN int forkSafe(void);

// Detach a forked process so it can continue running after the parent process has exited. This is not a typical daemon startup
// because the parent process may continue to run and perform work for some time.
FN_EXTERN void forkDetach(void);

#endif
