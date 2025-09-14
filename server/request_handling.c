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

        char buff;
        for (int i = 0; i < bytes_received; i++) {
            buff = recv_msg[i];
            if (buff == '\r') {
                if ((i+1) < bytes_received) {
                    if (recv_msg[i+1] != '\n') {
                        // '\r' is valid as a standalone character only in the context of the body, which,
                        // for now at least, I don't think I am going to validate character by character
                        status_code = 400;
                        if (!rqst_line_received)
                            rspns_msg = "Carriage Return character was encountered outside of CRLF in request line\n";
                        else
                            rspns_msg = "Carriage Return character was encountered outside of CRLF in a header\n";
                        content_type = "text/html; charset=utf-8";
                        break;
                    } else {
                        // encountered CRLF - remember that you can encounter CRLF in the context of LWS in a header
                    }
                } else {
                    // There are 2 possible outcomes:
                    // - request is over and it is invalid
                    // - there is more to read, so we need to call recv again
                    // if we are going to call recv more, we need some state variable, like `char last_read_char;`
                }
            } else if (buff == '\n') {
                // it means request is definitely invalid, because if it was part of CRLF, we should have
                // handled it in the condition above
            } else if (buff == ' ') {
            } else if (buff == '\t') {
               // tab MAY be encountered in the context of LWS, which MAY be encountered in the context of a header
            } else {}
        }
    }
}
