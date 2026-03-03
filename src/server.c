#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#define PORT 8080
#define ADDR "127.0.0.1" // TODO: change that for release
#define MAX_CLIENTS 10
#define CHANK_SIZE 1024

int initConnection();

// signals
void handleSigchld(int sig);

int main() {
	initConnection();

	return EXIT_SUCCESS;
}

int initConnection() {
	int sockfd, ret, clientSocket;
	struct sockaddr_in serverAddr, cliAddr;
	socklen_t addrSize = 0;
	pid_t childPID;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Error at socket creation!\n");
		exit(1);
	}

	int reuse = 1;
	int result = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));
	if (result < 0) {
		printf("Error at socket option setting\n");
		exit(1);
	}

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr(ADDR);

	ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		printf("Error at binding\n");
	}

	if (listen(sockfd, MAX_CLIENTS) == 0)
		printf("Listening for Connections\n");

	signal (SIGCHLD, handleSigchld);

	while (1) {
		clientSocket = accept(sockfd, (struct sockaddr*)&cliAddr, &addrSize);
		if (clientSocket < 0) {
			printf("Error at accept");
			exit(1);
		}

		if ((childPID = fork()) > 0)
			continue;

		close(sockfd);



		exit(0);
	}

	close(clientSocket);
	return 0;
}

void handleSigchld(int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
