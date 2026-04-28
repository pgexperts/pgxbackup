/***********************************************************************************************************************************
Read Limited Data

Wraps an existing IoRead and reports EOF after `limit` bytes have been read, leaving any remaining bytes in the underlying stream
untouched. Used to consume fixed-length records from a longer stream (e.g. one chunk out of a multi-chunk protocol response).
***********************************************************************************************************************************/
#ifndef COMMON_IO_LIMITREAD_H
#define COMMON_IO_LIMITREAD_H

#include "common/io/read.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN IoRead *ioLimitReadNew(IoRead *read, uint64_t limit);

#endif
