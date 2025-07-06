#include <stdio.h>
#include <string.h>

int parse_http_request_message(char *);

int main() {
    char test_data[10][1000] = {
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n",
        "GET /index.html HTTP/1.1\r\n",
        "POST /users HTTP/2.0\r\n\r\nSome Body, idk"
    };
    for (int i = 0; i < 3; ++i)
        parse_http_request_message(test_data[i]);
}


int parse_http_request_message(char *message) {
    /*
     * Assumptions for now:
     * - Request line can contain more than one SP
     * - HTTP request without body must have one more CRLF, and the one with body may not have one
     * */
    // Trying to create some algorithm:
    // 1. Everything before the first CRLF is a Request line
    // 2. Everything after CRLF x 2 is a body

    // NOTE: Get indices of first CRLF and CRLF that is followed by one more CRLF, this way
    // the request message can be divided
    int first_crlf_index = -1;
    int empty_line_crlf_index = -1;
    int message_len = strlen(message);

    for (int i = 0; message[i] != '\0'; ++i) {
        if (message[i] == '\r') {
            if ((i+1) > message_len || message[i+1] != '\n') {
                printf("ERROR: '\\r' is not followed by '\\n'\n");
                return 1;
            } else if (message[i+1] == '\n') {
                if (first_crlf_index < 0)
                    first_crlf_index = i;
                if ((i+3) <= message_len && message[i+2] == '\r' && message[i+3] == '\n') {
                    empty_line_crlf_index = i;
                    break;
                } else
                  ++i; // We know that next char is \n
            }
        }
    }

    if (first_crlf_index < 0) {
        printf("ERROR: CRLF is not found in message\n");
        return 2;
    }
    if (empty_line_crlf_index < 0) {
        printf("ERROR: message doesn't contain double CRLF\n");
        return 3;
    }

    printf("Index of first CRLF: %d\n", first_crlf_index);
    printf("Index of empty line CRLF: %d\n", empty_line_crlf_index);
    return 0;
}
