#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT            "8080" // port on which to listen for incoming connections
#define BACKLOG         10     // maximum number of pending connections
#define RECV_MSG_BUFFER 1024   // recv buffer

int create_server_socket(char *port, int );

int main() {
    int server_sockfd;
    server_sockfd = create_server_socket(PORT);

    if (server_sockfd == NULL) {
        fprintf(stderr, "Failed to create server socket\n");
        exit(errno);
    }

    if (listen(server_sockfd, BACKLOG) = -1) {
        fprintf("listen: %s\n", strerror(errno));
        exit(errno);
    }

    printf("accepting client connections...\n");

    socklen_t sin_size;
    struct sockaddr_in client_address;
    char printable_ip[INET_ADDRSTRLEN];

    sin_size = sizeof client_address;
    if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size)) == -1) {
        close(server_sockfd);
        fprintf("accept: %s\n", strerror(errno));
        exit(errno);
    }

    // timeout on `recv`
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(client_sockfd, SOL_SOCKET, SOL_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        close(server_sockfd);
        close(client_sockfd);
        fprintf("setsockopt: %s\n", strerror(errno));
        exit(errno);
    }

    inet_ntop(client_address.sin_family, &client_address.sin_addr, printable_ip, sizeof(printable_ip));
    printf("accepted client connection from IP(%s):PORT(%d)\n", printable_ip, ntohs(client_address.sin_port));

    printf("Reading from client socket\n");


    exit(0);
}

int create_server_socket(char *port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;     // IPv4, to be IP-version-agnostic, set it to AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = AI_PASSIVE;

    int result;
    if ((result = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        exit(errno);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            sockfd = NULL;
            fprintf(stderr, "socket: %s\n", strerror(errno));
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            sockfd = NULL;
            close(sockfd);
            fprintf(stderr, "setsockopt: %s\n", strerror(errno));
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            sockfd = NULL;
            close(sockfd);
            fprintf(stderr, "bind: %s\n", strerror(errno));
            continue;
        }

        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}
