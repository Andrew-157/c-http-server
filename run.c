#include <stdio.h>

#include "server.h"

#define PORT "8080"

struct response users_callback(struct request req) {
    (void)req;
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 200;
    resp.headers = NULL;
    resp.body = "<h1>LIST_OF_USERS</h1>";
    return resp;
}

int main() {
    register_uri_callback("/users", users_callback);
    serve(PORT);
    freecallbacks();
}
