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

struct printBuf {
    char *b;
    int len;
};

static void printBufAppend(struct printBuf *pb, const char *s, int len) {
    char *new = realloc(pb->b, pb->len + len);

    if (new == NULL) return;
    memcpy(&new[pb->len], s, len);
    pb->b = new;
    pb->len += len;
}

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
// TODO: handle sending messages to syslog
// TODO: handle sending messages to remote syslog
// TODO: if server in the end will be handling multiple connections simultaneously,
// maybe it is worth to specify in logged messages, to which connection a particular message
// relates?
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

    va_list ap; // points to each unnamed arg in turn
    char *p, *sval;
    int ival;
    double dval;
    struct printBuf pb = {NULL, 0};
    char buf[64];

    va_start(ap, msg); // make ap point to 1st unnamed arg
    for (p = msg; *p; p++) {
        if (*p != '%') {
            snprintf(buf, sizeof(buf), "%c", *p);
            printBufAppend(&pb, buf, strlen(buf));
            continue;
        }
        switch (*++p) {
            case 'd':
                ival = va_arg(ap, int);
                snprintf(buf, sizeof(buf), "%d", ival);
                printBufAppend(&pb, buf, strlen(buf));
                break;
            case 'f':
                dval = va_arg(ap, double);
                snprintf(buf, sizeof(buf), "%f", dval);
                printBufAppend(&pb, buf, strlen(buf));
                break;
            case 's':
                for (sval = va_arg(ap, char *); *sval; sval++) {
                    snprintf(buf, sizeof(buf), "%c", *sval);
                    printBufAppend(&pb, buf, strlen(buf));
                }
                break;
            case 'c':
                ival = va_arg(ap, int);
                snprintf(buf, sizeof(buf), "%c", ival);
                printBufAppend(&pb, buf, strlen(buf));
                break;
            default:
                snprintf(buf, sizeof(buf), "%c", *p);
                printBufAppend(&pb, buf, strlen(buf));
                break;
        }
    }
    va_end(ap); // clean up when done

    // TODO: print datetime before level
    fprintf(stream, "%s: %s", level, pb.b);
    free(pb.b);
}
