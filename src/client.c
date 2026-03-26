#include "lib/lib.h"

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"

size_t CHUNK_SIZE = 1024;
char *DATA_PATH = "data-client/";

int initConnection();
int handleIncomingData(int socket);
int handleResponse(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize);

// test functions
void sendUpload(int sockfd);
void sendDownload(int sockfd);

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

	sendUpload(sockfd);
	
	close(sockfd);
	return 0;
}

void sendDownload(int sockfd) {
	Header headers[10] = {0};
	setHeader(headers, 10, "type", "download");

	char dateBuf[30];
	getDate(dateBuf, 30);
	setHeader(headers, 10, "date", dateBuf);

	setHeader(headers, 10, "fileid", "1");

	char *buffer;
	ssize_t headerCount = createHeader(&buffer, headers, 10);

	sendHeader(sockfd, buffer);

	handleIncomingData(sockfd);
}

void sendUpload(int sockfd) {
	Header headers[10] = {0};
	setHeader(headers, 10, "path", "testiger");
	setHeader(headers, 10, "fileid", "1");
	setHeader(headers, 10, "hash", "82a45b7f");

	char dateBuf[30];
	getDate(dateBuf, 30);
	setHeader(headers, 10, "date", dateBuf);

	setHeader(headers, 10, "type", "upload");

	char fullPath[256];
	getFullPath("testiger", fullPath, 256);

	char lengthBuffer[30];
	snprintf(lengthBuffer, 30, "%d", getFileLength(fullPath));
	setHeader(headers, 10, "file-length", lengthBuffer);

	char *buffer;
	ssize_t headerCount = createHeader(&buffer, headers, 10);

	sendHeader(sockfd, buffer);
	sendFile(sockfd, headers, headerCount);

	handleIncomingData(sockfd);
}

int handleIncomingData(int socket) {
	char *buffer = NULL;
	size_t totalRead = 0;
	char tmp[CHUNK_SIZE];
	ssize_t tmpRead;

	while ((tmpRead = recv(socket, tmp, sizeof tmp, 0)) > 0) {
		buffer = (char*) realloc(buffer, totalRead + tmpRead + 1);
		memcpy(buffer + totalRead, tmp, tmpRead);
		totalRead += tmpRead;
		buffer[totalRead] = '\0';

		char *headerEnd = strstr(buffer, "\r\n\r\n");
		if (!headerEnd)
			continue;

		*headerEnd = '\0';
		char *dataStart = headerEnd + 4;
		size_t overhangSize = totalRead - (dataStart - buffer);

		Header headers[10] = {0};
		int headerCount = parseHeaders(buffer, headers, 10);

		int res = handleResponse(socket, headers, headerCount, dataStart, overhangSize);

		free(buffer);
		return 0;
	}

	free(buffer);
	return -1;
}

int handleResponse(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize) {
	char *status = getHeaderValue(headers, headerCount, "status");
	if (!status) return HEADER_PARSE_ERROR;

	if (strcmp(status, "success") == 0) {
		char *fileID = getHeaderValue(headers, headerCount, "fileid");

		if (fileID) { // upload
			printf("%s\n", fileID);

		} else { // download
			char *fileSize = getHeaderValue(headers, headerCount, "file-length");
			char *hash = getHeaderValue(headers, headerCount, "hash");
			setHeader(headers, headerCount+1, "path", "test");

			if (!fileSize || !hash) return HEADER_PARSE_ERROR;

			writeFile(socket, headers, headerCount, overhang, overhangSize);
		}

	} if (strcmp(status, "failure") == 0) {
		printf("Failure response");
	} else
		return -1;

	return 0;
}
