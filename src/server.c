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

#define PORT                 "8080" // port on which to listen for incoming connections
#define BACKLOG              10     // maximum number of pending connections
#define RECV_MSG_BUFFER_SIZE 1024   // recv buffer

struct cli_data {
    int enable_debug;
};

static struct cli_data cli(int, char **);

void signal_handler(int);
int create_server_socket(char *);
char *read_template(char *);
char *accept_rqst(int, int);

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

        accept_rqst(client_sockfd, RECV_MSG_BUFFER_SIZE);

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

char * accept_rqst(int client_sockfd, int recv_msg_buffer_size) {
    log_message(INFO, "Reading message from client\n");

    char recv_msg[recv_msg_buffer_size];
    int chunk_bytes_received;
    chunk_bytes_received = recv(client_sockfd, recv_msg, recv_msg_buffer_size, 0);
    if (chunk_bytes_received == 0) {
        log_message(WARNING, "client closed connection, ending communication\n");
        return NULL;
    } else if (chunk_bytes_received == -1) {
        log_message(ERROR, "error occurred while trying to read from client socket: %s, ending communication\n", strerror(errno));
        return NULL;
    }

    log_message(INFO, "Bytes received: %d\n", chunk_bytes_received);

    // utils for reading request line
    char rqst_line[100]; // i don't have a slightest idea what would be the correct size
    int rqst_line_read = 0;
    char method[8]; // is OPTIONS the longest method?
    int method_read = 0;
    char url[100]; // also no idea how long the url should be
    int url_read = 0;
    char protocol[9]; // yeah, im sure this time
    int protocol_read = 0;

    // utils for reading headers
    char curr_header[100]; // variable to store one header at a time

    int j = 0; // index for keeping track of element in method, url, protocol
    for (int i = 0; i < chunk_bytes_received; i++) {
        if ((recv_msg[i] == '\r') && ((i+1) < chunk_bytes_received) && (recv_msg[i+1] == '\n')) {
            log_message(DEBUG, "Encountered CRLF\n");
            if (!rqst_line_read) {
                protocol[j] = '\0';
                j = 0;
                protocol_read = 1;
                rqst_line[i] = '\0';
                rqst_line_read = 1;
                log_message(DEBUG, "Finished reading request line\n");
                log_message(DEBUG, "Received request line from client:\n%s\n", rqst_line);
                log_message(DEBUG, "Request message method is: %s\n", method);
                log_message(DEBUG, "Request message url is: %s\n", url);
                log_message(DEBUG, "Request message protocol is: %s\n", protocol);
             } else {
                curr_header[j] = '\0';
                j = 0;
                log_message(DEBUG, "Read a header: %s\n", curr_header);
             }
             if ((i+3) < chunk_bytes_received && recv_msg[i+2] == '\r' && recv_msg[i+3] == '\n') {
                 log_message(DEBUG, "Encountered double CRLF\n");
                 if ((i+4) < chunk_bytes_received) {
                     log_message(DEBUG, "There is more to read after double CRLF, continuing\n");
                     i += 3;
                     continue;
                 }
                 log_message(DEBUG, "HTTP request message has been completely read\n");
                 break;
             } else
                 i++; // skip next character which is '\n'
             continue;
        }

        if (!rqst_line_read)
            rqst_line[i] = recv_msg[i];
        else
            curr_header[j++] = recv_msg[i];

        if (recv_msg[i] == ' ') {
            if (!method_read) {
                method[j] = '\0';
                method_read = 1;
                j = 0;
            } else if (!url_read) {
                url[j] = '\0';
                url_read = 1;
                j = 0;
            }
        } else {
            if (!method_read) {
                method[j++] = recv_msg[i];
            } else if (!url_read) {
                url[j++] = recv_msg[i];
            } else if (!protocol_read) {
                protocol[j++] = recv_msg[i];
            }
        }
    }

    return NULL;
}

