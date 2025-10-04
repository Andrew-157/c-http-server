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

#include "consts.h"
#include "../tools/logger.h"
#include "request_handling.h"

struct rqstLine {
    char *method;
    int method_len; // is '\0' counted?
    int max_method_len;
    int method_recvd;

    char *uri;
    int uri_len;
    int max_uri_len;
    int uri_recvd;

    char *protocol;
    int protocol_len;
    int max_protocol_len;
    int protocol_recvd;
};

struct resp {
    int resp_composed;
    int status_code;
    char *content_type;
    char *body;
};

static void appendRqstLine(struct rqstLine *, const char *, int);
static void validateRqstLine(struct rqstLine *, struct resp *);
static void populateResp(struct resp *, int, char *, char *);
static char *read_template(char *);

void accept_rqst(int client_sockfd, int buffer_size) {
    char recv_msg[buffer_size];
    int bytes_received;

    int recved;
    recved = 0;

    struct rqstLine rl;
    rl.method = NULL;
    rl.method_len = 0;
    rl.max_method_len = 10; // length of the longest supported method
    rl.method_recvd = 0;
    rl.uri = NULL;
    rl.uri_len = 0;
    rl.max_uri_len = 25; // dynamically calculated based on the resources served
    rl.uri_recvd = 0;
    rl.protocol = 0;
    rl.protocol_len = 0;
    rl.max_protocol_len = 10; // length of the longest supported protocol
    rl.protocol_recvd = 0;

    struct resp rp;
    rp.resp_composed = 0;

    int rqst_line_recved;

    char c;
    char prev_c;
    while (!recved) {
        int i = 0;
        bytes_received = recv(client_sockfd, recv_msg, buffer_size, 0);
        if (bytes_received == 0) {
            log_message(WARNING, "client closed the connection, ending communication\n");
            close(client_sockfd); // not sure if it is a good idea to close the socket here, but let it be like this for now
            return; // exit the function somehow
        }
        else if (bytes_received == -1) {
            // We cannot know if timeout, for example, occurred due to client not sending anything anymore
            // and us trying to read or because of faulty connection, so to make matters simple, at least for now,
            // let's just send assume error on the server side
            populateResp(&rp, INTERNAL_SERVER_ERROR, TEXT_PLAIN, "server failed to read client request\r\n");
            break;
        }

        log_message(DEBUG, "Bytes received: %d\n", bytes_received);

        if (!rqst_line_recved) {
             while (i < bytes_received) {
                 c = recv_msg[i];
                 if (c == ' ') {
                     if (!rl.method_recvd && rl.method_len > 0) {
                         rl.method_recvd = 1;
                     } else if (!rl.uri_recvd && rl.uri_len > 0) {
                         rl.uri_recvd = 1;
                     } else {
                         // i think rfc said that request line can start with space or crlf, but im not sure
                         populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "' ' character encountered not as a separator between method, uri and protocol\r\n");
                         break;
                     }
                     prev_c = ' ';
                 } else if (c == '\r') {
                     if ((i+1) > (bytes_received-1)) {
                         prev_c = '\r';
                         break;
                     } else if (recv_msg[i+1] != '\n') {
                         populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "'\\r' character encountered not as part of CRLF in Request Line\r\n");
                         break;
                     } else {
                         i++;
                         rl.protocol_recvd = 1;
                         rqst_line_recved = 1;
                         validateRqstLine(&rl, &rp);
                         break;
                     }
                 } else if (c == '\n') {
                     if (prev_c != '\r') {
                         populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "'\\n' character encountered not as part of CRLF in Request Line\r\n");
                         break;
                     }
                     rl.protocol_recvd = 1;
                     rqst_line_recved = 1;
                     validateRqstLine(&rl, &rp);
                     break;
                 } else if (c == '\t') {
                     // i don't think tab is valid anywhere in the request line
                     populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "'\\t' character encountered in request line\r\n");
                     break;
                 } else {
                     if (!rl.method_recvd && (rl.method_len+1) > rl.max_method_len) {
                         populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "method is too long\r\n");
                         break;
                     } else if (!rl.uri_recvd && (rl.uri_len+1) > rl.max_uri_len) {
                         populateResp(&rp, URI_TOO_LONG, TEXT_PLAIN, "URI is too large - server refused to process it.\r\n");
                         break;
                     } else if (!rl.protocol_recvd && (rl.protocol_len+1) > rl.max_protocol_len) {
                         populateResp(&rp, BAD_REQUEST, TEXT_PLAIN, "protocol is too long\r\n");
                         break;
                     }
                     char append_buf[2];
                     snprintf(append_buf, sizeof(append_buf), "%c", c);
                     appendRqstLine(&rl, append_buf, strlen(append_buf));
                     prev_c = c;
                 }
                 i++;
             }
        }

        if (rp.resp_composed) {
            // response was composed - send it and exit
            char response[200];
            snprintf(response, sizeof(response), "HTTP/1.1 %d Phrase\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
                rp.status_code, rp.content_type, strlen(rp.body), rp.body);
            send(client_sockfd, response, strlen(response), 0);
        }
    }

    close(client_sockfd);
    free(rl.method);
    free(rl.uri);
    free(rl.protocol);
}

static void appendRqstLine(struct rqstLine *rl,  const char *s, int len) {
    char *new;
    if (!rl->method_recvd) {
        new = realloc(rl->method, rl->method_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->method_len], s, len);
        rl->method = new;
        rl->method_len += len;
    } else if (!rl->uri_recvd) {
        new = realloc(rl->uri, rl->uri_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->uri_len], s, len);
        rl->uri = new;
        rl->uri_len += len;
    } else {
        new = realloc(rl->protocol, rl->protocol_len + len);
        if (new == NULL) return;
        memcpy(&new[rl->protocol_len], s, len);
        rl->protocol = new;
        rl->protocol_len += len;
    }
}

// if request line is valid - does nothing
// else - sets erroneous response
static void validateRqstLine(struct rqstLine *rl, struct resp *rp) {
    // validate and compose a response;
    char buf[100];
    snprintf(buf, sizeof(buf), "Method: %s\nURI: %s\nProtocol: %s\r\n", rl->method, rl->uri, rl->protocol);
    populateResp(rp, OK, TEXT_PLAIN, buf);
}

static void populateResp(struct resp *rp, int status_code, char *content_type, char *body) {
    rp->status_code = status_code;
    rp->content_type = content_type;
    rp->body = body;
    rp->resp_composed = 1;
}

static char *read_template(char *template_path) {
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
