// client.c

#include "file_transfer_common.h"

/*
displays the download and upload progress
*/
void display_progress(long current, long total) {
    int percent = (int)((current * 100) / total);
    printf("\rProgress: %d%%", percent);
    fflush(stdout);
}

/*
return the list content in the ./client_folder/ as filelist
*/
char* get_file_list() {
    static char file_list[4096];
    memset(file_list, 0, sizeof(file_list));

    DIR *dir = opendir(CLIENT_INPUT_FOLDER);
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
            if (current_length + formatted_length + 2 < sizeof(file_list)) { 
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
Binding the client sock into a fixed port
*/
int bind_socket(int sockfd,char *client_ip ){
    struct sockaddr_in client_addr;

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip); 
    client_addr.sin_port = htons(CLIENT_PORT);  

    // Bind the socket to the specified IP and port
    if (bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    printf("Client ip address: %s, port: %d\n", client_ip, CLIENT_PORT);

    return 0 ;
}

/*
socket creation
*/
int create_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }
    return sock;
}

/*
connecting to the server
*/
int connect_to_server(int sock) {
    struct sockaddr_in serv_addr = {0};

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }
    //Displaying connected  information------------------------------//
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &serv_addr.sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(serv_addr.sin_port);
    printf("Connected to server at %s:%d \n", client_ip, client_port);
    printf("Connected to server.\n");
    //-------------------------------------------------------------------//
    
    return 0;
}

/*
sends request or command -->{"GET", "POST", "RESUME"}
*/
int send_request(int sock, const char* request) {
    if (send(sock, request, strlen(request), 0) == -1) {
        perror("Error sending request");
        return -1;
    }
    return 0;
}

/*
Retrieve file of the server input directory
*/
int receive_file_list(int sock) {
    char buffer[CHUNK_SIZE] = {0};
    ssize_t recv_bytes = recv(sock, buffer, CHUNK_SIZE, 0);
    if (recv_bytes <= 0) {
        if (recv_bytes == 0) {
            fprintf(stderr, "Server closed connection\n");
        } else {
            perror("Error receiving file list");
        }
        return -1;
    }
    printf("//************************Available files*******************************//\n");
    printf("%s\n",  buffer);
    printf("//**********************************************************************//\n");

    return 0;
}

