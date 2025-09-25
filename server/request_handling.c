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

struct rqstLine {
    char *method;
    int method_len;
    int max_method_len;
    int method_recvd;

    char *url;
    int url_len;
    int max_url_len;
    int url_recvd;

    char *protocol;
    int protocol_len;
    int max_protocol_len;
    int protocol_recvd;
};

#define RQST_LINE_INIT {NULL, 0, 0, NULL, 0, 0, NULL, 0, 0}

static void appendRqstLine(struct rqstLine *, const char *, int);

char * accept_rqst(int client_sockfd, int buffer_size) {
    char recv_msg[buffer_size];
    int bytes_received;

    int recved; // yeah, I know, great name, i don't care
    recved = 0;

    struct rqstLine rl = RQST_LINE_INIT;

    char c;
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

        for (int i = 0; i < bytes_received; i++) {
            c = recv_msg[i];
            if (c == ' ') {
                if (!rl.method_recvd) {
                    rl.method_recvd = 1;
                } else if (!rl.url_recvd) {
                    rl.url_recvd = 1;
                }
            } else if (c == '\r') {
                if (!rl.protocol_recvd) {
                   rl.protocol_recvd = 1;
                   recved = 1;
                }
            } else {
                char append_buf[2];
                snprintf(append_buf, sizeof(append_buf), "%c", c);
                if (!rl.method_recvd) {
                    appendRqstLine(&rl, append_buf, strlen(append_buf));
                } else if (!rl.url_recvd) {
                    appendRqstLine(&rl, append_buf, strlen(append_buf));
                } else if (!rl.protocol_recvd) {
                    appendRqstLine(&rl, append_buf, strlen(append_buf));
                }
            }
        }
    }

    log_message(INFO, "Method: %s, URL: %s, Protocol: %s\n", rl.method, rl.url, rl.protocol);
    free(rl.method);
    free(rl.url);
    free(rl.protocol);
    return NULL;
}

static void appendRqstLine(struct rqstLine *rl,  const char *s, int len) {
    char *new;
    if (!rl->method_recvd) {
        new = realloc(rl->method, rl->method_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->method_len], s, len);
        rl->method = new;
        rl->method_len += len;
    } else if (!rl->url_recvd) {
        new = realloc(rl->url, rl->url_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->url_len], s, len);
        rl->url = new;
        rl->url_len += len;
    } else if (!rl->protocol_recvd) {
        new = realloc(rl->protocol, rl->protocol_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->protocol_len], s, len);
        rl->protocol = new;
        rl->protocol_len += len;
    } else {
        // I don't know, free all memory?
    }
}
