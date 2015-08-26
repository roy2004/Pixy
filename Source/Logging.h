/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdio.h>
#include <stdlib.h>

#include "Utility.h"
#include "Atomic.h"


#ifdef NDEBUG
#define LOG_DEBUG(format, ...) \
    (void)0
#else
#define LOG_DEBUG(format, ...) \
    __LOG(Debug, format, ##__VA_ARGS__)
#endif

#define LOG_INFORMATION(format, ...) \
    __LOG(Information, format, ##__VA_ARGS__)

#define LOG_WARNING(format, ...) \
    __LOG(Warning, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    __LOG(Error, format, ##__VA_ARGS__)

#define LOG_FATAL_ERROR(format, ...)          \
    __LOG(FatalError, format, ##__VA_ARGS__); \
    abort()

#define __LOG(level, format, ...)                                                               \
    do {                                                                                        \
        if (Logging##level < GetLoggingLevel()) {                                               \
            break;                                                                              \
        }                                                                                       \
                                                                                                \
        fprintf(stderr, "(Pixy) " #level ": " __FILE__ ":" STRINGIZE(__LINE__) ": " format "\n" \
                , ##__VA_ARGS__);                                                               \
    } while (0)


enum LoggingLevel
{
    LoggingDebug,
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
