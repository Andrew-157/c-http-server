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

static volatile sig_atomic_t terminate = 0;
static void signal_handler(int);
static void handle_client_data(int);

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
                        handle_client_data(client);
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

static void signal_handler(int sig) {
    printf("Received signal %d\n", sig);  // Doesn't it spam since child processes constantly exit?
    if (sig == SIGCHLD) {
        // reap child processes
        while (waitpid(-1, NULL, WNOHANG) > 0);
    } else {
        terminate = 1;
    }
}

static void handle_client_data(int send_sock) {
    printf("[server]: Accepting client request\n");
    int nread;
    char buf[BUF_SIZE];
    if ((nread = recv(send_sock, buf, sizeof(buf), 0)) > 0) {
        buf[nread] = '\0';

        char c;
        char *request_line = NULL;
        //char *headers = NULL;
        for (int i = 0; i < nread; i++) {
            c = buf[i];
            if (c == '\r') {
                if (i < nread && buf[i+1] != '\n') {
                    fprintf(stderr, "[server]: Carriage return not followed by newline character\n");
                } else {
                    if (!request_line) {
                        printf("Extracting request line\n");
                        request_line = malloc(sizeof(char) * (i - 2 + 1));
                        for (int j = 0; j < i; j++)    {
                            request_line[j] = buf[j];
                        }
                    }
                }
            }
        }

        printf("[server]: Received %d bytes from client:\n%s\n", nread, buf);
        if (request_line) {
            printf("Request line: %.*s\n", (int)strlen(request_line), request_line);
            free(request_line);
        }
        char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<h1>C HTTP Server</h1>";
        printf("[server]: Sending response\n");
        send(send_sock, response, strlen(response), 0);
        printf("[server]: Response sent successfully\n");  // doubt
    } else if (nread == 0) {
        fprintf(stderr, "[server]: Client closed the connection\n");
    } else {
        perror("[server]: recv");
    }
}
