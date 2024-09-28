//Dont Include the file_transfer_common.h file :) <3!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9122
#define CHUNK_SIZE 1024
#define TEST_FILENAME "./client_folder/hello.c"  // Replace with a valid file name for testing
#define RESUME_TEST_FILENAME "hello.c" // Replace with a valid file name for testing

void send_command(int socket, const char *command) {
    send(socket, command, strlen(command), 0);
}

void send_file(int socket, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file for sending");
        return;
    }

    // Send file name
    send_command(socket, filename);
    
    // Send file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    send(socket, &file_size, sizeof(file_size), 0);

    // Send file data
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void receive_file(int socket, const char *filename) {
    long file_size;
    recv(socket, &file_size, sizeof(file_size), 0);
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for receiving");
        return;
    }

    char buffer[CHUNK_SIZE];
    long received_bytes = 0;
    while (received_bytes < file_size) {
        ssize_t bytes_recv = recv(socket, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, bytes_recv, file);
        received_bytes += bytes_recv;
    }

    fclose(file);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    int RESUME_TEST_OFFSET = 50;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(client_socket);
        return 1;
    }

    // Test GET command
    printf("Testing GET command...\n");
    send_command(client_socket, "GET");
    char buffer[CHUNK_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("Received file list:\n%s\n", buffer);

    // Test POST command
    printf("Testing POST command...\n");
    send_command(client_socket, "POST");
    send_file(client_socket, TEST_FILENAME);
    printf("File %s uploaded successfully.\n", TEST_FILENAME);

    // Test RESUME command
    printf("Testing RESUME command...\n");
    send_command(client_socket, "RESUME");
    send_command(client_socket, RESUME_TEST_FILENAME);
    send(client_socket, &RESUME_TEST_OFFSET, sizeof(RESUME_TEST_OFFSET), 0);
    printf("Resuming upload of %s from offset %d.\n", RESUME_TEST_FILENAME, RESUME_TEST_OFFSET);

    // Clean up
    close(client_socket);
    return 0;
}