/*
Retrieving the requested file from the server and save it in the ./Client folder
*/
int download_file(int sock, const char* filename, long offset) {
    char filepath[MAX_FILENAME + sizeof(CLIENT_OUTPUT_FOLDER)];
    snprintf(filepath, sizeof(filepath), "%s%s", CLIENT_OUTPUT_FOLDER, filename);

    FILE *file;
    if (offset > 0) {
        file = fopen(filepath, "r+b");
        if (file == NULL) {
            perror("Error opening file for resume");
            return -1;
        }
        fseek(file, offset, SEEK_SET);
    }else{
        file = fopen(filepath, "wb");
        if (file == NULL) {
            perror("Error creating file");
            return -1;
        }
    }

    long file_size = 0;
    if (recv(sock, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Error receiving file size");
        return -1;
    }
    printf("Remaining File size: %ld bytes\n", file_size);
    
    char buffer[CHUNK_SIZE];
    long received_bytes = 0;
    while (received_bytes < file_size) {
        ssize_t recv_bytes = recv(sock, buffer, CHUNK_SIZE, 0);
        if (recv_bytes <= 0) {
            if (recv_bytes < 0) {
                perror("Error receiving file chunk");
            }
            break;
        }
        
        size_t bytes_written = fwrite(buffer, 1, recv_bytes, file);
        if (bytes_written < recv_bytes) {
            perror("Error writing to file");
            break;
        }
        received_bytes += recv_bytes;
        display_progress(received_bytes, file_size);
    }

    printf("\n");
    fclose(file);
    return (received_bytes == file_size) ? 0 : -1;
}

/*
Uploading the selected file in the ./client_folder/ to the server
*/
int upload_file(int sock, const char* filename) {
    char filepath[MAX_FILENAME + sizeof(CLIENT_INPUT_FOLDER)];
    snprintf(filepath, sizeof(filepath), "%s%s", CLIENT_INPUT_FOLDER, filename);
    
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (send(sock, &file_size, sizeof(file_size), 0) == -1) {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

    char buffer[CHUNK_SIZE];
    long total_bytes_sent = 0;
    while (total_bytes_sent < file_size) {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, file);
        if (bytes_read == 0) {
            if (ferror(file)) {
                perror("Error reading file");
            }
            break;
        }
        if (send(sock, buffer, bytes_read, 0) == -1) {
            perror("Error sending file chunk");
            break;
        }
        total_bytes_sent += bytes_read;
        display_progress(total_bytes_sent, file_size);
    }

    printf("\n");
    fclose(file);
    return (total_bytes_sent == file_size) ? 0 : -1;
}

int main(int argc, char *argv[]) {
    if(argc != 2){
        fprintf(stderr, "Usage: %s <client_ip_address>\n", argv[0]);
        return -1;
    }

    int sock = create_socket();
    if (sock == -1 || bind_socket(sock, argv[1]) == -1) {
        perror("Bind Failed!\n");
        return -1;
    }

    if(connect_to_server(sock) == -1){
        perror("Connection Failed by the Server!\n");
        return -1;
    }

    //Abstraction for server commands
    /*
    GET = Download
    POST = Upload
    RESUME = Resume Download
    */
    printf("Select Option:\n1. Download\n2. Upload\n3. Resume Download\n4. Exit\n[1], [2], [3], or [4]: ");
    char input[10];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        close(sock);
        return -1;
    }

    int option = atoi(input);

    switch(option) {
        case 1: {
            printf("Selected option: Download\n");
            printf("\n");

            if (send_request(sock, "GET") == -1 || receive_file_list(sock) == -1) {
                close(sock);
                return -1;
            }
            
            char filename[MAX_FILENAME];
            printf("Enter the name of the file you want to download: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL) {
                fprintf(stderr, "Error reading input\n");
                close(sock);
                return -1;
            }
            filename[strcspn(filename, "\n")] = 0;
            
            if (send_request(sock, filename) == -1 || download_file(sock, filename, 0) == -1) {
                fprintf(stderr, "Download failed\n");
            } else {
                printf("File downloaded successfully\n");
            }
            break;
        }
        case 2: {
            printf("Selected option: Upload\n");
            printf("\n");

            if (send_request(sock, "POST") == -1) {
                close(sock);
                return -1;
            }

            char *file_list = get_file_list();
            if (file_list == NULL) {
                close(sock);
                return -1;
            }
            printf("//************************Available files*******************************//\n");
            printf("%s\n",  file_list);
            printf("//**********************************************************************//\n");

            char filename[MAX_FILENAME];
            printf("Enter the name of the file you want to upload: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL) {
                fprintf(stderr, "Error reading input\n");
                close(sock);
                return -1;
            }
            filename[strcspn(filename, "\n")] = 0;
            
            if (send_request(sock, filename) == -1 || upload_file(sock, filename) == -1) {
                fprintf(stderr, "Upload failed\n");
            } else {
                printf("File uploaded successfully\n");
            }
            break;
        }
        case 3:{
            printf("Selected option: Resume\n");
            printf("\n");

            if (send_request(sock, "RESUME") == -1) {
                close(sock);
                return -1;
            }
            char filename[MAX_FILENAME];
            printf("Enter the name of the file you want to resume downloading: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL) {
                fprintf(stderr, "Error reading input\n");
                close(sock);
                return -1;
            }
            filename[strcspn(filename, "\n")] = 0;
            
            if (send_request(sock, filename) == -1) {
                close(sock);
                return -1;
            }

            char filepath[MAX_FILENAME + sizeof(CLIENT_OUTPUT_FOLDER)];
            snprintf(filepath, sizeof(filepath), "%s%s", CLIENT_OUTPUT_FOLDER, filename);
            
            FILE *file = fopen(filepath, "rb");
            long offset = 0;
            if (file != NULL) {
                fseek(file, 0, SEEK_END);
                offset = ftell(file);
                fclose(file);
            }

            if (send(sock, &offset, sizeof(offset), 0) == -1) {
                perror("Error sending offset");
                close(sock);
                return -1;
            }
            if (download_file(sock, filename, offset) == -1) {
                fprintf(stderr, "Resume download failed\n");
            } else {
                printf("File resume download successful\n");
            }
            break;
        }
        case 4:
            printf("Connection terminated\n");
            break;
        default:
            fprintf(stderr, "Invalid option\n");
    }

    close(sock);
    return 0;
}

