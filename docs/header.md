# Header

This is a small documentation of the headers used in the protocol. They are heavly based on HTTP headers (actually the whole protocol is just HTTP with some small changes, but i don't care)

## General

First the header name has to be specified and speparated with a '=' without spaces the value.
After every header/values pair there should be a "\r\n" zu signal the end of the header.
And just like in HTTP at the end of the whole header the has to be a double "\r\n".

## Requests
These are the messages the server recieves from the client.

- date: The "Day Month Year Hour:Minutes:Seconds" of the time the request was made
- type: upload/download/index
    - upload: Upload a file to the server
    - download: Download a file from the server
    - index: Index the index file (if not already the same)
- hash: Only for Upload & Index; the hash of the file
- file-length: Only for Upload; the file size in bytes
- path: Only for Upload; the relative path of the file
- fileid: Only for Download and Upload (is -1 if file is new); the File-ID of the file specified in the index file

## Response Header
These are the messages the server makes to respond to the client requests.

- date: The "Day Month Year Hour:Minutes:Seconds" of the time the response was made
- status: success/failure
    - success: means the request is valid and being processed
    - failure: means something went wrong or the index file is already sync'd
- fileid: Only for Upload responses; returns the fileID of the newly uploaded file
- file-length: Only for Download & Index Responses; the file size of the file in bytes
- path: Only for Download; the relative path of the file
- hash: Only for Download & Index Repsonses; the hash of the file
