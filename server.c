#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define BUF_SIZE 1000

int main(void) {
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

}
