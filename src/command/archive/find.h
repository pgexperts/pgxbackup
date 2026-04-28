/***********************************************************************************************************************************
Archive Segment Find

Find a WAL segment (or segments) in a repository. The code paths for finding single or multiple WAL segments are both optimized.

The single-segment path adds a regex to the storage list call to bound the result size; the multi-segment path lists once per
prefix and consumes matches in order. Finding more than one match for the same segment indicates split brain (multiple primaries
archiving to the same stanza) and is treated as a hard error rather than a "pick one" situation.
***********************************************************************************************************************************/
#ifndef COMMAND_ARCHIVE_FIND_H
#define COMMAND_ARCHIVE_FIND_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct WalSegmentFind WalSegmentFind;

#include "common/type/string.h"
#include "storage/storage.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN WalSegmentFind *walSegmentFindNew(const Storage *storage, const String *archiveId, bool single, TimeMSec timeout);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Find a WAL segment in the repository. The file name can have several things appended such as a hash, compression extension, and
// partial extension so it is possible to have multiple files that match the segment, though more than one match is not a good
// thing.
FN_EXTERN String *walSegmentFind(WalSegmentFind *this, const String *walSegment);

/***********************************************************************************************************************************
Helper functions
***********************************************************************************************************************************/
// Find a single WAL segment (see walSegmentFind() for details)
FN_EXTERN String *walSegmentFindOne(const Storage *storage, const String *archiveId, const String *walSegment, TimeMSec timeout);

#endif
