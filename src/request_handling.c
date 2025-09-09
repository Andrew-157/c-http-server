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
