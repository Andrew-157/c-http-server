#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct request {
    char *method;
};

struct response {
    int status_code;
    char *content_type;
    char *body;
};

struct url_callback {
    char *url;
    struct response (*callback)(struct request);
};

struct url_callback *ucls;

struct response default_404_handler(struct request req) {
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 404;
    resp.body = "<h1>Page not found</h1>";
    return resp;
}

struct state {
    struct response (*_404_handler)(struct request);
};

struct state global_state = {
    ._404_handler = default_404_handler,
};

int handlers_count = 0;

void register_handler(char *url, struct response (*callback)(struct request)) {
    ucls = realloc(ucls, sizeof(struct url_callback) * (handlers_count + 1));

    struct url_callback ucl = {
        //.url = malloc(sizeof(url)),
        .url = url,
        .callback = callback,
    };

    //strcpy(ucl.url, url);
    ucls[handlers_count++] = ucl;
}

struct response _index_handler(struct request req) {
    struct response resp;
    resp.content_type = "text/html";
    if (strcmp(req.method, "GET") != 0) {
        resp.status_code = 405;
        resp.body = "<h1>Method is not allowed</h1>";
    } else {
        resp.status_code = 200;
        resp.body = "<h1>C HTTP Server</h1>";
    }
    return resp;
}

typedef struct response (*callback_type)(struct request);
callback_type handle_request(char *url) {
    for (int i = 0; i < handlers_count; i++) {
        if (strcmp(url, ucls[i].url) == 0) {
            return ucls[i].callback;
        }
    }
    if (strcmp(url, "/") == 0) {
        return _index_handler;
    }
    return global_state._404_handler;
}

void register_404_handler(callback_type handler) {
    global_state._404_handler = handler;
}

struct response users_handler(struct request req) {
    (void)req.method;
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 200;
    resp.body = "<h1>USERS LIST</h1>";
    return resp;
}

struct response new_404_handler(struct request req) {
    (void)req.method;
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 404;
    resp.body = "<h1>non default 404 request handler</h1>";
    return resp;
}

void freehandlers() {
    //for (int i = 0; i < handlers_count; i++) {
    //    free(ucls[i].url);
    //}
    free(ucls);
}

int main() {
    register_handler("/users", users_handler);
    register_404_handler(new_404_handler);
    struct request req = {
        .method = "GET",
    };

    char *urls[] = {
        "/users",
        "/",
        "/idk",
    };

    for (int i = 0; i < 3; i++) {
        printf("%s\n", handle_request(urls[i])(req).body);
    }

    freehandlers();

}
