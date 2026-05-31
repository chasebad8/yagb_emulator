#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "logging.h"

/* ANSI color codes for terminal output */
#define COLOUR_RESET   "\033[0m"
#define COLOUR_RED     "\033[31m"
#define COLOUR_GREEN   "\033[32m"
#define COLOUR_YELLOW  "\033[33m"
#define COLOUR_BLUE    "\033[34m"
#define COLOUR_PURPLE  "\033[35m"

static const char* log_level_to_string(uint8_t level)
{
   switch(level)
   {
      case LOG_LEVEL_OPCODE: return "OPCODE";
      case LOG_LEVEL_DEBUG:  return "DEBUG";
      case LOG_LEVEL_INFO:   return "INFO";
      case LOG_LEVEL_WARN:   return "WARN";
      case LOG_LEVEL_ERROR:  return "ERROR";
      default:         return "UNKNOWN";
   }
}

static const char* log_level_to_color(uint8_t level)
{
   switch(level)
   {
      case LOG_LEVEL_OPCODE: return COLOUR_PURPLE;
      case LOG_LEVEL_DEBUG:  return COLOUR_BLUE;
      case LOG_LEVEL_INFO:   return COLOUR_GREEN;
      case LOG_LEVEL_WARN:   return COLOUR_YELLOW;
      case LOG_LEVEL_ERROR:  return COLOUR_RED;
      default:         return COLOUR_RESET;
   }
}

void yagb_log(uint8_t     level,
              const char *file,
              int         line,
              const char *fmt,
              ...)
{
   /* get color for this log level */
   const char *color = log_level_to_color(level);

#ifdef LOGTIME
   /* get the current time */
   time_t now = time(NULL);
   struct tm *local_time = localtime(&now);

   if (local_time != NULL)
   {
      fprintf(stdout, "[%02d:%02d:%02d]",
             local_time->tm_hour,
             local_time->tm_min,
             local_time->tm_sec);
   }
#endif

#ifdef LOGFILE
   fprintf(stdout, "[%s:%d]", file, line);
#endif

   fprintf(stdout, "%s[%s]%s ", color, log_level_to_string(level), COLOUR_RESET);

   /* create a variadic list struct */
   va_list ptr_arg;
   /* call va_start passing the last known arg */
   va_start(ptr_arg, fmt);
   /* essentially printf but with va_list arg instead of direct multi length arg */
   vfprintf(stdout, fmt, ptr_arg);
   /* cleanup */
   va_end(ptr_arg);

   fprintf(stdout, "\n");
}