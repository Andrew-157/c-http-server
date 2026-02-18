#define _GNU_SOURCE
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "server.h"

#define BACKLOG 10
#define BUF_SIZE 1000

static struct response index_callback(struct request req) {
    (void)req;  // silence compiler warning for now
    struct response resp;
    resp.content_type = "text/html";
    resp.status_code = 200;
    char *html =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>C-Server | Active</title>\n"
        "    <style>\n"
        "        :root { --bg: #0d1117; --card: #161b22; --text: #c9d1d9; --accent: #238636; --border: #30363d; }\n"
        "        body { font-family: sans-serif; background-color: var(--bg); color: var(--text); \n"
        "               display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }\n"
        "        .container { background-color: var(--card); padding: 2rem; border-radius: 8px; \n"
        "                    border: 1px solid var(--border); text-align: center; max-width: 500px; }\n"
        "        .status-badge { display: inline-block; background: rgba(35, 134, 54, 0.2); color: var(--accent); \n"
        "                        padding: 4px 12px; border-radius: 20px; font-size: 0.8rem; margin-bottom: 1rem; \n"
        "                        border: 1px solid var(--accent); }\n"
        "        h1 { margin: 0 0 0.5rem 0; }\n"
        "        .stats { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 2rem 0; text-align: left; }\n"
        "        .stat-item { background: var(--bg); padding: 10px; border-radius: 4px; border: 1px solid var(--border); }\n"
        "        .stat-label { font-size: 0.7rem; color: #8b949e; text-transform: uppercase; }\n"
        "        button { background-color: var(--accent); color: white; border: none; padding: 10px 20px; \n"
        "                 border-radius: 6px; cursor: pointer; font-weight: 600; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"status-badge\">● SERVER ONLINE</div>\n"
        "        <h1>C HTTP Server</h1>\n"
        "        <p>Hand-crafted in C for maximum performance.</p>\n"
        "        <div class=\"stats\">\n"
        "            <div class=\"stat-item\"><span class=\"stat-label\">Client IP</span>"
        "                <span id=\"client-ip\" style=\"display:block; font-family:monospace;\">Loading...</span></div>\n"
        "            <div class=\"stat-item\"><span class=\"stat-label\">Local Time</span>"
        "                <span id=\"server-time\" style=\"display:block; font-family:monospace;\">--:--:--</span></div>\n"
        "        </div>\n"
        "        <button onclick=\"pingServer()\">Send Ping</button>\n"
        "        <div id=\"ping-res\" style=\"margin-top: 15px; font-family: monospace; font-size: 0.8rem;\"></div>\n"
        "    </div>\n"
        "    <script>\n"
        "        document.getElementById('client-ip').innerText = window.location.hostname;\n"
        "        setInterval(() => {\n"
        "            document.getElementById('server-time').innerText = new Date().toLocaleTimeString();\n"
        "        }, 1000);\n"
        "        function pingServer() {\n"
        "            const out = document.getElementById('ping-res');\n"
        "            out.innerText = 'Request sent...';\n"
        "            fetch('/ping').then(r => r.text()).then(d => { out.innerText = 'Reply: ' + d; });\n"
        "        }\n"
        "    </script>\n"
        "</body>\n"
        "</html>";
    resp.body = html;
    return resp;
}

static struct response favicon_callback(struct request req) {
    (void)req;  // silence compiler warning for now
    struct response resp;
    resp.status_code = 200;
    resp.content_type = "image/svg+xml";
    resp.body = 
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 64 64\">"
        "<rect width=\"64\" height=\"64\" fill=\"#0d1117\" rx=\"8\"/>"
        "<g fill=\"#238636\">"
        "<path d=\"M20 12h28v8H20zm-8 8h8v24h-8zm8 24h28v8H20z\"/>"
        "<rect x=\"48\" y=\"22\" width=\"6\" height=\"6\"/>"
        "<rect x=\"48\" y=\"36\" width=\"6\" height=\"6\"/>"
        "<rect x=\"32\" y=\"30\" width=\"14\" height=\"4\"/>"
        "<circle cx=\"46\" cy=\"32\" r=\"5\"/></g></svg>";
    return resp;
}

static struct response not_found_callback(struct request req) {
    (void)req;
    struct response resp;
    resp.status_code = 404;
    resp.content_type = "text/html";
    resp.body = "<h1>Page Not Found</h1>";
    return resp;
}


typedef struct response (*uri_callback)(struct request);
static struct {
    uri_callback index_callback;
    uri_callback favicon_callback;
    uri_callback not_found_callback;
} global = {
    .index_callback = index_callback,
    .favicon_callback = favicon_callback,
    .not_found_callback = not_found_callback,
};

