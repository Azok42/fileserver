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

#define CRC32_FAST 0
#include "crc32.h"

typedef struct {
	char key[32];
	char value[128];
} Header;

// errors
#define HEADER_PARSE_ERROR -2
#define FILE_IO_ERROR -3
#define INDEX_ERROR -4

#define INDEX_RECORD_SIZE 256

// globals
extern size_t CHUNK_SIZE;
extern char *DATA_PATH;

void getFullPath(char *path, char *fullPath, int size);
int getLastID();
int getLastIDstr(char **result);
int createIndexFile();

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
		if (strcmp(headers[i].key, "") == 0)
			continue;

		int written = sprintf(*buffer + currentPos, "%s=%s\r\n", headers[i].key, headers[i].value);

		currentPos += written;
	}

	memcpy(*buffer + currentPos, "\r\n", 2);
	currentPos += 2;

	(*buffer)[currentPos] = '\0';

 	return currentPos;
}

ssize_t sendFile(int socket, char *fullPath, char *fileLength) {

	char fData[CHUNK_SIZE];
	FILE *fp = fopen(fullPath, "rb");
	if (!fp) {
		printf("file doesn't exist: %s\n", fullPath);
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
	if (totalSent != atoi(fileLength)) {
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

int addToIndex(char *path, uint32_t hash) {
	char fullPath[256];
	getFullPath("index", fullPath, 256);

	FILE *fp = fopen(fullPath, "ab");
	if (!fp) return FILE_IO_ERROR;

	int id = getLastID() + 1;

	char buffer[INDEX_RECORD_SIZE + 1];
	int written = snprintf(buffer, sizeof buffer, "%-15d %-230s %08x\n", id, path, hash);
	
	buffer[written] = ' ';
	buffer[INDEX_RECORD_SIZE - 1] = '\n';

	int writtenToFile = fwrite(buffer, 1, INDEX_RECORD_SIZE, fp);
	if (writtenToFile <= 0) return FILE_IO_ERROR;

	fclose(fp);
	return id;
}

int updateIndex(char *id, char *path, uint32_t hash) {
	char fullPath[256];
	getFullPath("index", fullPath, 256);

	FILE *fp = fopen(fullPath, "r+");
	if (!fp) return FILE_IO_ERROR;

	char line[INDEX_RECORD_SIZE];
	while (fread(line, 1, INDEX_RECORD_SIZE, fp) == INDEX_RECORD_SIZE) {

		if (strncmp(line, id, strlen(id)) != 0 || line[strlen(id)] != ' ')
			continue;

		fseek(fp, -INDEX_RECORD_SIZE, SEEK_CUR);

		char buffer[INDEX_RECORD_SIZE];
		memset(buffer, ' ', INDEX_RECORD_SIZE);

		snprintf(buffer, INDEX_RECORD_SIZE, "%-15s %-230s %08x", id, path, hash);
		buffer[INDEX_RECORD_SIZE - 1] = '\n';

		fwrite(buffer, 1, INDEX_RECORD_SIZE, fp);
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return INDEX_ERROR;
}

int getLastID() {
	char fullPath[256];
	getFullPath("index", fullPath, 256);

	FILE *fp = fopen(fullPath, "rb");
	if (!fp) return FILE_IO_ERROR;

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	if (fileSize < INDEX_RECORD_SIZE) {
		fclose(fp);
		return INDEX_ERROR;
	}

	fseek(fp, -INDEX_RECORD_SIZE, SEEK_END);
	char line[INDEX_RECORD_SIZE];
	if (fread(line, 1, INDEX_RECORD_SIZE, fp) != INDEX_RECORD_SIZE) {
		fclose(fp);
		return INDEX_ERROR;
	}
	fclose(fp);

	return (int) strtol(line, NULL, 10);
}

uint32_t getHashFromFile(char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp) return FILE_IO_ERROR;

	unsigned char buffer[CHUNK_SIZE];
	uint32_t hash = 0;
	size_t bytesRead;

	while ((bytesRead = fread(buffer, 1, sizeof buffer, fp)) > 0)
		hash = crc32(hash, buffer, bytesRead);

	fclose(fp);
	return hash;
}

int createIndexFile() {
	char fullPathToIndex[256];
	getFullPath("index", fullPathToIndex, 256);

	FILE *fp = fopen(fullPathToIndex, "wb");
	
	char buffer[INDEX_RECORD_SIZE + 1];
	int written = snprintf(buffer, sizeof buffer, "%-15s %-230s %08x\n", "0", "index", 0x000000);
	
	buffer[written] = ' ';
	buffer[INDEX_RECORD_SIZE - 1] = '\n';

	if (fwrite(buffer, 1, INDEX_RECORD_SIZE, fp) < INDEX_RECORD_SIZE)
		return FILE_IO_ERROR;
	
	fclose(fp);
	return 0;
}
