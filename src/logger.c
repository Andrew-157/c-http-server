#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

/*
 * Set up logger - idk, i need to learn how to write docstrings
 * */
void setupLogger(int enable_debug) {
    printf("Enabled debug: %d\n", enable_debug);
    if (enable_debug) {
        printf("Enabling debug\n");
        setenv(LOGGER_DEBUG_ENABLED, "1", 1);
    } else {
        printf("Disabling debug\n");
        setenv(LOGGER_DEBUG_ENABLED, "0", 1);
    }
    setenv(LOGGER_SETUP_ENV, "1", 1); // value doesn't really matter here
}

/*
 * log a message
 * */
void log_message(char *level, char *msg) {
    char *env = getenv(LOGGER_SETUP_ENV);
    if (env == NULL) {
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
        fprintf(stderr, "Invalid severity level \"%s\" for logger\n", level);
        exit(1);
    }
    if (strcmp(level, "ERROR") == 0 || strcmp(level, "WARNING") == 0){
        fprintf(stderr, "%s: %s", level, msg);
    } else if (strcmp(level, "DEBUG") == 0) {
        printf("Debug is enabled: %d\n", atoi(getenv(LOGGER_DEBUG_ENABLED)));
        if (atoi(getenv(LOGGER_DEBUG_ENABLED)))
            printf("%s: %s", level, msg);
    } else {
        printf("%s: %s", level, msg);
    }
}

