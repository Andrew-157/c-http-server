#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

// TODO: each level should also be in a variable
char *LEVELS[4] = {
    INFO,
    DEBUG,
    WARNING,
    ERROR
};

/*
 * Set up logger - idk, i need to learn how to write docstrings
 * */
void setupLogger(int enable_debug) {
    if (enable_debug)
        setenv(LOGGER_DEBUG_ENABLED, "1", 1);
    else
        setenv(LOGGER_DEBUG_ENABLED, "0", 1);
    setenv(LOGGER_SETUP_ENV, "1", 1); // value doesn't really matter here
}

/*
 * log a message
 * */
// TODO: extend accepted formatting options
void log_message(char *level, char *msg, ...) {
    if (getenv(LOGGER_SETUP_ENV) == NULL) {
        fprintf(stderr, "setupLogger function wasn't called beforehand\n");
        exit(1);
    }
    int level_valid = 0;
    for (int i = 0; i < 4; i++) {
        if (strcmp(level, LEVELS[i]) == 0) {
            level_valid = 1;
            break;
        }
    }
    if (!level_valid) {
        fprintf(stderr, "Invalid severity level \"%s\" for logger. Valid levels are: [ ", level);
        for (int i = 0; i < 4; i++) {
            if (i == 3)
                fprintf(stderr, "%s ].\n", LEVELS[i]);
            else
                fprintf(stderr, "%s, ", LEVELS[i]);
        }
        exit(1);
    }

    FILE *stream; // choose where to send message based on level - stdout or stderr
    if (strcmp(level, ERROR) == 0 || strcmp(level, WARNING) == 0)
        stream = stderr;
    else if (strcmp(level, DEBUG) == 0) {
        if (atoi(getenv(LOGGER_DEBUG_ENABLED)))
            stream = stdout;
        else
            return;
    } else
        stream = stdout;

    // TODO: print datetime before level
    fprintf(stream, "%s: ", level);
    // NOTE: took it from "The C Programming Language - 2nd Edition"(`minprintf` implementation),
    // it is fine for printing to console, but I don't think it will work well for sending to syslog, for example,
    // as I am printing here one character at a time, so it will have to be reworked, but for now it is okay
    va_list ap; // points to each unnamed arg in turn
    char *p, *sval;
    int ival;
    double dval;

    va_start(ap, msg); // make ap point to 1st unnamed arg
    for (p = msg; *p; p++) {
        if (*p != '%') {
            fprintf(stream, "%c", *p);
            continue;
        }
        switch (*++p) {
            case 'd':
                ival = va_arg(ap, int);
                fprintf(stream, "%d", ival);
                break;
            case 'f':
                dval = va_arg(ap, double);
                fprintf(stream, "%f", dval);
                break;
            case 's':
                for (sval = va_arg(ap, char *); *sval; sval++)
                    fprintf(stream, "%c", *sval);
                break;
            case 'c':
                ival = va_arg(ap, int);
                fprintf(stream, "%c", ival);
                break;
            default:
                fprintf(stream, "%c", *p);
                break;
        }
    }
    va_end(ap); // clean up when done
}
