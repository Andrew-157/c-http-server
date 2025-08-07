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

#define HOST "localhost"
#define PORT "8080"

struct cli_data {
    char *host;
    char *port;
};

static struct cli_data cli(int, char **);
// return type could be `void *`, but I need to check that
static struct sockaddr_in * cast_structs(struct sockaddr *);
static int create_socket(char *, char *);

int main(int argc, char **argv) {
    struct cli_data data = cli(argc, argv);
    int sockfd;
    sockfd = create_socket(data.host, data.port);

    char *send_msg = "GET / HTTP/1.1\r\n\r\n";
    if (send(sockfd, send_msg, strlen(send_msg), 0) == -1) {
        close(sockfd);
        fprintf(stderr, "send: %s\n", strerror(errno));
        exit(errno);
    }

    char recv_msg[1024];
    int bytes_received;
    if ((bytes_received = recv(sockfd, recv_msg, sizeof(recv_msg), 0)) == -1) {
        close(sockfd);
        fprintf(stderr, "recv: %s\n", strerror(errno));
        exit(errno);
    }

    recv_msg[bytes_received] = '\0';
    printf("Received msg from server:\n%s\n", recv_msg);

    close(sockfd);

}

static struct cli_data cli(int argc, char **argv) {
    const char *help_text = "Usage: client [OPTIONS]\n\
A simple TCP client program.\n\
Options:\n\
  -n, --host <HOST>       Specify the host which to connect to - hostname or IPv4 IP address. If not provided \"%s\" will be used.\n\
  -p, --port <PORT>       Specify the port to which to connect to. If not provided \"%s\" will be used.\n\
  -h, --help              Display this help message and exit.\n\
";
    char *host = NULL;
    char *port = NULL;

    char *option;
    for (int i = 1; i < argc; ++i) {
        option = argv[i];
    }

    // TODO: This is hell, you can do something better
    if (argc > 1) {
        char *option = argv[1];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf(help_text, HOST, PORT);
            exit(0);
        } else if (strcmp(option, "-n") == 0 || strcmp(option, "--host") == 0) {
            if (argc < 3) {
                printf("-n/--host option used without a value.\n");
                exit(1);
            }
            host = argv[2];
        } else if (strcmp(option, "-p") == 0 || strcmp(option, "--port") == 0) {
            if (argc < 3) {
                printf("-p/--port option used without a value.\n");
                exit(1);
            }
            port = argv[2];
        } else {
            printf("Invalid option %s was used, see -h/--help.\n", argv[1]);
            exit(1);
        }
        option = argv[3];
        if (argc > 3) {
            if (strcmp(option, "-n") == 0 || strcmp(option, "--host") == 0) {
                if (host != NULL) {
                    printf("-n/--host option provided twice.\n");
                    exit(1);
                }
                if (argc < 4) {
                    printf("-n/--host option used without a value.\n");
                    exit(1);
                }
                host = argv[4];
            } else if (strcmp(option, "-p") == 0 || strcmp(option, "--port") == 0) {
                if (port != NULL) {
                    printf("-p/--port option provided twice.\n");
                    exit(1);
                }
                if (argc < 4) {
                    printf("-p/--port option used without a value.\n");
                    exit(1);
                }
                port = argv[4];
            } else {
                printf("Invalid option %s was used, see -h/--help.\n", argv[3]);
                exit(1);
            }
        }
    } else {
        host = HOST;
        port = PORT;
    }

    if (host == NULL) host = HOST;
    if (port == NULL) port = PORT;

    struct cli_data data;
    data.host = host;
    data.port = port;

    return data;
}


static int create_socket(char *host, char *port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char printable_ip[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(errno);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            fprintf(stderr, "socket: %s\n", strerror(errno));
            continue;
        }

        // I wonder if this is needed
        // This works: inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
        inet_ntop(p->ai_family, &(cast_structs(p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
        printf("attempting connection to %s(%s) on port %hd\n", host, printable_ip, ntohs(cast_structs(p->ai_addr)->sin_port));

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            fprintf(stderr, "connect: %s\n", strerror(errno));
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "failed to connect\n");
        exit(errno);
    }

    // This works: inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
    inet_ntop(p->ai_family, &(cast_structs(p->ai_addr)->sin_addr), printable_ip, sizeof(printable_ip));
    printf("attempting connection to %s(%s) on port %hd\n", host, printable_ip, ntohs(cast_structs(p->ai_addr)->sin_port));

    freeaddrinfo(servinfo);

    return sockfd;
}

static struct sockaddr_in * cast_structs(struct sockaddr *p_ai_addr) {
    return (struct sockaddr_in *)p_ai_addr;
}
