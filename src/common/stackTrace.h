/***********************************************************************************************************************************
Stack Trace Handler

Maintains an explicit per-process call stack pushed and popped by the FUNCTION_LOG_* and FUNCTION_TEST_* macros (via STACK_TRACE_PUSH
and STACK_TRACE_POP) so that error messages can include parameter values and a useful trace even on platforms or build configs where
libbacktrace is unavailable. When libbacktrace is present (HAVE_LIBBACKTRACE), stackTraceToZ merges the runtime backtrace with our
synthetic stack to combine real source lines with logged parameter values. Pushes are also tagged with errorTryDepth() so
stackTraceClean can unwind frames added inside a TRY block when an exception is caught.
***********************************************************************************************************************************/
#ifndef COMMON_STACKTRACE_H
#define COMMON_STACKTRACE_H

#include <stdbool.h>
#include <sys/types.h>

#include "common/logLevel.h"

/***********************************************************************************************************************************
Maximum size of a single parameter (including NULL terminator)
***********************************************************************************************************************************/
#define STACK_TRACE_PARAM_MAX                                       4096

/***********************************************************************************************************************************
Macros to access internal functions
***********************************************************************************************************************************/
#define STACK_TRACE_PUSH(logLevel)                                                                                                 \
    stackTracePush(__FILE__, __func__, logLevel)

#ifdef DEBUG
#define STACK_TRACE_POP(test)                                                                                                      \
    stackTracePop(__FILE__, __func__, test);
#else
#define STACK_TRACE_POP(test)                                                                                                      \
    stackTracePop();
#endif

/***********************************************************************************************************************************
Internal Functions
***********************************************************************************************************************************/
#ifdef DEBUG

// Enable/disable test function logging
FN_EXTERN void stackTraceTestStart(void);
FN_EXTERN void stackTraceTestStop(void);
FN_EXTERN bool stackTraceTest(void);

// Set line number for the current function on the stack
FN_EXTERN void stackTraceTestFileLineSet(unsigned int fileLine);

#else
// Must always be valid since it might be needed to compile (though not used) during profiling
#define stackTraceTestFileLineSet(fileLine)
#endif

// Push a new function onto the trace stack
FN_EXTERN LogLevel stackTracePush(const char *fileName, const char *functionName, LogLevel functionLogLevel);

// Pop a function from the trace stack
#ifdef DEBUG
FN_EXTERN void stackTracePop(const char *fileName, const char *functionName, bool test);
#else
FN_EXTERN void stackTracePop(void);
#endif

// Generate the stack trace
FN_EXTERN size_t stackTraceToZ(
    char *buffer, size_t bufferSize, const char *fileName, const char *functionName, unsigned int fileLine);

// Mark that parameters are being logged -- if none appear then the function is void
FN_EXTERN void stackTraceParamLog(void);

// Get parameters for the top function on the stack
FN_EXTERN const char *stackTraceParam(void);

// Get the next location where a parameter can be added in the param buffer
FN_EXTERN char *stackTraceParamBuffer(const char *param);

// Add a parameter to the function on the top of the stack
FN_EXTERN void stackTraceParamAdd(size_t bufferSize);

// Clean the stack at and below the try level. Called by the error to cleanup the stack when an exception occurs.
FN_EXTERN void stackTraceClean(unsigned int tryDepth, bool fatal);

#endif
