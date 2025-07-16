#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // contains read, write, etc.
#include <arpa/inet.h> // contains stuff for socket manipulations

#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        fprintf(stderr, "Server socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Created server socket successfully\n");

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        fprintf(stderr, "Setting of socket options failed\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) == -1) {
        fprintf(stderr, "Server socket bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) == -1) {
        fprintf(stderr, "Listen failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Server is listening on %d\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd == -1) {
        fprintf(stderr, "Failed to accept client connection\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Accepted client connection\n");

    struct sockaddr_in client_address;
    int client_address_len = sizeof(client_address);
    getpeername(client_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_address_len);

    printf("Client data: %d, %s, %d\n", client_address.sin_family, inet_ntoa(client_address.sin_addr), client_address.sin_port);

    // TODO: handle saving data to buffer by adjust index or something like that, idk
    //ssize_t valread = read(client_fd, buffer, BUFFER_SIZE); // Waaaait, what if BUFFER SIZE is not enough for what client sent me?
    //printf("Read from client socket %ld bytes of data: %s\n", valread, buffer);
    //memset(buffer, 0, strlen(buffer));

    // NOTE: This loop doesn't work
    ssize_t valread;
    // Most likely the problem is that read TRIES to read whole BUFFER_SIZE
    while ((valread = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        printf("Received from client %ld bytes with data: %s\n", valread, buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    // TODO: handle writing to socket until all the data was written
    char http_response[] = "HTTP/1.1 200 OK\r\n\\"
                           "Content-Type: text/html\r\n"
                           "Content-Length: 97\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "<!DOCTYPE html>"
                           "<html>"
                           "<head><title>Minimal HTML</title></head>"
                           "<body>Hello, world!</body>"
                           "</html>";
    ssize_t valwritten = write(client_fd, http_response, strlen(http_response));
    printf("Written bytes %ld vs expected written bytes %ld\n", valwritten, strlen(http_response));

    // Try to write to client socket in chunks - WORKS!
    //char first_part[] = "HTTP/1.1 200";
    //char second_part[] = " OK\r\n\r\n";
    //write(client_fd, first_part, strlen(first_part));
    //write(client_fd, second_part, strlen(second_part));

    // For some reason before client socket can be closed, to make client receive response properly, I either need to:
    // - sleep

    sleep(1);

    //read(client_fd, buffer, BUFFER_SIZE);
    //printf("Read from client socket again: %s\n", buffer);
    //memset(buffer, 0, strlen(buffer));

    close(server_fd);
    exit(0);
}
