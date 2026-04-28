/***********************************************************************************************************************************
Log Levels

Split out from log.h so that headers which only need the LogLevel type (notably stackTrace.h and debug.h) don't pull in the full
logging interface. Order matters: numeric increase = more verbose, and several sites do direct numeric comparison
(e.g. "logLevel >= logLevelDebug" in log.c).
***********************************************************************************************************************************/
#ifndef COMMON_LOGLEVEL_H
#define COMMON_LOGLEVEL_H

/***********************************************************************************************************************************
Log types
***********************************************************************************************************************************/
typedef enum
{
    logLevelOff,
    logLevelAssert,
    logLevelError,
    logLevelWarn,
    logLevelInfo,
    logLevelDetail,
    logLevelDebug,
    logLevelTrace,
} LogLevel;

#define LOG_LEVEL_MIN                                               logLevelAssert
#define LOG_LEVEL_MAX                                               logLevelTrace

#endif
