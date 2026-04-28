/***********************************************************************************************************************************
Blob Handler

Bump-allocator-style packing of small allocations into 64KiB blocks. Returned pointers are stable for the lifetime of the Blob
(the allocator never frees individual entries), so callers must keep the Blob alive as long as any of its pointers are used.
Allocations larger than BLOB_BLOCK_SIZE bypass the block packing and get a dedicated allocation.
***********************************************************************************************************************************/
#include <build.h>

#include <string.h>

#include "common/debug.h"
#include "common/memContext.h"
#include "common/type/blob.h"

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
struct Blob
{
    char *block;                                                    // Current block for writing
    size_t pos;                                                     // Position in current block
};

/**********************************************************************************************************************************/
FN_EXTERN Blob *
blbNew(void)
{
    FUNCTION_TEST_VOID();

    OBJ_NEW_BEGIN(Blob, .allocQty = MEM_CONTEXT_QTY_MAX)
    {
        *this = (Blob)
        {
            .block = memNew(BLOB_BLOCK_SIZE),
        };
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(BLOB, this);
}

/**********************************************************************************************************************************/
FN_EXTERN const void *
blbAdd(Blob *const this, const void *const data, const size_t size)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(BLOB, this);
        FUNCTION_TEST_PARAM_P(VOID, data);
        FUNCTION_TEST_PARAM(SIZE, size);
    FUNCTION_TEST_END();

    void *result = NULL;

    MEM_CONTEXT_OBJ_BEGIN(this)
    {
        // If data is >= block size then allocate a special block for the data
        if (size >= BLOB_BLOCK_SIZE)
        {
            result = memNew(size);
            memcpy(result, data, size);
        }
        else
        {
            // If data will fit in the current block
            if (BLOB_BLOCK_SIZE - this->pos >= size)
            {
                result = (uint8_t *)this->block + this->pos;

                memcpy(result, data, size);
                this->pos += size;
            }
            // Else allocate a new block. The previous block is not freed — its pointers are still valid and the mem context
            // owns it. This is the source of the "no individual free" property that callers rely on.
            else
            {
                this->block = memNew(BLOB_BLOCK_SIZE);
                result = this->block;

                memcpy(result, data, size);
                this->pos = size;
            }
        }
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN_P(VOID, result);
}
