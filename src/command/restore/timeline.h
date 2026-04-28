/***********************************************************************************************************************************
Timeline Management

Verifies that the requested target timeline can actually reach the chosen backup. This catches the common "promoted standby"
mistake before restore touches any files: if the user tries to restore a backup taken from a primary on timeline N onto a target
timeline that forked off before the backup's start LSN, no WAL path connects them and recovery would never succeed.
***********************************************************************************************************************************/
#ifndef COMMAND_RESTORE_TIMELINE_H
#define COMMAND_RESTORE_TIMELINE_H

#include "common/crypto/common.h"
#include "storage/storage.h"

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Verify that target timeline is valid for a backup
FN_EXTERN void timelineVerify(
    const Storage *storageRepo, const String *archiveId, unsigned int pgVersion, unsigned int timelineBackup,
    uint64_t lsnBackup, const String *timelineTargetStr, unsigned int recoveryType, CipherType cipherType,
    const String *cipherPass);

#endif
