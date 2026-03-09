#include <stdio.h>
#include "lib/lib.h"
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
#define CHUNK_SIZE 1024
#define DATA_PATH "data-client/"

int initConnection();
int getDate(char *buffer, int bufferLength);
int getFileLength(char *path);
int sendFile(int socket, char *path);
int sendHeader(int socket, char *buffer);
int makeSyncHeader(char **buffer);
int makeDownloadHeader(char **buffer, int fileID);
int makeUploadHeader(char **buffer, char *filePath);

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

	if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof serverAddr) != 0 ) {
		printf("Connection failed");
		exit(1);
	}

	Header headers[10] = {0};
	setHeader(headers, 10, "path", "test");
	
	char dateBuf[30];
	getDate(dateBuf, 30);
	setHeader(headers, 10, "date", dateBuf);
	
	setHeader(headers, 10, "type", "upload");

	char lengthBuffer[30];
	snprintf(lengthBuffer, 30, "%d", getFileLength("test"));
	setHeader(headers, 10, "file-length", lengthBuffer);

	char *buffer;
	createHeader(&buffer, headers, 4);

	sendHeader(sockfd, buffer);
	sendFile(sockfd, "test");

	close(sockfd);
	return 0;
}

int sendHeader(int socket, char *buffer) {
	int size = strlen(buffer);

	int sent, offset = 0;
	while ((sent = send(socket, buffer + offset, size, 0)) > 0) {
		offset += sent;
		size -= sent;
	}

	free(buffer);
	return 0;
}

int sendFile(int socket, char *path) {
	char fData[CHUNK_SIZE];
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		printf("File doesn't exist");
		return 1;
	}

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

int getDate(char *buffer, int bufferLength) {
	time_t now = time(NULL);
	strftime(buffer, bufferLength, "%d %b %Y %H:%M:%S", gmtime(&now));

	return 0;
}

int getFileLength(char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return 1;

	fseek(fp, 0, SEEK_END);
	int contentSize = ftell(fp);
	fclose(fp);
	return contentSize;
}
