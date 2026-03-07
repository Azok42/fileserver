#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
int sendFile(int socket, char *path);
int getHeaderAndFile(int socket, char **buffer);
int extractDataFromBuffer(char *buffer, char *date, char *type, char *hash, int *length, char **path);

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

		char *buffer;
		getHeaderAndFile(clientSocket, &buffer);
		printf("%s\n", buffer);

		free(buffer);
		exit(0);
	}

	close(clientSocket);
	return 0;
}

int getHeaderAndFile(int socket, char **buffer) {
	char bufferTmp[CHUNK_SIZE];
	ssize_t receivedBytes;
	*buffer = NULL;

	while ((receivedBytes = recv(socket, bufferTmp, CHUNK_SIZE - 1 , 0)) > 0) {
		bufferTmp[receivedBytes++] = '\0';

		if (!(*buffer)) {
			*buffer = (char *) malloc(receivedBytes);
			strcpy(*buffer, bufferTmp);
		} else {
			*buffer = realloc(*buffer, strlen(*buffer) + receivedBytes);
			strcat(*buffer, bufferTmp);
		}

		char *headerEnd = strstr(*buffer, "\r\n\r\n");
		if (!headerEnd)
			continue;

		*headerEnd = '\0';

		char date[20], type[9], hash[10];
		char *path;
		int fileSize;
		extractDataFromBuffer(*buffer, date, type, hash, &fileSize, &path);

		if (!strcmp(type, "upload"))
			return 0;

		FILE *fp = fopen(path, "wb");
		if (!fp) {
			return -1;
		}

		int bytesToWrite = CHUNK_SIZE - (headerEnd - *buffer) - 5;
		fwrite(headerEnd + 4, bytesToWrite, 1, fp);

		char writeBuffer[CHUNK_SIZE];
		int readBytes = 0;
		while ((readBytes = read(socket, writeBuffer, fmin(CHUNK_SIZE, bytesToWrite))) > 0) {
			fwrite(writeBuffer, strlen(writeBuffer), 1, fp);
			bytesToWrite -= readBytes;
		}

		fclose(fp);
		free(path);
		return 0;
	}

	return 0;
}

int extractDataFromBuffer(char *buffer, char *date, char *type, char *hash, int *length, char **path) {
	char *dateLoc = strstr(buffer, "date=");
	if (!dateLoc)
		return -1;
	strncpy(date, dateLoc + 5, 20);

	char *typeLoc = strstr(buffer, "type=");
	if (!typeLoc)
		return -1;
	strncpy(type, typeLoc + 5, 9);

	char *hashLoc = strstr(buffer, "hash=");
	if (!hashLoc)
		return -1;
	strncpy(hash, hashLoc + 5, 10);

	char *sizeLoc = strstr(buffer, "file-length=");
	if (!sizeLoc)
		return -1;
	*length = strtol(sizeLoc + 12, NULL, 10);

	char *pathLoc = strstr(buffer, "path=");
	if (!pathLoc)
		return -1;
	int pathLength = strstr(pathLoc, "\r\n") - pathLoc - 5;
	*path = (char *) malloc(pathLength + strlen(DATA_PATH));
	strcpy(*path, DATA_PATH);
	strncat(*path, pathLoc + 5, pathLength);

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
