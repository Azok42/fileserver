# File Server

This is a small file server using a self made protocol (not much). It includes a CLI server and CLI client. It probably isn't going to be any good/fast/secure, but it's just for education.

> Disclaimer: This project was developed strictly for educational purposes. It is not intended for production use, as it lacks the optimizations, concurrency handling, and security features (like encryption or robust authentication) found in established protocols like FTP or HTTP.

## How to use (Linux)
Currently only supports *localhost*. I'm going to add a config file for server and client later

- **Clone**:
```bash
git clone https://github.com/Azok42/fileserver # clone the repo
cd fileserver
```

- **Compile**:
```bash
mkdir build
gcc src/server.c -o build/server
gcc src/client.c -o build/client
```

- **Pray to Holy GabeN everything works**:
```bash
# server
build/server

# client
build/client
```

## How to use (Windows)

- WSL
- Docker
- make a windows port (good luck)
