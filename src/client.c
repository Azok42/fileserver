#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"

int initConnection();

int main() {
	initConnection();

	return 0;
}

int initConnection() {
	int sockfd;

	struct sockaddr_in serverAddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (!sockfd) {
		printf("Socket creation failed");
		exit(0);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	serverAddr.sin_port = htons(SERVER_PORT);

	if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0 ) {
		printf("Connection failed");
		exit(1);
	}

	return 0;
}
