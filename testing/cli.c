#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HOST "localhost"
#define PORT "8080"


int main(int argc, char **argv) {
    const char *help_text = "Usage: client [OPTIONS]\n\
A simple TCP client program.\n\
Options:\n\
  -n, --host <HOST>       Specify the host which to connect to - hostname or IPv4 IP address. If not provided \"%s\" will be used.\n\
  -p, --port <PORT>       Specify the port to which to connect to. If not provided \"%s\" will be used.\n\
  -h, --help              Display this help message and exit.\n\
";
    char *host = NULL;
    char *port = NULL;

    char *option;
    for (int i = 1; i < argc; i++) {
        option = argv[i];
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            printf(help_text, HOST, PORT);
            exit(0);
        } else if (strcmp(option, "-n") == 0 || strcmp(option, "--host") == 0) {
            // TODO: add validation whether host is a valid FQDN/IP?
            if (host != NULL) {
                printf("-n/--host option was provided twice.\n");
                exit(1);
            }
            if ((i+1) == argc) {
                printf("-n/--host option used without a value.\n");
                exit(1);
            }
            if (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0) {
                // TODO: add a separate help text for --host option
                printf("Help text for --host\n");
                exit(0);
            }
            host = argv[i++];
        } else if (strcmp(option, "-p") == 0 || strcmp(option, "--port") == 0) {
            // TODO: add validation whether port is integer?
            if (port != NULL) {
                printf("-p/--port option was provided twice.\n");
                exit(1);
            }
            if ((i+1) == argc) {
                printf("-p/--port option used without a value.\n");
                exit(1);
            }
            if (strcmp(argv[i+1], "-h") == 0 || strcmp(argv[i+1], "--help") == 0) {
                // TODO: add a separate help text for --port option
                printf("Help text for --port\n");
                exit(0);
            }
            port = argv[i++];
        }
    }

    if (host == NULL) host = HOST;
    if (port == NULL) port = PORT;

    printf("Host: %s, Port: %s\n", host, port);
}
