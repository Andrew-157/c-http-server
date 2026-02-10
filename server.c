#define _GNU_SOURCE
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define BUF_SIZE 1000

static volatile sig_atomic_t terminate = 0;

static void signal_handler(int sig) {
    printf("[server]: Received signal: %d\n", sig);
    terminate = 1;
}

void handle_client_data(int, int);

int main() {
    struct sigaction act = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&act.sa_mask);
    int signals[2] = {
        SIGINT,
        SIGTERM,
    };
    for (int i = 0; i < sizeof(signals) / sizeof(int); i++) {
        int sig = signals[i];
        if (sigaction(sig, &act, NULL) == -1) {
            perror("[server]: sigaction");
            exit(EXIT_FAILURE);
        }
    }

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int gai;
    if ((gai = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
        fprintf(stderr, "[server]: getaddrinfo: %s\n", gai_strerror(gai));
        exit(EXIT_FAILURE);
    }

    int listen_sock;
    int yes = 1;
    for (rp = result; rp != NULL; rp = rp -> ai_next) {
        listen_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_sock == -1) {
            perror("[server]: socket");
            continue;
        }

        if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(listen_sock);
            perror("[server]: setsockopt");
            continue;
        }

        if (bind(listen_sock, rp->ai_addr, rp->ai_addrlen) == -1) {
            close(listen_sock);
            perror("[server]: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        fprintf(stderr, "[server]: failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock, BACKLOG) == -1) {
        perror("[server]: listen");
        exit(EXIT_FAILURE);
    }

    int fd_size = 5;
    int fd_count = 0;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    pfds[0].fd = listen_sock;
    pfds[0].events = POLLIN;

    fd_count = 1;
    int exit_code = EXIT_SUCCESS;

    while (!terminate) {
        sigset_t sigmask;
        sigemptyset(&sigmask);
        int poll_count = ppoll(pfds, fd_count, NULL, &sigmask);

        if (poll_count == -1) {
            perror("[server]: ppoll");
            exit_code = EXIT_FAILURE;
            break;
        } else {
            for (int i = 0; i < fd_count; i++) {
                if (pfds[i].revents & POLLIN) {
                    if (pfds[i].fd == listen_sock) {
                        struct sockaddr_storage peer_addr;
                        socklen_t peer_addrlen = sizeof peer_addr;
                        int client_sock;
                        if ((client_sock = accept(listen_sock, (struct sockaddr *)&peer_addr, &peer_addrlen)) == -1) {
                            perror("[server]: accept");
                            continue;
                        } else {
                            printf("[server]: Accepting new connection\n");
                            int gnf;
                            char host[NI_MAXHOST], service[NI_MAXSERV];
                            if ((gnf = getnameinfo((struct sockaddr *)&peer_addr,
                                                    peer_addrlen, host, NI_MAXHOST,
                                                    service, NI_MAXSERV, NI_NUMERICSERV)) == 0) {
                                printf("[server]: Accepted connection from %s:%s\n", host, service);
                            } else {
                                fprintf(stderr, "[server]: getnameinfo: %s\n", gai_strerror(gnf));
                            }
                            if (fd_count == fd_size) {
                                printf("[server]: Number of client has overflown  - increasing\n");
                                fd_size++;
                                pfds = realloc(pfds, sizeof *pfds * fd_size);
                            }
                            pfds[fd_count].fd = client_sock;
                            pfds[fd_count].events = POLLIN;
                            fd_count++;
                        }
                    } else {
                        int client_sock = pfds[i].fd;
                        handle_client_data(client_sock, BUF_SIZE);
                        close(client_sock);
                        pfds[i--] = pfds[fd_count--];
                    }
                }
            }
        }
    }

    printf("[server]: Closing all connections and listening socket\n");
    for (int i = 0; i < fd_count; i++) {
        close(pfds[i].fd);
    }

    free(pfds);

    exit(exit_code);
}

void handle_client_data(int client_sock, int buf_size) {
    printf("[server]: Accepting client request\n");
    int nread;
    char buf[buf_size];
    if ((nread = recv(client_sock, buf, sizeof(buf), 0)) > 0) {
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
        send(client_sock, response, strlen(response), 0);
        printf("[server]: Response sent successfully\n");  // doubt
    } else if (nread == 0) {
        fprintf(stderr, "[server]: Client closed the connection\n");
    } else {
        perror("[server]: recv");
    }
}
