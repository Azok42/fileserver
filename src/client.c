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
int getDate(char *buffer, int bufferLength);
int getFileLength(char *path);

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
