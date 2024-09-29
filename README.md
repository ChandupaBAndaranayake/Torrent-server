# File Transfer System

This project implements a simple file transfer system with a server and client application.

# Compilation and Running Instructions

## Prerequisites

Ensure you have the following files in your working directory:
- `server.c`
- `client.c`
- `file_transfer_common.h`

Also, make sure you have the necessary folders created:
- `./shared_folder/`
- `./Server/`
- `./client_folder/`
- `./Client/`
- `./log/`

## File Generation Script -> execute.sh

#### Purpose
This script generates 50 random binary files of 10MB each in both the `shared_folder`,`client_folder`,`Server` and `Client` directories.If the parent directory './' doesnt not containe `shared_folder` and `client_folder`, run this script and script with create the necessary directories in the project directory.

##### Give Execute Permission

To make the script executable, run the following command:

```bash
chmod +x generate_files.sh
```

##### Execute the Script

The script will create the `shared_folder` and `client_folder` directories (if they don't already exist) and will generate 50 random binary files (each 10MB) in both directories.

Run the script by using:

```bash
./generate_files.sh
```

#### Verfiy

Check the respective directories (`shared_folder` and `client_folder`) to verify that the files have been created:

```bash
ls -lh ./shared_folder/
ls -lh ./client_folder/
```
## Compilation
### Compile the Server

To compile the server program, use the following command:

```bash
gcc -o server server.c
```

The `-DSERVER` flag defines the `SERVER` macro, which is used in `file_transfer_common.h` to include server-specific headers.

### Compile the Client

To compile the client program, use the following command:

```bash
gcc -o client client.c
```

## Running the Programs

### Server

To run the server, use the following command:

```bash
./server
```

The server will start and listen for incoming connections on port 9122 (as defined in `file_transfer_common.h`).

### Client

To run the client, use a loopback address if the system is in the local machine (127.0.0.x[1-255]) use the following command:

```bash
./client <IP addresss>
```

The client will connect to the server and present a menu with options to download, upload, or resume downloading a file.

## Usage

1. Start the server first.
2. Then, in a separate terminal, start the client.
3. Follow the prompts in the client program to upload, download, or resume downloading files.

## Troubleshooting

- If you encounter "Address already in use" errors when starting the server, ensure no other instance of the server is running, or change the `SERVER_PORT` in `file_transfer_common.h`.
- If the client fails to connect, ensure the server is running and that `SERVER_IP` in `file_transfer_common.h` is set correctly.
- For permission errors, check that your user has read/write access to all the necessary folders.

## Core Functionalities

### Server

The server program provides the following core functionalities:

1. **File Listing**: Sends a list of available files to the client upon request.
2. **File Upload**: Receives files from clients and stores them in a designated server directory.
3. **File Download**: Sends requested files to clients.
4. **Resume Transfer**: Supports resuming file transfers from a specified offset.
5. **Concurrent Connections**: Handles multiple client connections simultaneously using forking.
6. **Logging**: Maintains a log of all file transfer activities, including client IP, port, filename, and transfer status.

### Client

The client program offers the following core functionalities:

1. **File Download**: Requests and receives files from the server.
2. **File Upload**: Sends files to the server for storage.
3. **Resume Download**: Allows resuming interrupted downloads from the last received byte.
4. **File Listing**: Requests and displays the list of available files on the server.
5. **Progress Display**: Shows real-time progress of file transfers.
6. **User Interface**: Provides a simple text-based menu for user interaction.

## Folder Structure and Purpose

The file transfer system uses specific folders for input and output on both the server and client sides. Understanding these folders is crucial for proper system operation:

### Server Folders

1. **Server Input Folder** (`./shared_folder/`):
   - Purpose: This folder contains files that are available for clients to download.
   - Usage: The server reads from this folder when a client requests a file download.

2. **Server Output Folder** (`./Server/`):
   - Purpose: This folder is where files uploaded by clients are stored.
   - Usage: When a client uploads a file, the server saves it in this folder.

### Client Folders

1. **Client Input Folder** (`./client_folder/`):
   - Purpose: This folder contains files that the client can upload to the server.
   - Usage: When a user chooses to upload a file, the client reads from this folder.

2. **Client Output Folder** (`./Client/`):
   - Purpose: This folder is where files downloaded from the server are saved.
   - Usage: When a user downloads a file from the server, it is saved in this folder.

### Important Notes

- Ensure that these folders exist and have the correct read/write permissions before running the programs.
- The server and client must have access to their respective input and output folders for the system to function properly.
- When using the system, be aware of which folder serves which purpose to avoid confusion about file locations.

## Assumptions

1. The server and client are running on the same machine (localhost).
2. The server uses port 2206 by default.
3. The server has read/write access to the specified input and output folders.
4. The client has read/write access to its specified input and output folders.
5. File names do not contain spaces or special characters.
6. The maximum file size is limited by available memory and disk space.

## Challenges Encountered

1. **Handling Partial Transfers**: Implementing the resume functionality required careful management of file offsets and ensuring data integrity.

2. **Concurrency**: The server uses forking to handle multiple clients simultaneously, which introduced complexity in managing shared resources and child processes.

3. **Error Handling**: Robust error handling and reporting were necessary to manage various failure scenarios, such as network interruptions or file access issues.

4. **Progress Tracking**: Implementing a progress bar for file transfers required careful calculation and timely updates without significantly impacting transfer speed.

5. **Cross-Platform Compatibility**: Ensuring the code works on different UNIX-like systems required careful use of standard libraries and system calls.

6. **Security Considerations**: While not fully implemented, considerations for file access permissions and potential security vulnerabilities were important aspects of the design process.

7. **Large File Handling**: Efficiently transferring large files required careful memory management and proper use of buffering.

## Future Improvements

1. Implement authentication and encryption for secure file transfers.
2. Add support for transferring entire directories.
3. Implement a graphical user interface for the client.
4. Add more robust error recovery mechanisms for network interruptions.
5. Implement file integrity checks using checksums or hashes.