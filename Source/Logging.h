#pragma once


#include <stdio.h>
#include <stdlib.h>

#include "Base.h"


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
        if (Logging##LEVEL < LoggingLevel) {                                           \
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


enum LoggingLevel LoggingLevel;
