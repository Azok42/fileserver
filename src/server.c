#include "lib/lib.h"
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
#define CHUNK_SIZE 1024
#define DATA_PATH "data-server/"

int initConnection();
int handleConnection(int socket);
int writeFile(int socket, Header *headers, int count, char *overhang, size_t overhangSize);
int handleRequest(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize);
int sendFile(int socket, char *path);

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
	int result = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof reuse);
	if (result < 0) {
		printf("Error at socket option setting\n");
		exit(1);
	}

	memset(&serverAddr, '\0', sizeof serverAddr);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr(ADDR);

	ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof serverAddr);
	if (ret < 0) {
		printf("Error at binding\n");
	}

	if (listen(sockfd, MAX_CLIENTS) == 0)
		printf("Listening for Connections\n");

	signal(SIGCHLD, handleSigchld);

	while (1) {
		clientSocket = accept(sockfd, (struct sockaddr*)&cliAddr, &addrSize);
		if (clientSocket < 0) {
			printf("Error at accept");
			exit(1);
		}

		if ((childPID = fork()) > 0)
			continue;
		close(sockfd);

		handleConnection(clientSocket);

		exit(0);
	}

	close(clientSocket);
	return 0;
}

int handleConnection(int socket) {
	char *buffer = NULL;
	size_t totalRead = 0;
	char tmp[CHUNK_SIZE];
	ssize_t tmpRead;

	while ((tmpRead = recv(socket, tmp, sizeof tmp, 0)) > 0) {
		buffer = realloc(buffer, totalRead + tmpRead + 1);
		memcpy(buffer + totalRead, tmp, tmpRead);
		totalRead += tmpRead;
		buffer[totalRead] = '\0';

		char *headerEnd = strstr(buffer, "\r\n\r\n");
		if (!headerEnd)
			continue;

		*headerEnd = '\0';
		char *dataStart = headerEnd + 4;
		size_t overhangSize = totalRead - (dataStart - buffer);

		Header headers[10];
		int headerCount = parseHeaders(buffer, headers, 10);

		handleRequest(socket, headers, headerCount, dataStart, overhangSize);

		free(buffer);
		return 0;
	}

	free(buffer);
	return -1;
}

int handleRequest(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize) {
	char *type = getHeaderValue(headers, headerCount, "type");
	if (!type) return -1;

	if (strcmp(type, "upload") == 0)
		writeFile(socket, headers, headerCount, overhang, overhangSize);
	else
		return -2;

	return 0;
}

int writeFile(int socket, Header *headers, int count, char *overhang, size_t overhangSize) {
	char *path = getHeaderValue(headers, count, "path");
	char *size = getHeaderValue(headers, count, "file-length");

	if (!path || !size) return -1;

	char fullPath[256];
	snprintf(fullPath, sizeof fullPath, "%s%s", DATA_PATH, path);

	FILE *fp = fopen(fullPath, "wb");
	if (!fp) return -1;

	if (overhangSize > 0)
		fwrite(overhang, 1, overhangSize, fp);

	long remaining = atol(size) - overhangSize;
	char buffer[CHUNK_SIZE];
	while (remaining > 0) {
		ssize_t read = recv(socket, buffer, (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE, 0);
		if (read <= 0) break;
		fwrite(buffer, 1, read, fp);
		remaining -= read;
	}

	fclose(fp);
	return 0;
}

int sendFile(int socket, char *path) {
	char *fData[CHUNK_SIZE];
	FILE *fp = fopen(path, "rb");

	size_t nBytes = 0;
	int sent = 0;

	while ((nBytes = fread(fData, sizeof(char), CHUNK_SIZE, fp)) > 0) {
		int offset = 0;

		while ((sent = send(socket, fData + offset, nBytes, 0)) > 0) {
			offset += sent;
			nBytes -= sent;
		}
	}

	fclose(fp);

	return 0;
}

void handleSigchld(int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
