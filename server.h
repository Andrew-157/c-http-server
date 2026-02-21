#ifndef SERVER_DOT_H
#define SERVER_DOT_H

int serve(const char *);

struct request {
    char *method;
};

struct response {
    int status_code;
    char *content_type;
    char *body;
    char *headers;
};

typedef struct response (*uri_callback)(struct request);
void register_uri_callback(char *, uri_callback);
void freecallbacks();

#endif
