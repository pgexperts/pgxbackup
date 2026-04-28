/***********************************************************************************************************************************
Object Helper Macros and Functions

Out-of-line implementations for the OBJ_* macros and the objMove/objFree primitives declared in object.h. These functions accept NULL pointers
silently to make freeing partially-constructed or already-freed objects a no-op, simplifying error-path cleanup.
***********************************************************************************************************************************/
#include <build.h>

#include "common/type/object.h"

/**********************************************************************************************************************************/
FN_EXTERN void *
objMove(THIS_VOID, MemContext *const parentNew)
{
    if (thisVoid != NULL)
        memContextMove(memContextFromAllocExtra(thisVoid), parentNew);

    return thisVoid;
}

/**********************************************************************************************************************************/
FN_EXTERN void *
objMoveToInterface(THIS_VOID, void *const interfaceVoid, const MemContext *const current)
{
    return objMemContext(thisVoid) != current ? objMove(thisVoid, objMemContext(interfaceVoid)) : thisVoid;
}

/**********************************************************************************************************************************/
FN_EXTERN void
objFree(THIS_VOID)
{
    if (thisVoid != NULL)
        memContextFree(memContextFromAllocExtra(thisVoid));
}
