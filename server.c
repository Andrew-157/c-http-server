#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define BUF_SIZE 500

static volatile sig_atomic_t terminate = 0;

// NOTE: this approach to signal handler doesn't currently work, because accept is blocking
static void signal_handler(int sig) {
	printf("Caught signal: %d\n", sig);
	terminate = 1;
}

int main(int argc, char **argv) {
	struct sigaction act = {
		.sa_handler = signal_handler,
		.sa_flags = SA_RESTART,
	};
	sigemptyset(&act.sa_mask);
	int signals[2] = {SIGINT, SIGTERM};
	for (int i = 0; i < sizeof(signals) / sizeof(int); i++) {
		int sig = signals[i];
		if (sigaction(sig, &act, NULL) == -1) {
			perror("[server]: sigaction:");
			exit(EXIT_FAILURE);
		}
	}

	int server_fd, client_fd;
	socklen_t peer_addrlen;
	struct addrinfo hints, *result, *rp;
	struct sockaddr_storage peer_addr;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	int gai;
	int yes = 1;
	if ((gai = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
		fprintf(stderr, "[server]: getaddrinfo: %s\n", gai_strerror(gai));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		server_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (server_fd == -1) {
			perror("[server]: socket:");
			continue;
		}

		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("[server]: setsockopt:");
			exit(EXIT_FAILURE);			
		}

		if (bind(server_fd, rp->ai_addr, rp->ai_addrlen) == -1) {
			close(server_fd);
			perror("[server: bind:]");
			continue;
		}

		break;
	}

	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, "[server]: Failed to bind\n");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, BACKLOG) == -1) {
		perror("[server]: listen:");
		exit(EXIT_FAILURE);
	}

	while (!terminate) {
		peer_addrlen = sizeof(peer_addr);
		client_fd = accept(server_fd, (struct sockaddr *)&peer_addr, &peer_addrlen);	
		if (client_fd == -1) {
			perror("[server]: accept:");
			continue;
		}

		int gnf;
		char host[NI_MAXHOST], service[NI_MAXSERV];
		if ((gnf = getnameinfo((struct sockaddr *)&peer_addr,
								peer_addrlen, host, NI_MAXHOST,
								service, NI_MAXSERV, NI_NUMERICSERV)) == 0) {
			printf("Accepted connection from %s:%s\n", host, service);
		} else {
			fprintf(stderr, "[server]: getnameinfo: %s\n", gai_strerror(gnf));
		}

		char buf[BUF_SIZE];
		ssize_t nread;
		if ((nread = recv(client_fd, buf, BUF_SIZE, 0)) > 0) {
			printf("Received %zd bytes from client:\n%s\n", nread, buf);
		} else if (nread == 0) {
			printf("Client closed the connection - ending communication\n");
			close(client_fd);
			continue;
		} else if (nread == -1) {
			perror("[server]: recv:");
			close(client_fd);
			continue;
		}

		char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
		send(client_fd, response, strlen(response), 0);
		close(client_fd);
	}

	if (client_fd && client_fd != -1) {
		close(client_fd);
	}

	close(server_fd);
}
