#define _GNU_SOURCE
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig) {
	printf("Caught signal: %d\n", sig);
}

int main(void)
{
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

    struct pollfd pfds[1]; // More if you want to monitor more

    pfds[0].fd = 0;          // Standard input
    pfds[0].events = POLLIN; // Tell me when ready to read

    // If you needed to monitor other things, as well:
    //pfds[1].fd = some_socket; // Some socket descriptor
    //pfds[1].events = POLLIN;  // Tell me when ready to read

    printf("Hit RETURN or wait 2.5 seconds for timeout\n");

	sigset_t mask;
	sigemptyset(&mask);

    int num_events = ppoll(pfds, 1, NULL, &mask);

    if (num_events == 0) {
        printf("Poll timed out!\n");
    } else {
        int pollin_happened = pfds[0].revents & POLLIN;

        if (pollin_happened) {
            printf("File descriptor %d is ready to read\n",
                    pfds[0].fd);
        } else {
            printf("Unexpected event occurred: %d\n",
                    pfds[0].revents);
        }
    }

    return 0;
}
