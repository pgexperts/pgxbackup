/***********************************************************************************************************************************
Common Functions and Definitions for Backup and Expire Commands

Shared label parsing/formatting and repo-path layout helpers. The label format is the public contract for backup naming
(YYYYMMDD-HHMMSS{F,F_YYYYMMDD-HHMMSS{D,I}}) and is compared lexicographically by the expire command — preserving this ordering is
load-bearing.
***********************************************************************************************************************************/
#ifndef COMMAND_BACKUP_COMMON_H
#define COMMAND_BACKUP_COMMON_H

#include <stdbool.h>
#include <time.h>

#include "common/compress/helper.h"
#include "common/type/string.h"
#include "info/infoBackup.h"

/***********************************************************************************************************************************
Backup constants
***********************************************************************************************************************************/
#define BACKUP_PATH_HISTORY                                         "backup.history"
#define BACKUP_BLOCK_INCR_EXT                                       ".pgbi"

// Date and time must be in %Y%m%d-%H%M%S format, for example 20220901-193409
#define DATE_TIME_REGEX                                             "[0-9]{8}\\-[0-9]{6}"
#define DATE_TIME_LEN                                               (8 + 1 + 6)

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Determine the path/file where the file is backed up in the repo
typedef struct BackupFileRepoPathParam
{
    const String *manifestName;                                     // File name in manifest
    uint64_t bundleId;                                              // Is the file bundled?
    CompressType compressType;                                      // Is the file compressed?
    bool blockIncr;                                                 // Is the file a block incremental?
} BackupFileRepoPathParam;

#define backupFileRepoPathP(backupLabel, ...)                                                                                          \
    backupFileRepoPath(backupLabel, (BackupFileRepoPathParam){__VA_ARGS__})

FN_EXTERN String *backupFileRepoPath(const String *backupLabel, BackupFileRepoPathParam param);

// Format a backup label from a type and timestamp with an optional prior label
FN_EXTERN String *backupLabelFormat(BackupType type, const String *backupLabelPrior, time_t timestamp);

// Returns an anchored regex string for filtering backups based on the type (at least one type is required to be true)
typedef struct BackupRegExpParam
{
    bool full;
    bool differential;
    bool incremental;
    bool noAnchorEnd;
} BackupRegExpParam;

#define backupRegExpP(...)                                                                                                         \
    backupRegExp((BackupRegExpParam){__VA_ARGS__})

FN_EXTERN String *backupRegExp(BackupRegExpParam param);

// Create a symlink to the specified backup (if symlinks are supported)
FN_EXTERN void backupLinkLatest(const String *backupLabel, unsigned int repoIdx);

#endif
