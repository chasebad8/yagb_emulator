#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "logging.h"

/* ANSI color codes for terminal output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

static const char* log_level_to_string(enum log_level level)
{
   switch(level)
   {
      case LOG_DEBUG: return "DEBUG";
      case LOG_INFO:  return "INFO";
      case LOG_WARN:  return "WARN";
      case LOG_ERROR: return "ERROR";
      default:        return "UNKNOWN";
   }
}

static const char* log_level_to_color(enum log_level level)
{
   switch(level)
   {
      case LOG_DEBUG: return COLOR_BLUE;
      case LOG_INFO:  return COLOR_GREEN;
      case LOG_WARN:  return COLOR_YELLOW;
      case LOG_ERROR: return COLOR_RED;
      default:        return COLOR_RESET;
   }
}

void yagb_log(enum  log_level level,
              const char     *file,
              int             line,
              const char     *fmt,
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

   fprintf(stdout, "%s[%s]%s ", color, log_level_to_string(level), COLOR_RESET);

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