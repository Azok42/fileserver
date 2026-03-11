#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

typedef struct {
	char key[32];
	char value[128];
} Header;

// errors
#define FILE_IO_ERROR -3
#define HEADER_PARSE_ERROR -2

// globals
extern size_t CHUNK_SIZE;
extern char *DATA_PATH;

size_t parseHeaders(char *buffer, Header *headers, int maxHeaders) {
	size_t count = 0;
	char *copy = strdup(buffer);
	char *savePtr;

	char *header = strtok_r(copy, "\r\n", &savePtr);
	while (header && count < maxHeaders) {
		char *seperator = strchr(header, '=');
		if (seperator) {
			*seperator = '\0';
			strncpy(headers[count].key, header, 31);
			strncpy(headers[count].value, seperator+1, 127);
			count++;
		} header = strtok_r(NULL, "\r\n", &savePtr); }
	
	free(copy);
	return count;
}

char* getHeaderValue(Header *headers, int count, char *key) {
	for (int i = 0; i < count; i++) {
		if (strcmp(headers[i].key, key) == 0) return headers[i].value;
	}

	return NULL;
}

ssize_t setHeader(Header *headers, int count, char *key, char *value) {
	for (int i = 0; i < count; i++) {
		if (strcmp(headers[i].key, key) != 0)
			continue;

		strcpy(headers[i].value, value);
		return i;	
	}

	for (int i = 0; i < count; i++) {
		if (strcmp(headers[i].key, "") != 0)
			continue;

		strcpy(headers[i].key, key);
		strcpy(headers[i].value, value);
		return i;	
	}

	return HEADER_PARSE_ERROR;
}

ssize_t createHeader(char **buffer, Header *headers, size_t headerCount) {
	size_t count, bufferSize = 0;
	*buffer = NULL;

	for (int i = 0; i < headerCount; i++) {
		bufferSize += strlen(headers[i].key) + 1 + strlen(headers[i].value) + 2;
	}
	bufferSize += 3;

	*buffer = (char*) malloc(bufferSize);
	if (!(*buffer)) return -1; 

	size_t currentPos = 0;
	for (int i = 0; i < headerCount; i++) {
		if (strcmp(headers[1].key, "") == 0)
			continue;

		int written = sprintf(*buffer + currentPos, "%s=%s\r\n", headers[i].key, headers[i].value);

		currentPos += written;
	}

	memcpy(*buffer + currentPos, "\r\n", 2);
	currentPos += 2;

	(*buffer)[currentPos] = '\0';

 	return currentPos;
}

ssize_t sendFile(int socket, Header *headers, size_t headerSize) {
	char *path = getHeaderValue(headers, headerSize, "path");
	if (!path)
		return HEADER_PARSE_ERROR;

	char fData[CHUNK_SIZE];
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		printf("File doesn't exist");
		return FILE_IO_ERROR;
	}

	size_t nBytes = 0;
	ssize_t sent, totalSent = 0;

	while ((nBytes = fread(fData, sizeof(char), CHUNK_SIZE, fp)) > 0) {
		int offset = 0;
		while ((sent = send(socket, fData + offset, nBytes, 0)) > 0) {
			offset += sent;
			nBytes -= sent;
		}
		totalSent += offset;
	}

	fclose(fp);
	size_t sizeHeader = atol(getHeaderValue(headers, headerSize, "file-length"));
	if (totalSent != sizeHeader) {
		return FILE_IO_ERROR;
	}

	return totalSent;
}

void getFullPath(char *path, char *fullPath, int size) {
	snprintf(fullPath, size, "%s%s", DATA_PATH, path);
}

int writeFile(int socket, Header *headers, int count, char *overhang, size_t overhangSize) {
	char *path = getHeaderValue(headers, count, "path");
	char *size = getHeaderValue(headers, count, "file-length");

	if (!path || !size) return HEADER_PARSE_ERROR;

	char fullPath[256];
	getFullPath(path, fullPath, 256);

	FILE *fp = fopen(fullPath, "wb");
	if (!fp) return FILE_IO_ERROR;

	if (overhangSize > 0)
		fwrite(overhang, 1, overhangSize, fp);

	long remaining = atol(size) - overhangSize;
	char buffer[CHUNK_SIZE];
	int written = 0;
	while (remaining > 0) {
		ssize_t read = recv(socket, buffer, (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE, 0);
		if (read <= 0) break;
		written += fwrite(buffer, 1, read, fp);
		remaining -= read;
	}

	fclose(fp);
	written += overhangSize;
	if (written != atol(size))
		return FILE_IO_ERROR;

	return written;
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

int getFileLength(char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return FILE_IO_ERROR;

	fseek(fp, 0, SEEK_END);
	int contentSize = ftell(fp);
	fclose(fp);
	return contentSize;
}

int getDate(char *buffer, int bufferLength) {
	time_t now = time(NULL);
	strftime(buffer, bufferLength, "%d %b %Y %H:%M:%S", gmtime(&now));

	return 0;
}
