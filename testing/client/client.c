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
    char *rqst_msg;
};

static struct cli_data cli(int, char **);
// return type could be `void *`, but I need to check that
static struct sockaddr_in * cast_structs(struct sockaddr *);
static int create_socket(char *, char *);

int main(int argc, char **argv) {
    struct cli_data data = cli(argc, argv);
    int sockfd;
    sockfd = create_socket(data.host, data.port);

    printf("Sending to server request: %s\n", data.rqst_msg);
    if (send(sockfd, data.rqst_msg, strlen(data.rqst_msg), 0) == -1) {
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
    const char *help_text = "Usage: client \"Request Message\" [OPTIONS]\n\
A simple HTTP client program.\n\
Options:\n\
  -n, --host <HOST>       Specify the host which to connect to - hostname or IPv4 IP address. If not provided \"%s\" will be used.\n\
  -p, --port <PORT>       Specify the port to which to connect to. If not provided \"%s\" will be used.\n\
  -h, --help              Display this help message and exit.\n\
";
    char *host = NULL;
    char *port = NULL;
    char *rqst_msg = NULL;
    char *option;

    for (int i = 1; i < argc; i++) {
        option = argv[i];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf(help_text, HOST, PORT);
            exit(0);
        } else if (strcmp(option, "-n") == 0 || strcmp(option, "--host") == 0) {
            // TODO: add validation whether host is a valid FQDN/IP?
            if (host != NULL) {
                printf("-n/--host option was provided twice.\n");
                exit(1);
            }
            if ((i+1) == argc) {
                printf("-n/--host option used without a value.\n");
                exit(1);
            }
            if (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0) {
                // TODO: add a separate help text for --host option
                printf("Help text for --host\n");
                exit(0);
            }
            host = argv[++i];
        } else if (strcmp(option, "-p") == 0 || strcmp(option, "--port") == 0) {
            // TODO: add validation whether port is integer?
            if (port != NULL) {
                printf("-p/--port option was provided twice.\n");
                exit(1);
            }
            if ((i+1) == argc) {
                printf("-p/--port option used without a value.\n");
                exit(1);
            }
            if (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0) {
                // TODO: add a separate help text for --port option
                printf("Help text for --port\n");
                exit(0);
            }
            port = argv[++i];
        } else {
            rqst_msg = option;
        }
    }

    if (rqst_msg == NULL) {
        printf("Request Message was not provided to the client program\n");
        exit(1);
    }

    if (host == NULL) host = HOST;
    if (port == NULL) port = PORT;
    printf("Host: %s, Port: %s\n", host, port);
    struct cli_data data;
    data.host = host;
    data.port = port;
    data.rqst_msg = rqst_msg;

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
