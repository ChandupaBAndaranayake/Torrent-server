//server.c
#define SERVER

#include "file_transfer_common.h"

/*
logger: location ./log/server_log.log
*/
void log_message(const char *client_ip, int client_port, const char *requested_file, const char *transfer_status) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datetime[64];
    strftime(datetime, sizeof(datetime) - 1, "%Y-%m-%d %H:%M:%S", t);
    fprintf(log_file, "[%s] Client IP: %s, Port: %d, File: %s, Status: %s\n", 
            datetime, client_ip, client_port, requested_file, transfer_status);
    fclose(log_file);
}

/*
returning the list at ./server_folder/
*/
char* get_file_list() {
    static char file_list[4096];
    memset(file_list, 0, sizeof(file_list));

    DIR *dir = opendir(SERVER_INPUT_FOLDER);
    if (dir == NULL) {
        perror("Error opening shared folder");
        return NULL;
    }
    
    struct dirent *ent;
    size_t current_length = 0;
    int file_number = 1;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG) {
            char formatted_entry[MAX_FILENAME + 20]; 
            snprintf(formatted_entry, sizeof(formatted_entry), "%d: %s\n", file_number++, ent->d_name);

            size_t formatted_length = strlen(formatted_entry);
            if (current_length + formatted_length + 1 < sizeof(file_list)) { 
                strcat(file_list, formatted_entry);
                current_length += formatted_length;
            } else {
                fprintf(stderr, "Buffer overflow prevented: not enough space for more files.\n");
                break; 
            }
        }
    }

    if (closedir(dir) == -1) {
        perror("Error closing directory");
    }

    return file_list;
}

/*
sends the file list after getting the "GET" request
*/
int send_file_list(int client_socket) {
    char *file_list = get_file_list();
    if (file_list == NULL) {
        return -1;
    }
    if (send(client_socket, file_list, strlen(file_list), 0) == -1) {
        perror("Error sending file list");
        return -1;
    }
    return 0;
}

/*
sends the client requested file after client sending the required filename
*/
int send_file(int client_socket, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (send(client_socket, &file_size, sizeof(file_size), 0) == -1) {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Error sending file chunk");
            fclose(file);
            return -1;
        }
    }

    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/*
Retrieve the Client Uploaded File
*/
int receive_file(int client_socket, const char *filepath) {
    long file_size;
    if (recv(client_socket, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Error receiving file size");
        return -1;
    }

    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("Error creating file");
        return -1;
    }

    char buffer[CHUNK_SIZE];
    long received_bytes = 0;
    while (received_bytes < file_size) {
        ssize_t recv_bytes = recv(client_socket, buffer, CHUNK_SIZE, 0);
        if (recv_bytes <= 0) {
            if (recv_bytes < 0) {
                perror("Error receiving file chunk");
            }
            fclose(file);
            return -1;
        }
        size_t bytes_written = fwrite(buffer, 1, recv_bytes, file);
        if (bytes_written < recv_bytes) {
            perror("Error writing to file");
            fclose(file);
            return -1;
        }
        received_bytes += recv_bytes;
    }

    fclose(file);
    return 0;
}

