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

int create_server_socket(char *);
char *read_template(char *);
void accept_rqst(int, int, unsigned long, unsigned long long);

int main() {
    int server_sockfd;
    server_sockfd = create_server_socket(PORT);

    if (server_sockfd == -1) {
        fprintf(stderr, "Failed to create server socket: %s\n", strerror(errno));
        exit(errno);
    }

    if (listen(server_sockfd, BACKLOG) == -1) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        exit(errno);
    }

    printf("accepting client connections...\n");

    int client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in client_address;
    char printable_ip[INET_ADDRSTRLEN];

    sin_size = sizeof client_address;
    if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size)) == -1) {
        close(server_sockfd);
        fprintf(stderr, "accept: %s\n", strerror(errno));
        exit(errno);
    }

    // timeout on `recv`
    struct timeval timeout;
    timeout.tv_sec = 1; // even 1 second is a lot probably
    timeout.tv_usec = 0;
    if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        close(client_sockfd);
        close(server_sockfd);
        fprintf(stderr, "setsockopt: %s\n", strerror(errno));
        exit(errno);
    }

    inet_ntop(client_address.sin_family, &client_address.sin_addr, printable_ip, sizeof(printable_ip));
    printf("accepted client connection from IP(%s):PORT(%d)\n", printable_ip, ntohs(client_address.sin_port));

    printf("Reading from client socket\n");

    char recv_msg[RECV_MSG_BUFFER];
    int bytes_received;
    bytes_received = recv(client_sockfd, recv_msg, RECV_MSG_BUFFER, 0);
    // bytes_received == 0  - client closed connection ... I guess
    // bytes_received == -1 - error occurred
    if (bytes_received == 0) {
        printf("client closed connection, ending communication\n");
        close(client_sockfd);
        close(server_sockfd);
        exit(0);
    } else if (bytes_received == -1) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        close(client_sockfd);
        close(server_sockfd);
        exit(errno);
    }
    recv_msg[bytes_received] = '\0';
    printf("Message from client:\n%s\n", recv_msg);

    printf("Sending an HTTP response to client with HTML body\n");

    char *html = read_template("./templates/index.html");
    if (html == NULL) {
        fprintf(stderr, "failed to open html template: %s\n", strerror(errno));
        close(client_sockfd);
        close(server_sockfd);
        exit(errno);
    }

    char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";
    send(client_sockfd, http_response, strlen(http_response), 0);
    send(client_sockfd, html, strlen(html), 0);
    free(html);

    printf("Ending communication\n");
    close(client_sockfd);
    close(server_sockfd);

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
            fprintf(stderr, "socket: %s\n", strerror(errno));
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            sockfd = -1;
            close(sockfd);
            fprintf(stderr, "setsockopt: %s\n", strerror(errno));
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            sockfd = -1;
            close(sockfd);
            fprintf(stderr, "bind: %s\n", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
        fprintf(stderr, "server: failed to bind\n");

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

/*
 * Read from client socket and ... I don't know yet
 * - client sockfd (int) - client socket to read from,
 * - recv_msg_buffer (int) - buffer size in bytes for reading client socket using `recv`,
 * - max_rqst_line_headers_size (unsigned long) - maximum size in bytes of request line + header lines,
 * - max_body_size (unsigned long long) - maximum size of request body in bytes
 */
void accept_rqst(int client_sockfd, int recv_msg_buffer, unsigned long max_rqst_line_headers_size, unsigned long long max_body_size) {
    //int recv_retries; // maximum amount of times `recv` function will be called in attempt to receive full HTTP message
    // Rethink that: recv_retries = (int)((rqst_line_headers_size + body_size) / recv_msg_buffer) + 1;
    char recv_msg[recv_msg_buffer];
    int bytes_received;

    // Check how many bytes of everything we received
    int rqst_line_bytes_received, headers_bytes_received, body_bytes_received;
    rqst_line_bytes_received = 0;
    headers_bytes_received = 0;
    body_bytes_received = 0;

    // bool values to know whether an entity was read
    int rqst_line_received, headers_received, body_received;
    rqst_line_received = 0;
    headers_received = 0;
    body_received = 0;

    // Value from Content-Length header
    int content_length = 0;

    int msg_complete = 0;
    while (!msg_complete) {
        bytes_received = recv(client_sockfd, recv_msg, recv_msg_buffer, 0);
        if (!rqst_line_received) {
            rqst_line_bytes_received += bytes_received;
            if (rqst_line_bytes_received > max_rqst_line_headers_size) {
                 // Return Error to client "413 Entity Too Large"
                 break;
            }
        } else if (!headers_received) {
            headers_bytes_received += bytes_received;
            if ((rqst_line_bytes_received + headers_bytes_received) > max_rqst_line_headers_size) {
                // Return error to client "413 Entity Too Large"
                break;
            }
        } else if (!body_received) {
            // we can catch that early when reading Content-Length header, but
            // - what if this header is absent - if this is the method that needs body, then we need to return error
            // - what if actual body size is not equal to what is set in header, we could simply stop reading after Content-Length value
            // was received, but it is not very good to operate on not complete data, and may leave, for example,
            // some broken database record
            if (!content_length) {
                // Return error to client "400 Bad Request"
                break;
            } else if (content_length > max_body_size) {
               // Return error to client "413 Entity Too Large" - or should it be better done at the point of parsing headers
               break;
            }
            // What to do when receive body is less than Content-Length?
            body_bytes_received += bytes_received;
            if (body_bytes_received > max_body_size) {
                // in case Content-Length header doesn't equal what is truly sent
                // Return error to client "413 Entity Too Large"
                break;
            }
        }
    }
}
