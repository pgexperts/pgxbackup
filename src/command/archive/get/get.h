/***********************************************************************************************************************************
Archive Get Command

Implements PostgreSQL's restore_command. In sync mode each WAL request triggers an immediate repo lookup. In async mode the
foreground process places the request in the spool, forks a long-lived async worker, and then loops waiting for the .ok file; the
async worker fetches the requested segment plus a look-ahead window so subsequent requests are served from local disk.
***********************************************************************************************************************************/
#ifndef COMMAND_ARCHIVE_GET_GET_H
#define COMMAND_ARCHIVE_GET_GET_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Get an archive file from the repository (WAL segment, history file, etc.)
FN_EXTERN int cmdArchiveGet(void);

// Async version of archive get that runs in parallel for performance
FN_EXTERN void cmdArchiveGetAsync(void);

#endif
