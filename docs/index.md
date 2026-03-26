# Index file

## Purpose
The index file is supposed to give an overview of files to sync for both the server and client.
There, the path, hash and the File-ID, to reference the file with a simple number, is saved.

## Structure
This is the structure of the index file.

- Each line is a file

- At the start is the file-id
- Then seperated by a simple space there will be the relative path
- At the end there will be the hash.

#### Structure Overview

- [id] [path] [hash]
