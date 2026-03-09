#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char key[32];
	char value[128];
} Header;

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

	return -1;
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
		int written = sprintf(*buffer + currentPos, "%s=%s\r\n", headers[i].key, headers[i].value);

		currentPos += written;
	}

	memcpy(*buffer + currentPos, "\r\n", 2);
	currentPos += 2;

	(*buffer)[currentPos] = '\0';

 	return currentPos;
}
