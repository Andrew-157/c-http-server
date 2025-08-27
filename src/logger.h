#ifndef LOGGER_DOT_H
#define LOGGER_DOT_H

#define LOGGER_SETUP_ENV "LoggerWasSet"
#define LOGGER_DEBUG_ENABLED "LoggerDebugEnabled"

static char *LEVELS[4] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR"
};

void setupLogger(int);
void log_message(char *, char *);

#endif
