// file_transfer_common.h

#ifndef FILE_TRANSFER_COMMON_H
#define FILE_TRANSFER_COMMON_H

// Common system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

// Additional headers for server
#ifdef SERVER
#include <signal.h>
#include <sys/wait.h>
#endif

// Common macros
#define MAX_CLIENTS 100
#define CHUNK_SIZE 1024
#define MAX_FILENAME 256
#define SERVER_PORT 2206 // first 4 digits of my regNO
#define CLIENT_PORT 9122 // last 4 digits of my regNo
#define SERVER_IP "127.0.0.1"

// Folder paths
#define SERVER_INPUT_FOLDER "./shared_folder/"
#define SERVER_OUTPUT_FOLDER "./Server/"
#define CLIENT_INPUT_FOLDER "./client_folder/"
#define CLIENT_OUTPUT_FOLDER "./Client/"

// Log file path (for server)
#define LOG_FILE "./log/server_log.log"

#endif // FILE_TRANSFER_COMMON_H