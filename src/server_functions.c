#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "server_functions.h"

static int create_server_socket(const char *port, int backlog) {
    int sockfd;
    socklen_t sin_size;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // could be AF_UNSPEC to be IP version agnostic, but I want IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    int result;
    if ((result = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        exit(errno);
    }

    // NOTE: maybe `bind` and `listen` should be called in `run` function
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {// Allows to avoid "Address already in use" error
            perror("setsockopt");
            exit(errno);
        }

        // TODO: setsockopt SO_RCVTIMEO - seconds and microsecond should be passed via parameters

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind"); // "server: bind: Permission denied" - when trying to use port 80, for example
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(errno);
    }

    if (listen(sockfd, backlog) == -1) {
        perror("server: listen");
        exit(errno);
    }

    printf("server is waiting for connections...");
}
