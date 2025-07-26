#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#define PORT "8080"
#define BACKLOG 10 // Right now I can do absolutely nothing with that, but just so I know about that param
#define RCV_MSG_BUFFER 1024


int main() {

    int sockfd, new_fd;
    socklen_t sin_size;
    struct sockaddr_in their_addr;
    struct addrinfo hints, *servinfo, *p;
    char s[INET_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Could be AF_UNSPEC, to be IP version agnostic
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(errno);
    }

    // I honestly don't understand why we need to do it to create server structure if we already
    // know its parameters, but whatever
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) { // Allows to avoid "Address already in use" error
            perror("setsockopt");
            exit(errno);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind"); // server: bind: Permission denied - when using port 80, for example
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(errno);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(errno);
    }

    printf("server: waiting for connections...\n");

    // There are 10 BACKLOG connections, but I am accepting only one for now,
    // moreover, it means that after accepting one client socket, I can close server socket
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        exit(errno);
    }

    inet_ntop(their_addr.sin_family, &their_addr.sin_addr, s, sizeof(s));
    printf("server: got connection from IP: %s PORT: %d\n", s, their_addr.sin_port);

    char message[RCV_MSG_BUFFER];
    if (recv(new_fd, message, RCV_MSG_BUFFER, 0) == -1) // 0 is returned when client closes connection
        perror("recv error");

    printf("Received message from client:\n\n");
    printf("%s\n", message);

    printf("Writing to client minimal HTTP response\n");
    //char *response = "HTTP/1.1 200 OK\r\n\r\n";
    //send(new_fd, response, strlen(response), 0);

    // Putting HTML template into a string
    char *template_path = "./templates/index.html";
    FILE *file;
    file = fopen(template_path, "r");
    fseek(file, 0, SEEK_END); // Moves file pointer to the end of the file
    int length = ftell(file);
    fseek(file, 0, SEEK_SET); // Moves file pointer back to the beginning of the file

    char *html = malloc(sizeof(char) * (length+1)); // length + 1 because '\0'
    int c;
    int i = 0;
    while ((c = getc(file)) != EOF) // could be fgetc instead of getc
        html[i++] = c;
    html[i] = '\0';

    fclose(file);
    char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 232\r\n\r\n";
    send(new_fd, http_response, strlen(http_response), 0);
    send(new_fd, html, length, 0);

    free(html);

    printf("Ending communication\n");
    close(new_fd);
    close(sockfd);

}
