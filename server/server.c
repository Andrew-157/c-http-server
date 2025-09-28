#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "logger.h"
#include "request_handling.h"

#define PORT        "8080" // port on which to listen for incoming connections
#define BACKLOG     10     // maximum number of pending connections

#define BUFFER_SIZE 1024 // default buffer size, will be moved somewhere later

struct cli_data {
    int verbose;
    int buffer_size;
};

static struct cli_data cli(int, char **);

void signal_handler(int);
int create_server_socket(char *);

// add atexit maybe
int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct cli_data data = cli(argc, argv);

    setupLogger(data.verbose);

    int server_sockfd;
    server_sockfd = create_server_socket(PORT);

    if (server_sockfd == -1) {
        log_message(ERROR, "Failed to create server socket: %s\n", strerror(errno));
        exit(errno);
    }

    if (listen(server_sockfd, BACKLOG) == -1) {
        log_message(ERROR, "listen: %s\n", strerror(errno));
        exit(errno);
    }

    log_message(INFO, "Accepting client connections...\n");

    int client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in client_address;
    char printable_ip[INET_ADDRSTRLEN];

    sin_size = sizeof client_address;

    while (1) {
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size)) == -1) {
            close(server_sockfd);
            log_message(ERROR, "accept: %s", strerror(errno));
            exit(errno);
        }

        // timeout on `recv`
        struct timeval timeout;
        timeout.tv_sec = 1; // even 1 second is a lot probably
        timeout.tv_usec = 0;
        if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            close(client_sockfd);
            close(server_sockfd);
            log_message(ERROR, "setsockopt: %s\n", strerror(errno));
            exit(errno);
        }

        inet_ntop(client_address.sin_family, &client_address.sin_addr, printable_ip, sizeof(printable_ip));
        // TODO: try to also print hostname of the client, if possible
        log_message(INFO, "Accepted client connection from IP(%s):PORT(%d)\n", printable_ip, ntohs(client_address.sin_port));

        accept_rqst(client_sockfd, data.buffer_size);

        //log_message(INFO, "Sending an HTTP response to client with HTML body\n");

        //char *html = read_template("./templates/index.html");
        //if (html == NULL) {
        //    log_message(ERROR, "Failed to open html template: %s\n", strerror(errno));
        //    close(client_sockfd);
        //    close(server_sockfd);
        //    exit(errno);
        //}

        //char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";
        //send(client_sockfd, http_response, strlen(http_response), 0);
        //send(client_sockfd, html, strlen(html), 0);
        //free(html);

        //log_message(INFO, "Ending communication\n\n");
        //close(client_sockfd);
    }
    close(server_sockfd);
    exit(0);
}


static struct cli_data cli(int argc, char **argv) {
    const char *help_text = "Usage: server [OPTIONS]\n\
A simple HTTP server.\n\
Options:\n\
   -v, --verbose     Enable verbose logging. If flag is not provided, messages with level DEBUG won't be sent to stdout.\n\
   -b, --buff-size   Receive buffer size. If not provided, default value of %d will be used.\n\
";
    int verbose = 0;      // this option is store_true (in argparse terminology), so if it is not 0, it means user provided it twice
    int buffer_size = -1; // not a valid value, simply a value to detect that buffer_size was already provided, as
                          // values of 0 and -1 are not valid values for buffer_size and we won't let user set them

    char *option;
    for (int i = 1; i < argc; i++) {
        option = argv[i];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf(help_text, BUFFER_SIZE);
            exit(0);
        } else if (strcmp(option, "-b") == 0 || strcmp(option, "--buffer-size") == 0) {
            if (buffer_size != -1) {
                printf("-b/--buffer-size option was provided twice.\n");
                exit(1);
            }
            if ((i+1) == argc) {
                printf("-b/--buffer-size option used without a value.\n");
                exit(1);
            }
            if (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0) {
                printf("help text for --buffer-size\n");
                exit(0);
            }
            for (int j = 0; j < strlen(argv[i+1]); j++) {
                if (!isdigit(argv[i+1][j])) {
                    printf("-b/--buffer-size must be a positive integer.\n");
                    exit(1);
                }
            }
            buffer_size = atoi(argv[i+1]);
            if (buffer_size == 0) {
                printf("-b/--buffer-size must be a positive integer.\n");
                exit(1);
            }
            i++;
        } else if (strcmp(option, "-v") == 0 || strcmp(option, "--verbose") == 0) {
            if (verbose) {
                printf("-v/--verbose option was provided twice.\n");
                exit(1);
            }
            if (i < (argc-1) && (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0)) {
                printf("help text for --verbose\n");
                exit(0);
            }
            verbose = 1;
        } else {
            printf("Invalid option: %s, use -h/--help to see available options.\n", option);
            exit(1);
        }
    }

    struct cli_data data;
    data.verbose = verbose;
    if (buffer_size < 0) {
        data.buffer_size = BUFFER_SIZE;
    } else {
        data.buffer_size = buffer_size;
    }
    return data;
}

void signal_handler(int sig) {
    log_message(WARNING, "Caught signal: %d\n", sig);
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
        log_message(ERROR, "getaddrinfo: %s\n", gai_strerror(result));
        exit(errno);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_message(ERROR, "socket: %s\n", strerror(errno));
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            sockfd = -1;
            close(sockfd);
            log_message(ERROR, "setsockopt: %s\n", strerror(errno));
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            sockfd = -1;
            close(sockfd);
            log_message(ERROR, "bind: %s\n", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
        log_message(ERROR, "server: failed to bind\n");

    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}
