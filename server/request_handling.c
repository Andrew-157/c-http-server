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


char * accept_rqst(int client_sockfd, int buffer_size) {
    char *method = NULL;
    char *uri = NULL;
    char *http_version = NULL;
    int rqst_line_received = 0;

    int status_code;
    char *rspns_msg;
    char *content_type;

    char recv_msg[buffer_size];
    int bytes_received;

    int recved; // yeah, I know, great name, i don't care
    recved = 0;

    while (!recved) {
        bytes_received = recv(client_sockfd, recv_msg, buffer_size, 0);

        // add case for when recv times out
        if (bytes_received == 0) {
            log_message(WARNING, "client closed the connection, ending communication\n");
            return NULL;
        } else if (bytes_received == -1) {
            log_message(ERROR, "error occurred while trying to read from client socket: %s, ending communication\n", strerror(errno));
            return NULL;
        }

        log_message(INFO, "Bytes received: %d\n", bytes_received);

    }
}
