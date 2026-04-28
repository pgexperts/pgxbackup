/***********************************************************************************************************************************
Archive Push Command

Implements PostgreSQL's archive_command. In sync mode each WAL segment is pushed to every configured repo before returning. In
async mode the foreground call ack's PostgreSQL fast and a background worker drains the archive_status/.ready queue in parallel,
optionally dropping the oldest segments if archive-push-queue-max is reached so pg_wal does not fill the disk.
***********************************************************************************************************************************/
#ifndef COMMAND_ARCHIVE_PUSH_PUSH_H
#define COMMAND_ARCHIVE_PUSH_PUSH_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Push a WAL segment to the repository
FN_EXTERN void cmdArchivePush(void);

// Async version of archive push that runs in parallel for performance
FN_EXTERN void cmdArchivePushAsync(void);

#endif
