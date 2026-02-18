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
};

#endif
