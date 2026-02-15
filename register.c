#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *url;
    void (*callback)(void);
} handler;

static handler *handlers;
static int size = 0;

void register_handler(char *url, void (*callback)(void)) {
    handlers = realloc(handlers, sizeof(handler) * (size + 1));

    handler h = {
        .url = malloc(sizeof(url)),
        .callback = callback,
    };

    strcpy(h.url, url);
    handlers[size] = h;
    size += 1;
}

void mainpage(void) {
    printf("Handling mainpage request\n");
}

void users(void) {
    printf("Users page request\n");
}

void _404_handler(void) {
    printf("This page doesn't exist\n");
}

typedef void (*callback_type)(void);
callback_type handle_request(char *url) {
    for (int i = 0; i < size; i++) {
        if (strcmp(url, handlers[i].url) == 0) {
            return handlers[i].callback;
        }
    }
    return _404_handler;
}

void freehandlers() {
    for (int i = 0; i < size; i++) {
        free(handlers[i].url);
    }
    free(handlers);
}

int main() {
    register_handler("/homepage", mainpage);
    register_handler("/users", users);

    char *urls[] = {
        "/homepage",
        "/random",
        "/users",
    };
    for (int i = 0; i < 3; i++) {
        handle_request(urls[i])();
    }

    freehandlers();
}
