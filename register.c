#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *url;
	char *callback;
} handler;

handler *handlers;
int size = 0;

void register_handler(char *, char *);

int main() {
	register_handler("/homepage", "Homepage handler");

	for (int i = 0; i < size; i++) {
		printf("Url %s will be handled by %s\n", handlers[i].url, handlers[i].callback);
	}

	for (int i = 0; i < size; i++) {
		free(handlers[i].url);
		free(handlers[i].callback);
	}
	free(handlers);
}

void register_handler(char *url, char *callback) {
	handlers = realloc(handlers, sizeof(handler) * (size + 1));

	handler h = {
		.url = malloc(sizeof(url)),
		.callback = malloc(sizeof(callback)),
	};
	strcpy(h.url, url);
	strcpy(h.callback, callback);

	handlers[size] = h;
	size += 1;
}
