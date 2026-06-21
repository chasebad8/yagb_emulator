#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <string.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

#define LOG_LEVEL_OPCODE 0
#define LOG_LEVEL_DEBUG  1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_WARN   3
#define LOG_LEVEL_ERROR  4

/* ai made this, I couldn't be bothered */
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

void yagb_log(uint8_t     level,
              const char *file,
              int         line,
              const char *fmt,
              ...);

#if (defined LOG_OPCODES) || (LOG_LEVEL <= LOG_LEVEL_OPCODE)
#define LOG_OPCODE(...) (yagb_log(LOG_LEVEL_OPCODE, __FILENAME__, __LINE__, __VA_ARGS__))
#else
#define LOG_OPCODE(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) (yagb_log(LOG_LEVEL_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__))
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(...)  (yagb_log(LOG_LEVEL_INFO, __FILENAME__, __LINE__, __VA_ARGS__))
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(...)  (yagb_log(LOG_LEVEL_WARN, __FILENAME__, __LINE__, __VA_ARGS__))
#else
#define LOG_WARN(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(...) (yagb_log(LOG_LEVEL_ERROR, __FILENAME__, __LINE__, __VA_ARGS__))
#else
#define LOG_ERROR(...) ((void)0)
#endif

#endif
