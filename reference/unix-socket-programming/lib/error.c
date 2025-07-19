#include "unp.h"

#include <stdarg.h>  /* ANSI C header file */
#include <syslog.h>  /* for syslog(), contains LOG_INFO, LOG_ERR, etc. */

int daemon_proc;     /* set nonzero by daemon_init() */

static void err_doit(int, int, const char *, va_list);

/*
 * Nonfatal error related to system call
 * Print message and return
 * */
void err_ret(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/*
 * Fatal error related to system call
 * Print message and terminate
 * */
void err_sys(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/*
 * Fatal error related to system call
 * Print message, dump core, and terminate
 * */
void err_dump(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort();    /* dump core and terminate */
    exit(1);    /* shouldn't get here */
}
