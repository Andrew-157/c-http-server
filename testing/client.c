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

#include "default_config.h"

// return type could be `void *`, but I need to check that
static struct sockaddr_in * cast_structs(struct sockaddr *p);

int main(int argc, char **argv) {

    const char *help_text = "Usage: client [OPTIONS]\n\
A simple TCP client program.\n\
Options:\n\
  -n, --host <HOST>       Specify the host which to connect to - hostname or IPv4 IP address. If not provided \"%s\" will be used.\n\
  -h, --help              Display this help message and exit.\n\
";
    char *host; // TODO: allow passing port too
    if (argc > 1) {
        char *option = argv[1];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf(help_text, HOST);
            exit(0);
        } else if (strcmp(option, "-n") == 0 || strcmp(option, "--host") == 0) {
            if (argc < 3) {
                printf("-n/--host option used without a value.\n");
                exit(1);
            }
            host = argv[2];
        }
        else {
           printf("Invalid option %s, use -h/--help to see all supported options.\n", argv[1]);
           exit(1);
        }
    } else
       host = HOST;

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char printable_ip[INET_ADDRSTRLEN]; // IPv4 address as a string

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, "8081", &hints, &servinfo)) != 0) {
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

    printf("Not sending anything to server to see how it behaves\n");

    int sleep_time = 10;
    printf("Sleeping for %ds\n", sleep_time);
    sleep(sleep_time);

    //char *msg_to_server = "Hey, Server!\r\n";
    //if (send(sockfd, msg_to_server, strlen(msg_to_server), 0) == -1) {
    //    perror("client: send");
    //    exit(errno);
    //}

    //char rcv_msg[1024];
    //int bytes_received;
    //if ((bytes_received = recv(sockfd, rcv_msg, 1024, 0)) == -1) {
    //    perror("cliend: recv");
    //    exit(errno);
   // }

    //rcv_msg[bytes_received] = '\0';
    //printf("Received msg from server:\n%s\n", rcv_msg);

    close(sockfd);

    exit(0);
}

static struct sockaddr_in * cast_structs(struct sockaddr *p_ai_addr) {
    return (struct sockaddr_in *)p_ai_addr;
}
