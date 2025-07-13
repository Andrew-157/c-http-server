#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // contains read, write, etc.
#include <arpa/inet.h> // contains stuff for socket manipulations

#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
    int server_fd, client_fd; // file descriptors for server and client sockets
    struct sockaddr_in address; // Describes an IPv4 Internet domain socket address. The sin_port and sin_addr members are stored in network byte order.
    int opt = 1; // idk what that is
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // int socket(int domain, int type, int protocol);
                                     // AF_INET - IPv4 Internet protocols
                                     // SOCK_STREAM - TCP
                                     // 0 - The protocol specifies a particular protocol to be used with the socket.  Normally only a single protocol exists to support a particular socket type within a given protocol family, in which case protocol can be specified as 0.
    if (server_fd == -1) {
        fprintf(stderr, "Server socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Created server socket successfully\n");

    // setting options on a socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) { // I am not completely sure what it is and why it is needed
        fprintf(stderr, "Setting of socket options failed\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); // Can I set it to 127.0.0.1?
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        fprintf(stderr, "Server socket bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) == -1) {
        fprintf(stderr, "Listen failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %d\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen); // Is this blocking call?
    if (client_fd == -1) {
        fprintf(stderr, "Failed to accept client connection\n");
        exit(EXIT_FAILURE);
    }

    printf("Accepted client connection\n");

    char http_response[] = "HTTP/1.1 200 OK\r\n\r\n";
    write(client_fd, http_response, sizeof(http_response));
    // ssize_t valread;
    // while ((valread = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
    //    printf("Client: %s", buffer);
    //    memset(buffer, 0, sizeof(buffer)); // memset vs bzero
    //}

    close(server_fd);
    exit(0);
}