/*
Resuming the interrupted Download by retriveing the current size (as offset) of the clients file and sending the remaining file
*/
int send_file_from_offset(int client_socket, const char *filepath, long offset) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    
    if (offset >= file_size) {
        fclose(file);
        return -1; // Offset is beyond file size
    }

    long remaining_size = file_size - offset;
    if (send(client_socket, &remaining_size, sizeof(remaining_size), 0) == -1) {
        perror("Error sending remaining file size");
        fclose(file);
        return -1;
    }

    fseek(file, offset, SEEK_SET);

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Error sending file chunk");
            fclose(file);
            return -1;
        }
    }
    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/*
Handle clients request {GET or POST or RESUME}
*/
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(client_addr.sin_port);
    
    log_message(client_ip, client_port, "N/A", "Connected");

    char buffer[CHUNK_SIZE];
    ssize_t recv_bytes = recv(client_socket, buffer, CHUNK_SIZE - 1, 0);
    if (recv_bytes <= 0) {
        log_message(client_ip, client_port, "N/A", recv_bytes == 0 ? "Disconnected" : "Error receiving command");
        close(client_socket);
        return;
    }
    
    buffer[recv_bytes] = '\0';
    printf("Received command from client:%s:%d - %s\n", client_ip, client_port, buffer);

    if (strcmp(buffer, "GET") == 0) {
        if (send_file_list(client_socket) == -1) {
            log_message(client_ip, client_port, "N/A", "Error sending file list");
            close(client_socket);
            return;
        }

        recv_bytes = recv(client_socket, buffer, CHUNK_SIZE - 1, 0);
        if (recv_bytes <= 0) {
            log_message(client_ip, client_port, "N/A", "Error receiving file request");
            close(client_socket);
            return;
        }
        buffer[recv_bytes] = '\0';

        char filepath[MAX_FILENAME + sizeof(SERVER_INPUT_FOLDER)];
        snprintf(filepath, sizeof(filepath), "%s%s", SERVER_INPUT_FOLDER, buffer);
        
        if (send_file(client_socket, filepath) == -1) {
            log_message(client_ip, client_port, buffer, "Transfer failed");
        } else {
            log_message(client_ip, client_port, buffer, "Transfer successful");
        }
    } else if (strcmp(buffer, "POST") == 0) {
        recv_bytes = recv(client_socket, buffer, CHUNK_SIZE - 1, 0);
        if (recv_bytes <= 0) {
            log_message(client_ip, client_port, "N/A", "Error receiving file name");
            close(client_socket);
            return;
        }
        buffer[recv_bytes] = '\0';

        char filepath[MAX_FILENAME + sizeof(SERVER_OUTPUT_FOLDER)];
        snprintf(filepath, sizeof(filepath), "%s%s", SERVER_OUTPUT_FOLDER, buffer);
        
        if (receive_file(client_socket, filepath) == -1) {
            log_message(client_ip, client_port, buffer, "Upload failed");//logging 
        } else {
            log_message(client_ip, client_port, buffer, "Upload successful");//logging
        }
    } else if(strcmp(buffer, "RESUME") == 0){
        // Receive filename
        recv_bytes = recv(client_socket, buffer, CHUNK_SIZE - 1, 0);
        if (recv_bytes <= 0) {
            log_message(client_ip, client_port, "N/A", "Error receiving filename for resume");
            close(client_socket);
            return;
        }

        buffer[recv_bytes] = '\0';
        char filepath[MAX_FILENAME + sizeof(SERVER_INPUT_FOLDER)];
        snprintf(filepath, sizeof(filepath), "%s%s", SERVER_INPUT_FOLDER, buffer);

        // Receive offset
        long offset;
        if (recv(client_socket, &offset, sizeof(offset), 0) <= 0) {
            log_message(client_ip, client_port, buffer, "Error receiving offset for resume");
            close(client_socket);
            return;
        }

        if (send_file_from_offset(client_socket, filepath, offset) == -1) {
            log_message(client_ip, client_port, buffer, "Resume transfer failed");
        } else {
            log_message(client_ip, client_port, buffer, "Resume transfer successful");
        }
    }else {
        log_message(client_ip, client_port, "N/A", "Unknown command");
    }
    close(client_socket);
}

/*
child process termination signal handler
 */
void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    //signal handler init......
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    //socket creation
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //socket binding
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(server_socket);
        exit(1);
    }

    //server is listen state
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error listening on socket");
        close(server_socket);
        exit(1);
    }

    printf("Server started on IP: 127.0.0.1, Port: %d\n", SERVER_PORT);
    printf("Waiting for client connections...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }
        //Displaying connected clients information------------------------------//
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        printf("Client Connect From: %s , Port: %d \n", client_ip, client_port);
        //------------------------------------------------------------------------//

        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("Error forking process");
            close(client_socket);
        } else if (child_pid == 0) {
            close(server_socket);
            handle_client(client_socket, client_addr);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}