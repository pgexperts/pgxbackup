/***********************************************************************************************************************************
GCS Storage Helper

Translates pgBackRest configuration options into a storageGcsNew() call. The repo-gcs-key option holds either a path to a
service-account JSON file (key-type=service) or a static OAuth token (key-type=token). The `auto` key-type leaves the key NULL
since GCE metadata-server tokens are fetched on demand inside storage.c.
***********************************************************************************************************************************/
#include <build.h>

#include "common/debug.h"
#include "common/io/io.h"
#include "common/log.h"
#include "config/config.h"
#include "storage/gcs/helper.h"
#include "storage/helper.h"

/**********************************************************************************************************************************/
FN_EXTERN Storage *
storageGcsHelper(const unsigned int repoIdx, const bool write, StoragePathExpressionCallback pathExpressionCallback)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(UINT, repoIdx);
        FUNCTION_LOG_PARAM(BOOL, write);
        FUNCTION_LOG_PARAM_P(VOID, pathExpressionCallback);
    FUNCTION_LOG_END();

    ASSERT(cfgOptionIdxStrId(cfgOptRepoType, repoIdx) == STORAGE_GCS_TYPE);

    Storage *const result = storageGcsNew(
        cfgOptionIdxStr(cfgOptRepoPath, repoIdx), write, storageRepoTargetTime(), pathExpressionCallback,
        cfgOptionIdxStr(cfgOptRepoGcsBucket, repoIdx), (StorageGcsKeyType)cfgOptionIdxSeq(cfgOptRepoGcsKeyType, repoIdx),
        cfgOptionIdxStrNull(cfgOptRepoGcsKey, repoIdx), (size_t)cfgOptionIdxUInt64(cfgOptRepoStorageUploadChunkSize, repoIdx),
        cfgOptionIdxKvNull(cfgOptRepoStorageTag, repoIdx), cfgOptionIdxStr(cfgOptRepoGcsEndpoint, repoIdx), ioTimeoutMs(),
        cfgOptionIdxBool(cfgOptRepoStorageVerifyTls, repoIdx), cfgOptionIdxStrNull(cfgOptRepoStorageCaFile, repoIdx),
        cfgOptionIdxStrNull(cfgOptRepoStorageCaPath, repoIdx), cfgOptionIdxStrNull(cfgOptRepoGcsUserProject, repoIdx));

    FUNCTION_LOG_RETURN(STORAGE, result);
}
