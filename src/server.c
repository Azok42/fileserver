#include "lib/lib.h"
#include <signal.h>

#define PORT 8080
#define ADDR "127.0.0.1" // TODO: change that for release
#define MAX_CLIENTS 10

size_t CHUNK_SIZE = 1024;
char *DATA_PATH = "data-server/";

int initConnection();
int handleRequest(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize);
int handleConnection(int socket);

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

int handleRequest(int socket, Header *headers, int headerCount, char *overhang, size_t overhangSize) {
	char *type = getHeaderValue(headers, headerCount, "type");
	if (!type) return -1;

	if (strcmp(type, "upload") == 0) {
		ssize_t result = writeFile(socket, headers, headerCount, overhang, overhangSize);

		Header responseHeaders[10] = {0};
		setHeader(responseHeaders, 10, "status", (result > 0) ? "success" : "failure");

		char dateBuf[30];
		getDate(dateBuf, sizeof dateBuf);
		setHeader(responseHeaders, headerCount, "date", dateBuf);

		char *fileID = "1";
		setHeader(responseHeaders, 10, "fileid", fileID);

		char *buffer;
		ssize_t responseHeaderCount = createHeader(&buffer, responseHeaders, 10);

		sendHeader(socket, buffer);

	} if (strcmp(type, "download") == 0) {
		Header responseHeaders[10] = {0};
		setHeader(responseHeaders, headerCount, "status", "success");

		char dateBuf[30];
		getDate(dateBuf, sizeof dateBuf);
		setHeader(responseHeaders, headerCount, "date", dateBuf);

		char lengthBuffer[30];
		char fullPath[256];
		getFullPath("test", fullPath, 256);
		snprintf(lengthBuffer, 30, "%d", getFileLength(fullPath));
		setHeader(responseHeaders, 10, "file-length", lengthBuffer);

		setHeader(responseHeaders, 10, "hash", "todo");

		setHeader(responseHeaders, 10, "path", "test");

		char *buffer;
		ssize_t responseHeaderCount = createHeader(&buffer, responseHeaders, 10);

		sendHeader(socket, buffer);
		sendFile(socket, responseHeaders, responseHeaderCount);
	} else
		return -1;

	return 0;
}

int handleConnection(int socket) {
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

		handleRequest(socket, headers, headerCount, dataStart, overhangSize);

		free(buffer);
		return 0;
	}

	free(buffer);
	return -1;
}

void handleSigchld(int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
