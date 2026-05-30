#ifndef LOGGING_H
#define LOGGING_H

enum log_level
{
   LOG_OPCODE,
   LOG_DEBUG,
   LOG_INFO,
   LOG_WARN,
   LOG_ERROR
};

void yagb_log(enum  log_level level,
              const char     *file,
              int             line,
              const char     *fmt,
              ...);

#define LOG_OPCODE(...) (yagb_log(LOG_OPCODE, __FILE__, __LINE__, __VA_ARGS__))
#define LOG_DEBUG(...) (yagb_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__))
#define LOG_INFO(...) (yagb_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__))
#define LOG_WARN(...) (yagb_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__))
#define LOG_ERROR(...) (yagb_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__))

#endif
