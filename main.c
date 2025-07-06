#include <stdio.h>


void parse_http_request_message(char *);
void print_section(char *, int);

int main() {
    char http_request_message[1000] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    //printf("%s", http_request_message);
    parse_http_request_message(http_request_message);
}


void parse_http_request_message(char *message) {
    /*
     * Assumptions for now:
     * - Request line can contain more than one SP
     * - HTTP request without body must have one more CRLF, and the one with body may not have one
     * */
    // Trying to create some algorithm:
    // 1. Everything before the first CRLF is a Request line
    char *p = message;
    for (int i = 0; message[i] != '\0'; i++) {
        if (message[i] == '\r' && message[i+1] == '\n') {
            print_section(p, i-1);
            break;
        }
    }
}

void print_section(char *p, int end) {
    for (int i = 0; i <= end; ++i)
        putchar(*(p + i));
    putchar('\n');
}
