#pragma once

#include <ctime>
#include <iostream>
#include <string.h>
#include <string>

//ANSI CONSOLE COLOR MACROS
#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

#define AR_RESET_COLOR RESET

#define AR_CORE_TRACE_COLOR GREEN
#define AR_CORE_INFO_COLOR
#define AR_CORE_WARN_COLOR BOLDYELLOW
#define AR_CORE_ERROR_COLOR RED
#define AR_CORE_FATAL_COLOR BOLDRED

#define AR_TRACE_COLOR GREEN
#define AR_INFO_COLOR
#define AR_WARN_COLOR BOLDYELLOW
#define AR_ERROR_COLOR RED
#define AR_FATAL_COLOR BOLDRED

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define AR_TIME_STRING                   \
    std::time_t rawtime;                 \
    struct std::tm* timeinfo;            \
    char buffer[80];                     \
    std::time(&rawtime);                 \
    timeinfo = std::localtime(&rawtime); \
    std::strftime(buffer, 80, "[%I:%M:%S %p]", timeinfo);

#ifdef AR_DEBUG
#define AR_LOG_ENABLED
#endif
#ifdef AR_RELEASE
#define AR_LOG_ENABLED
#endif

#ifdef AR_LOG_ENABLED

// Core (Artifice) logging
#define AR_CORE_TRACE(format, ...)                                                                                             \
    {                                                                                                                          \
        AR_TIME_STRING                                                                                                         \
        printf(AR_CORE_TRACE_COLOR "%s AR::TRACE (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_CORE_INFO(format, ...)                                                           \
    {                                                                                       \
        AR_TIME_STRING                                                                      \
        printf(AR_CORE_INFO_COLOR "%s AR::INFO " format RESET "\n", buffer, ##__VA_ARGS__); \
    }
#define AR_CORE_WARN(format, ...)                                                                                            \
    {                                                                                                                        \
        AR_TIME_STRING                                                                                                       \
        printf(AR_CORE_WARN_COLOR "%s AR::WARN (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_CORE_ERROR(format, ...)                                                                                             \
    {                                                                                                                          \
        AR_TIME_STRING                                                                                                         \
        printf(AR_CORE_ERROR_COLOR "%s AR::ERROR (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_CORE_FATAL(format, ...)                                                                                             \
    {                                                                                                                          \
        AR_TIME_STRING                                                                                                         \
        printf(AR_CORE_FATAL_COLOR "%s AR::FATAL (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
        throw std::runtime_error("fatal");                                                                                     \
    }

// Client (App) logging
#define AR_TRACE(format, ...)                                                                                             \
    {                                                                                                                     \
        AR_TIME_STRING                                                                                                    \
        printf(AR_TRACE_COLOR "%s AR::TRACE (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_INFO(format, ...)                                                            \
    {                                                                                   \
        AR_TIME_STRING                                                                  \
        printf(AR_INFO_COLOR "%s APP::INFO " format RESET "\n", buffer, ##__VA_ARGS__); \
    }
#define AR_WARN(format, ...)                                                                                             \
    {                                                                                                                    \
        AR_TIME_STRING                                                                                                   \
        printf(AR_WARN_COLOR "%s APP::WARN (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_ERROR(format, ...)                                                                                              \
    {                                                                                                                      \
        AR_TIME_STRING                                                                                                     \
        printf(AR_ERROR_COLOR "%s APP::ERROR (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
    }
#define AR_FATAL(format, ...)                                                                                              \
    {                                                                                                                      \
        AR_TIME_STRING                                                                                                     \
        printf(AR_FATAL_COLOR "%s APP::FATAL (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
        throw std::runtime_error("fatal");                                                                                 \
    }

#define AR_VK_VALIDATION(format, ...)                                                       \
    {                                                                                       \
        AR_TIME_STRING                                                                      \
        printf(BOLDBLUE "%s AR::VK::VALIDATION " format RESET "\n", buffer, ##__VA_ARGS__); \
    }

#define VK_CALL(result)                           \
    if (result)                                   \
    {                                             \
        std::cout << result << std::endl;         \
        throw std::runtime_error("AR::VK_FATAL"); \
    }

#define AR_CORE_ASSERT(condition, format, ...)                     \
    if (!(condition))                                              \
    {                                                              \
        AR_CORE_FATAL("Assertion failed! " format, ##__VA_ARGS__); \
    }
#define AR_ASSERT(condition, format, ...)                     \
    if (!(condition))                                         \
    {                                                         \
        AR_ERROR("Assertion failed! " format, ##__VA_ARGS__); \
    }

#else

#define AR_CORE_TRACE(format, ...)
#define AR_CORE_INFO(format, ...)
#define AR_CORE_WARN(format, ...)
#define AR_CORE_ERROR(format, ...)
#define AR_CORE_FATAL(format, ...)                                                                                             \
    {                                                                                                                          \
        AR_TIME_STRING                                                                                                         \
        printf(AR_CORE_FATAL_COLOR "%s AR::FATAL (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
        throw std::runtime_error("fatal");                                                                                     \
    }
// Client (App) logging
#define AR_TRACE(format, ...)
#define AR_INFO(format, ...)
#define AR_WARN(format, ...)
#define AR_ERROR(format, ...)
#define AR_FATAL(format, ...)                                                                                              \
    {                                                                                                                      \
        AR_TIME_STRING                                                                                                     \
        printf(AR_FATAL_COLOR "%s APP::FATAL (%s=>%s) " format RESET "\n", buffer, __FILENAME__, __func__, ##__VA_ARGS__); \
        throw std::runtime_error("fatal");                                                                                 \
    }
#define AR_VK_VALIDATION(format, ...)

#define VK_CALL(result) result

#define AR_CORE_ASSERT(condition, format, ...)
#define AR_ASSERT(condition, format, ...)

#endif