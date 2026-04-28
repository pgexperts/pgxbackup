/***********************************************************************************************************************************
Compression Common

Builds the Pack-encoded parameter lists used to serialize compress/decompress filter constructor arguments across the protocol
boundary. The pack layout (compress: int32 level then bool raw; decompress: bool raw) is a wire format -- helper.c's
compressFilterPack() reads it back -- so the field count and order must stay in sync with both ends.
***********************************************************************************************************************************/
#include <build.h>

#include "common/compress/common.h"
#include "common/debug.h"

/**********************************************************************************************************************************/
FN_EXTERN Pack *
compressParamList(const int level, const bool raw)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_LOG_PARAM(INT, level);
        FUNCTION_TEST_PARAM(BOOL, raw);
    FUNCTION_TEST_END();

    Pack *result;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        PackWrite *const packWrite = pckWriteNewP();

        pckWriteI32P(packWrite, level);
        pckWriteBoolP(packWrite, raw);
        pckWriteEndP(packWrite);

        result = pckMove(pckWriteResult(packWrite), memContextPrior());
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PACK, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Pack *
decompressParamList(const bool raw)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(BOOL, raw);
    FUNCTION_TEST_END();

    Pack *result;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        PackWrite *const packWrite = pckWriteNewP();

        pckWriteBoolP(packWrite, raw);
        pckWriteEndP(packWrite);

        result = pckMove(pckWriteResult(packWrite), memContextPrior());
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PACK, result);
}
