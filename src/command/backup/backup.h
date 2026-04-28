/***********************************************************************************************************************************
Backup Command

Top-level entry point for the backup command. Coordinates the full lifecycle: connect to the cluster, build the manifest,
optionally resume a prior aborted backup, dispatch parallel file copy jobs through the protocol layer, copy/check WAL needed for
consistency, and persist the manifest plus updated backup.info into the repo.
***********************************************************************************************************************************/
#ifndef COMMAND_BACKUP_BACKUP_H
#define COMMAND_BACKUP_BACKUP_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Make a backup
FN_EXTERN void cmdBackup(void);

#endif
