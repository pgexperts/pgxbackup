/***********************************************************************************************************************************
Restore Command

Top-level entry point for the restore command. Selects a backup, validates the timeline against history files, cleans/builds the
target data directory, dispatches parallel file copies through the protocol layer, writes recovery settings, and renames pg_control
last so that an aborted restore cannot be started by PostgreSQL.
***********************************************************************************************************************************/
#ifndef COMMAND_RESTORE_RESTORE_H
#define COMMAND_RESTORE_RESTORE_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Restore a backup
FN_EXTERN void cmdRestore(void);

#endif
