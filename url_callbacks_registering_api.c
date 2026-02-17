#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* TODO: add a function for users which will allow users to call it like that:
 * `get_template("index.html")`, where index.html is a file inside templates/ directory
 * of the project, the returned value could then be passed to response.body field, so user doesn't need to bother with reading the file manually
 * Though the default handlers of the library, aka index_handler and _404_handler must not read from no file, but html should be hardcoded in the program(or dynamically generated), so there are no dependencies for the program in the form of HTML templates, lol
 * */

struct request {  // exposed to user
    char *method;
};

struct response {  // exposed to user
    int status_code;
    char *content_type;
    char *body;
};

typedef struct response (*url_callback)(struct request);

struct state {
    url_callback index_callback;
    url_callback _404_callback;
};

static struct response default_index_callback(struct request req) {
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

static struct response default_404_callback(struct request req) {
    (void)req;  // we don't really care what is in request
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 404;
    resp.body = "<h1>Page Not Found</h1>";
    return resp;
}

static struct state global = {
    .index_callback = default_index_callback,
    ._404_callback = default_404_callback,  // TODO: allow setting custom 404 handler
};

// if it is not static, can user of the library access it?
struct url_callback_map {
    char *url;
    url_callback callback;
};

static struct url_callback_map *ucls;
static int callbacks_count = 0;

static url_callback handle_request(char *url) {
    if (strcmp(url, "/") == 0) {
        return global.index_callback;
    }
    for (int i = 0; i < callbacks_count; i++) {
        if (strcmp(ucls[i].url, url) == 0) {
            return ucls[i].callback;
        }
    }
    return global._404_callback;
}

void register_url_callback(char *url, url_callback callback) {  // exposed to user
    // handle duplicated urls
    if (strcmp(url, "/") == 0) {
        global.index_callback = callback;
    } else {
        ucls = realloc(ucls, sizeof(struct url_callback_map) * (callbacks_count + 1));
        struct url_callback_map ucl = {
            .url = url,
            .callback = callback,
        };
        ucls[callbacks_count++] = ucl;
    }
}

void freecallbacks() {  // exposed to user
    if (callbacks_count > 0) {
        free(ucls);
    }
}

// dummy function for demonstration only
struct response users_url_callback(struct request req) {
    (void)req.method;
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 200;
    resp.body = "<h1>Users list</h1>";
    return resp;
}

// dummy function for demonstration only
struct response homepage_url_callback(struct request req) {
    (void)req.method;
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 200;
    resp.body = "<h1>This is the homepage</h1>";
    return resp;
}

struct response index_callback(struct request req) {
    struct response resp;
    resp.content_type = "text/html";
    if (strcmp(req.method, "GET") != 0) {
        resp.status_code = 405;
        resp.body = "<h1>Method is not allowed</h1>";
    } else {
        resp.status_code = 200;
        resp.body = "<h1>Welcome to the main page</h1>";
    }
    return resp;
}


int main(void) {
    //register_url_callback("/", index_callback);
    register_url_callback("/users", users_url_callback);
    register_url_callback("/homepage", homepage_url_callback);
    struct request req = {
        .method = "GET",
    };

    char *urls[] = {
        "/",
        "/users",
        "/homepage",
        "/page-not-found",
    };

    for (int i = 0; i < 4; i++) {
        printf("%s\n", handle_request(urls[i])(req).body);
    }

    freecallbacks();
}
