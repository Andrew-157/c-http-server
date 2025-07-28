#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8080"

// return type could be `void *`, but I need to check that
static struct sockaddr_in * cast_structs(struct sockaddr *p);

int main() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char printable_ip[INET_ADDRSTRLEN]; // IPv4 address as a string

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(errno);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // This works: inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
        inet_ntop(p->ai_family, &(cast_structs(p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
        printf("attempting connection to %s on port %d\n", printable_ip, ntohs(cast_structs(p->ai_addr)->sin_port));

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(errno);
    }

    // This works: inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
    inet_ntop(p->ai_family, &(cast_structs(p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
    printf("client: connected to %s on port %d\n", printable_ip, ntohs(cast_structs(p->ai_addr)->sin_port));

    freeaddrinfo(servinfo);

    char *msg_to_server = "Hey, Server!\r\n";
    if (send(sockfd, msg_to_server, strlen(msg_to_server), 0) == -1) {
        perror("client: send");
        exit(errno);
    }

    char rcv_msg[1024];
    int bytes_received;
    if ((bytes_received = recv(sockfd, rcv_msg, 1024, 0)) == -1) {
        perror("cliend: recv");
        exit(errno);
    }

    rcv_msg[bytes_received] = '\0';
    printf("Received msg from server:\n%s\n", rcv_msg);

    close(sockfd);

    exit(0);
}

static struct sockaddr_in * cast_structs(struct sockaddr *p_ai_addr) {
    return (struct sockaddr_in *)p_ai_addr;
}
