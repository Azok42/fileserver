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
#define CHUNK_SIZE 1024
#define DATA_PATH "data/"

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

	if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0 ) {
		printf("Connection failed");
		exit(1);
	}

	return 0;
}

int makeUploadHeader(char **buffer, char *filePath) {
	char dateBuf[30];
	getDate(dateBuf, sizeof(dateBuf));

	int fileLength = getFileLength(filePath);
	
	char hash[5] = "todo";

	int length = asprintf(buffer, 
			"date=%s\r\n"
			"type=upload\r\n"
			"file-length=%d\r\n"
			"path=%s\r\n"
			"hash=%s\r\n"
			"\r\n",
			dateBuf, fileLength, filePath, hash); 

	return length;
}

int makeDownloadHeader(char **buffer, int fileID) {
	char dateBuf[30];
	getDate(dateBuf, sizeof(dateBuf));

	int length = asprintf(buffer, 
			"date=%s\r\n"
			"type=download\r\n"
			"fileid=%d\r\n"
			"\r\n",
			dateBuf, fileID); 

	return length;
}

int makeSyncHeader(char **buffer) {
	char dateBuf[30];
	getDate(dateBuf, sizeof(dateBuf));

	char hash[5] = "todo";

	int length = asprintf(buffer, 
			"date=%s\r\n"
			"type=sync\r\n"
			"hash=%s\r\n"
			"\r\n",
			dateBuf, hash); 

	return length;
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
