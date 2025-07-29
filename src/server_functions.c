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

#include "default_config.h"
#include "server_functions.h"

static int create_server_socket(const char *port, int backlog);
static char *read_template(char *template_path);
static char *read_rcv_msg(int client_sockfd);

void run() {
    int server_sockfd, client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in client_address;
    char printable_ip[INET_ADDRSTRLEN]; // IPv4 address as a string

    server_sockfd = create_server_socket(PORT, BACKLOG);

    // TODO: handle multiple clients
    sin_size = sizeof client_address;
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size);
    if (client_sockfd == -1) {
        perror("server: accept");
        exit(errno);
    }

    inet_ntop(client_address.sin_family, &client_address.sin_addr, printable_ip, sizeof(printable_ip));
    printf("server: accepted connection from %s:%d\n", printable_ip, ntohs(client_address.sin_port));

    printf("Receiving data from client\n");

    char *rcv_msg = read_rcv_msg(client_sockfd);
    if (rcv_msg == NULL) {
        printf("Client either closed the connection or error on read occured\n");
        printf("Ending communication\n");
        close(client_sockfd);
        close(server_sockfd);
    } else {
        printf("Received message from client:\n");
        printf("%s", rcv_msg);
        free(rcv_msg);

        printf("Sending an HTML template to client\n");

        char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";
        char *html = read_template("./templates/index.html");
        send(client_sockfd, http_response, strlen(http_response), 0);
        send(client_sockfd, html, strlen(html), 0);
        free(html);

        printf("Ending communication\n");
        close(client_sockfd);
        close(server_sockfd);
    }
}

static char * read_rcv_msg(int client_sockfd) {
    // this string and its size should be growing in a loop depending on how much data we received
    // from client I think, or we can allocate in advance if we know max size for the headers and
    // request line, but what about the body?
    char *rcv_msg = malloc(sizeof(char) * RCV_MSG_BUFFER); // this is to be able to return string from function
    int bytes_received;
    bytes_received = recv(client_sockfd, rcv_msg, RCV_MSG_BUFFER, 0);
    if (bytes_received == 0) { // as i understand, 0 is returned when client closes connection, which means there is no point to try to communicate anymore
        printf("Client closed the connection\n");
        free(rcv_msg);
        return NULL;
    } else if (bytes_received == -1) {
        perror("recv error");
        free(rcv_msg);
        return NULL;
    }
    rcv_msg[bytes_received] = '\0';
    return rcv_msg;
}

static int create_server_socket(const char *port, int backlog) {
    int sockfd;
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

    printf("server is waiting for connections...\n");
    return sockfd;
}

static char *read_template(char *template_path) {
    FILE *file;
    file = fopen(template_path, "r");
    fseek(file, 0, SEEK_END); // Moves file pointer to the end of the file
    int length = ftell(file);
    fseek(file, 0, SEEK_SET); // Moves file pointer back to the beginning of the file

    char *html = malloc(sizeof(char) * (length + 1)); // length + 1 because of '\0'

    int c, i;
    i = 0;
    while ((c = getc(file)) != EOF)// it could be fgetc instead of getc
        html[i++] = c;
    html[i] = '\0';
    fclose(file);
    return html;
}

