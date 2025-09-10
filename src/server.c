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

#define PORT                 "8080" // port on which to listen for incoming connections
#define BACKLOG              10     // maximum number of pending connections

struct cli_data {
    int enable_debug;
};

static struct cli_data cli(int, char **);

void signal_handler(int);
int create_server_socket(char *);
char *read_template(char *);

// add atexit maybe
int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    setupLogger(cli(argc, argv).enable_debug);

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

        accept_rqst(client_sockfd);

        log_message(INFO, "Sending an HTTP response to client with HTML body\n");

        char *html = read_template("./templates/index.html");
        if (html == NULL) {
            log_message(ERROR, "Failed to open html template: %s\n", strerror(errno));
            close(client_sockfd);
            close(server_sockfd);
            exit(errno);
        }

        char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";
        send(client_sockfd, http_response, strlen(http_response), 0);
        send(client_sockfd, html, strlen(html), 0);
        free(html);

        log_message(INFO, "Ending communication\n\n");
        close(client_sockfd);
    }
    close(server_sockfd);
    exit(0);
}


static struct cli_data cli(int argc, char **argv) {
    // NOTE: --debug is kinda bad name for enabling debug messages, debug could also mean prod or non-prod server mode
    const char *help_text = "Usage: server [OPTIONS]\n\
A simple HTTP server.\n\
Options:\n\
   -d, --debug    Enable debug logging. If flag is not provided, messages with level DEBUG won't be sent to stdout.\n\
";
    int debug;
    debug = 0;

    char *option;
    if (argc > 1) {
        option = argv[1];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf("%s", help_text);
            exit(0);
        } else if (strcmp(option, "-d") == 0 || strcmp(option, "--debug") == 0)
            debug = 1;
        else {
            printf("Invalid option: \"%s\". Use -h/--help to see available options.\n", option); // Should it go to stderr?
            exit(1);
        }
    }

    struct cli_data data;
    data.enable_debug = debug;
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

char *read_template(char *template_path) {
    FILE *file;
    if ((file = fopen(template_path, "r")) == NULL) return NULL;
    fseek(file, 0, SEEK_END); // Moves file pointer to the end of the file
    int length = ftell(file);
    fseek(file, 0, SEEK_SET); // Moves file pointer back to the beginning of the file

    char *html = malloc(sizeof(char) * (length + 1)); // length + 1 because of '\0'
    int c, i;
    i = 0;
    while ((c = getc(file)) != EOF) // it could be `fgetc` instead of `getc`
        html[i++] = c;
    html[i] = '\0';
    fclose(file);
    return html;
}