static void serve_client(int send_sock) {
    printf("Accepting client request\n");
    int nread;
    char buf[BUF_SIZE];
    int i;
    int j;
    struct request req;
    char method[50];
    char uri[50];
    char protocol[50];

    // For now I assume that the request will be in correct format, so parsing is easier
    if ((nread = recv(send_sock, buf, sizeof buf, 0)) > 0) {
        buf[nread] = '\0';  // assuming for now that we were able to read whole request in one chunk
        char c;
        char *request_line = NULL;
        for (i = 0; i < nread; i++) {
            c = buf[i];
            if (c == '\r') {
                request_line = malloc(sizeof(char) * (i - 1));
                for (j = 0; j < i; j++) {
                    request_line[j] = buf[j];
                }
                break;
            }
        }

        request_line[j] = '\0';
        sscanf(request_line, "%s %s %s", method, uri, protocol);
        free(request_line);

        req.method = method;

        struct response resp;
        if (strcmp(uri, "/") == 0) {
            resp = global.index_callback(req);
        } else if (strcmp(uri, "/favicon.ico") == 0) {
            resp = global.favicon_callback(req);
        }

        char response[3000];
        snprintf(response, 3000,
                "HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
                resp.status_code, resp.content_type, strlen(resp.body), resp.body);

        //char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<h1>C HTTP Server</h1>";
        //printf("[server]: Sending response\n");
        send(send_sock, response, sizeof(response), 0);
    } else if (nread == 0) {
        fprintf(stderr, "Client closed the connection\n");
    } else {
        perror("recv");
    }
}

static volatile sig_atomic_t terminate = 0;
static void signal_handler(int sig) {
    printf("Received signal %d\n", sig);  // Doesn't it spam since child processes constantly exit?
    if (sig == SIGCHLD) {
        // reap child processes
        while (waitpid(-1, NULL, WNOHANG) > 0);
    } else {
        terminate = 1;
    }
}

int serve(const char *port) {
    struct addrinfo hints, *result, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int gai;
    if ((gai = getaddrinfo(NULL, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return 1;
    }

    int listener;
    int yes = 1;
    for (p = result; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(listener);
            perror("setsockopt");
            continue;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("bind");
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (p == NULL) {
        fprintf(stderr, "failed to bind\n");
        return 1;
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    printf("Accepting connections...\n");

    int fd_size = 5;
    int fd_count = 1;  // 1 for listener from the start
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    struct sigaction act = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&act.sa_mask);
    int signals[3] = {
        SIGINT,
        SIGTERM,
        SIGCHLD,
    };
    for (int i = 0; i < 3; i++) {
        int sig = signals[i];
        if (sigaction(sig, &act, NULL) == -1) {
            close(listener);
            perror("sigaction");
            return 1;
        }
    }

    int return_code = 0;
    while (!terminate) {
        sigset_t sigmask;
        sigemptyset(&sigmask);
        sigaddset(&sigmask, SIGCHLD);
        int poll_count = ppoll(pfds, fd_count, NULL, &sigmask);

        if (poll_count == -1) {
            perror("ppoll");
            return_code = 1;
            break;
        }

        for (int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == listener) {
                    struct sockaddr_storage peer_addr;
                    socklen_t peer_addrlen = sizeof peer_addr;
                    int client;
                    if ((client = accept(listener, (struct sockaddr *)&peer_addr, &peer_addrlen)) == -1) {
                        perror("accept");
                        continue;
                    }

                    printf("Accepting new client connection\n");
                    char host[NI_MAXHOST], service[NI_MAXSERV];
                    int gnf;
                    if ((gnf = getnameinfo((struct sockaddr *)&peer_addr,
                                            peer_addrlen, host, NI_MAXHOST,
                                            service, NI_MAXSERV, NI_NUMERICSERV)) == -1) {
                        // maybe we should just ignore the failure
                        close(client);
                        fprintf(stderr, "[server]: getnameinfo: %s\n", gai_strerror(gnf));
                        continue;
                    }

                    printf("Accepted connection from %s:%s\n", host, service);

                    if (fd_count == fd_size) {
                        printf("Number of observed file descriptors has overflown - increasing\n");
                        fd_size++;
                        pfds = realloc(pfds, sizeof *pfds * fd_size);
                    }
                    pfds[fd_count].fd = client;
                    pfds[fd_count].events = POLLIN;
                    fd_count++;
                } else {
                    int client = pfds[i].fd;
                    int pid;
                    if (!(pid = fork())) {
                        close(listener);
                        serve_client(client);
                        close(client);
                        exit(0);
                    } else if (pid < 0) {
                        perror("fork");
                    } else {
                        printf("Forked child process %d to handle client data\n", pid);
                    }
                    close(client);
                    pfds[i--] = pfds[--fd_count];
                }
            }
        }
    }

    printf("Shutting down the server...\n");
    for (int i = 0; i < fd_count; i++) {
        close(pfds[i].fd);
    }

    free(pfds);

    return return_code;
}

