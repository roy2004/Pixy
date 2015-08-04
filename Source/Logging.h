#pragma once


#include <stdio.h>
#include <stdlib.h>

#include "Base.h"
#include "Atomic.h"


#define LOG_INFORMATION(FORMAT, ...) \
    __LOG(Information, FORMAT, ##__VA_ARGS__)

#define LOG_WARNING(FORMAT, ...) \
    __LOG(Warning, FORMAT, ##__VA_ARGS__)

#define LOG_ERROR(FORMAT, ...) \
    __LOG(Error, FORMAT, ##__VA_ARGS__)

#define LOG_FATAL_ERROR(FORMAT, ...) \
    __LOG(FatalError, FORMAT, ##__VA_ARGS__); \
    abort()

#define __LOG(LEVEL, FORMAT, ...)                                                      \
    do {                                                                               \
        if (Logging##LEVEL < GetLoggingLevel()) {                                      \
            break;                                                                     \
        }                                                                              \
                                                                                       \
        fprintf(stderr, #LEVEL ": " __FILE__ "(" STRINGIZE(__LINE__) "): " FORMAT "\n" \
                , ##__VA_ARGS__);                                                      \
    } while (false)


enum LoggingLevel
{
    LoggingInformation,
    LoggingWarning,
    LoggingError,
    LoggingFatalError
};


static inline void SetLoggingLevel(enum LoggingLevel);
static inline enum LoggingLevel GetLoggingLevel(void);


enum LoggingLevel __LoggingLevel;


static inline void
SetLoggingLevel(enum LoggingLevel loggingLevel)
{
    ATOMIC_EXCHANGE(__LoggingLevel, loggingLevel);
}


static inline enum LoggingLevel
GetLoggingLevel(void)
{
    enum LoggingLevel loggingLevel = 0;
    ATOMIC_ADD(__LoggingLevel, loggingLevel);
    return loggingLevel;
}
