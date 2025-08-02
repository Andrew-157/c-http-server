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

#define PORT               "8080" // server port
#define BACKLOG            10     // number of pending connections
#define RECV_MSG_BUFFER    1024   // buffer size for recv method

int main() {
    int server_sockfd, client_sockfd;
    struct addrinfo hints, *servinfo, *p;
    socklen_t sin_size;
    struct sockaddr_in client_address;
    char printable_ip[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;   // to make it IP version agnostic, set AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags    = AI_PASSIVE;

    int result;
    if ((result = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        exit(errno);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((server_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket creation");
            continue;
        }

        // SO_REUSEADDR allows to avoid "Address already in use" error
        int yes = 1;
        if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(server_sockfd);
            perror("server: setsockopt");
            continue;
        }

        /* If after 5 seconds no client connects, `accept` fails on "Resource temporarily unavailable"
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            close(server_sockfd);
            perror("server: setsockopt");
            continue;
        }
        */

        if (bind(server_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_sockfd);
            perror("server: bind"); // Exemplary error: "server: bind: Permission denied" - when trying to use port 80, for example
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "failed to establish server socket\n");
        exit(errno);
    }

    if (listen(server_sockfd, BACKLOG) == -1) {
        perror("server: listen");
        exit(errno);
    }

    printf("accepting client connections...\n");

    sin_size = sizeof client_address;
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size);
    if (client_sockfd == -1) {
        perror("server: accept");
        exit(errno);
    }

    /* If after 5 seconds, client doesn't send data, `recv` will fail with "recv: Resource temporarily unavailable"
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_sockfd, SOL_SOCKET, SOL_RCVTIMEO, &timeout, sizeof(timeout));
    */

    inet_ntop(client_address.sin_family, &client_address.sin_addr, printable_ip, sizeof(printable_ip));
    printf("server: accepted connection from %s:%d\n", printable_ip, ntohs(client_address.sin_port));

    printf("Reading data from client\n");

    char recv_msg[RECV_MSG_BUFFER];
    int bytes_received;
    bytes_received = recv(client_sockfd, recv_msg, RECV_MSG_BUFFER, 0);
    // bytes_received == 0 - as I understand it happens when client closes the connection
    // bytes_received == -1 - error
    if (bytes_received == 0) {
        printf("looks like client closed the connection, ending communication\n");
        close(client_sockfd);
        close(server_sockfd);
        exit(0);
    } else if (bytes_received == -1) {
        perror("server: recv");
        close(client_sockfd);
        close(server_sockfd);
        exit(errno);
    }

    recv_msg[bytes_received] = '\0';
    printf("Received message from client:\n\n%s\n", recv_msg);

    printf("Sending an HTTP response to client with body with HTML Content-Type\n");

    const char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";

    // read template and put it into string
    FILE *file;
    file = fopen("./templates/index.html", "r");
    fseek(file, 0, SEEK_END); // Moves file pointer to the end of the file
    int length = ftell(file);
    fseek(file, 0, SEEK_SET); // Moves file pointer back to the beginning of the file

    // Do I need to use malloc or can I just do char html[length]?
    char *html = malloc(sizeof(char) * (length + 1)); // length + 1 because of '\0'

    int c, i;
    i = 0;
    while ((c = getc(file)) != EOF) // it could be fgetc instead of getc
        html[i++] = c;
    html[i] = '\0';
    fclose(file);

    send(client_sockfd, http_response, strlen(http_response), 0);
    send(client_sockfd, html, length, 0);
    free(html);

    printf("Ending communication\n");
    close(client_sockfd);
    close(server_sockfd);

    exit(0);
}
