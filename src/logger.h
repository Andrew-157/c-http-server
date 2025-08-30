#ifndef LOGGER_DOT_H
#define LOGGER_DOT_H

#define LOGGER_SETUP_ENV     "LoggerWasSet"
#define LOGGER_DEBUG_ENABLED "LoggerDebugEnabled"

// levels
#define INFO    "INFO"
#define DEBUG   "DEBUG"
#define WARNING "WARNING"
#define ERROR   "ERROR"

extern char *LEVELS[4];

void setupLogger(int);
void log_message(char *, char *, ...);

#endif
