#include <stdio.h>
#include <string.h>

#include "constants.h"

void parse_http_request_message(char *);

// data from all request line, headers and body will be probably needed in one single place, but for now I am moving validation
// of each to a separate function
static void validate_request_line(char *);

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
    for (int i = 0; i < 7; ++i) {
        printf("------------------------------------\n");
        parse_http_request_message(test_data[i]);
        printf("------------------------------------\n");
    }
}


void parse_http_request_message(char *message) {
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
    printf("%s\n", message);

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
                  ++i; // We know that next char is \n
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
    //printf("Index of first CRLF: %d\n", first_crlf_index);
    //printf("Index of empty line CRLF: %d\n", empty_line_crlf_index);

    // Now with found info print request line, headers and body
    printf("DEBUG: Printing request line:\n");
    for (int i = 0; i < first_crlf_index; ++i)
        putchar(message[i]);
    putchar('\n');

    if (first_crlf_index != empty_line_crlf_index) {
        printf("DEBUG: Printing headers:\n");
        for (int i = (first_crlf_index+2); i < empty_line_crlf_index; ++i)
            putchar(message[i]);
        putchar('\n');
    } else
        printf("DEBUG: Headers are not present\n");

    if (message[empty_line_crlf_index+4] != '\0') {
        printf("DEBUG: Printing body:\n");
        for (int i = (empty_line_crlf_index+4); message[i] != '\0'; ++i)
            putchar(message[i]);
        putchar('\n');
    } else
        printf("DEBUG: Body is not present\n");

    return;
}

static validate_request_line(char *) {
    return;
}
