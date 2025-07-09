#include <stdio.h>
#include <string.h>

#include "constants.h"


void parse_http_request_message(char *);


int main() {
    char *test_data[10] = {
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n", // valid
        "GET /index.html HTTP/1.1\r\n", // invalid
        "POST /users HTTP/2.0\r\n\r\nSome Body, idk", // valid
        "", // invalid
        "PATCH /edit HTTP/1.1\r\n\r\nadjust=data", // valid
        "GET /items/1 HTTP/1.1\r\nHost: site.com\r\n\r\n", // valid
        "POST /users HTTP/1.1\r\nHost: mywebsite.auth.com\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n{\"user\":\n  {  \n\"password\": 1234\n}\n}"
    };
}


void parse_http_request_message(char *message) {
    int first_crlf_index = -1;
    int empty_line_crlf_index = -1;
    int message_len = strlen(message);

    for (int i = 0; message[i] != '\0'; ++i) {
        if (message[i] == '\r') {
            if ((i+1) > message_len || message[i+1] != '\n') {
                printf("ERROR: '\\r' is not followed by '\\n'\n");
                return;
            } else if (message[i+1] == '\n') {
                if (first_crlf_index < 0)
                    first_crlf_index = i;
                if ((i+3) <= message_len && message[i+2] == '\r' && message[i+3] == '\n') {
                    empty_line_crlf_index = i;
                    break;
                } else
                  ++i;
            }
        }
    }

    if (first_crlf_index < 0) {
        printf("ERROR: CRLF is not found in message\n");
        return;
    }
    if (empty_line_crlf_index < 0) {
        printf("ERROR: message doesn't contain double CRLF\n");
        return;
    }

    return;
}

