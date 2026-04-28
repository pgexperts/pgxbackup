/***********************************************************************************************************************************
Page Checksum Filter

Check all pages in a PostgreSQL relation to ensure the checksums are valid.

This is a soft check: page errors are reported in the manifest and surfaced as warnings, but they do NOT fail the backup. The
intent is early detection of on-disk corruption while still capturing whatever is salvageable. See command/backup/backup.c
backupJobResult() and the README "Notice of Obsolescence" for the rationale.
***********************************************************************************************************************************/
#ifndef COMMAND_BACKUP_PAGE_CHECKSUM_H
#define COMMAND_BACKUP_PAGE_CHECKSUM_H

#include "common/io/filter/filter.h"
#include "postgres/interface.h"

/***********************************************************************************************************************************
Filter type constant
***********************************************************************************************************************************/
#define PAGE_CHECKSUM_FILTER_TYPE                                   STRID5("pg-chksum", 0xdacd681ecf00)

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoFilter *pageChecksumNew(
    unsigned int segmentNo, unsigned int segmentPageTotal, PgPageSize pageSize, bool headerCheck, const String *fileName);
FN_EXTERN IoFilter *pageChecksumNewPack(const Pack *paramList);

#endif
