#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define BUF_SIZE 1000

static volatile sig_atomic_t terminate = 0;
static void signal_handler(int);

int main(void) {

    struct sigaction act = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&act.sa_mask);
    int signals[2] = {
        SIGINT,
        SIGTERM,
    };
    for (int i = 0; i < 2; i++) {
        int sig = signals[i];
        if (sigaction(sig, &act, NULL) == -1) {
            perror("[server]: sigaction");
            exit(EXIT_FAILURE);
        }
    }


    struct addrinfo hints, *result, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s;
    if ((s = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
        fprintf(stderr, "[server]: getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    int server_sock;
    int yes = 1;
    for (p = result; p != NULL; p = p->ai_next) {
        if ((server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[server]: socket");
            continue;
        }

        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(server_sock);
            perror("[server]: setsockopt");
            continue;
        }

        if (bind(server_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_sock);
            perror("[server]: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (p == NULL) {
        fprintf(stderr, "[server]: failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, BACKLOG) == -1) {
        perror("[server]: listen");
        exit(EXIT_FAILURE);
    }

    while (!terminate) {
        char host[NI_MAXHOST], service[NI_MAXSERV];
        socklen_t peer_addrlen;
        struct sockaddr_storage peer_addr;
        int client_sock;
        if ((client_sock = accept(server_sock, (struct sockaddr *)&peer_addr, &peer_addrlen)) == -1) {
            perror("[server]: accept");
            continue;
        }

        int s;
        if ((s = getnameinfo((struct sockaddr *)&peer_addr,
                            peer_addrlen, host, NI_MAXHOST,
                            service, NI_MAXSERV, NI_NUMERICSERV)) != 0) {
            close(client_sock);
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
            continue;
        }

        int pid;
        if (!(pid = fork())) {
            close(server_sock);
            printf("[server(child)]: Serving %s:%s\n", host, service);
            if (send(client_sock, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(client_sock);
            exit(0);
        } else if (pid < 0) {
            close(client_sock);
            perror("[server]: fork");
            continue;
        } else {
            printf("[server]: forked child process %d to handle data from %s:%s\n", pid, host, service);
            close(client_sock);
        }
    }

    exit(EXIT_SUCCESS);
}

static void signal_handler(int sig) {
    printf("[server]: Received signal: %d\n", sig);
    terminate = 1;
}
