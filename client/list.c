#include "header.h" 

void list_operation(int client_socket) {
    // 1. Send LIST request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = LIST_REQ;
    
    //printf("Sending LIST request to NS : %d\n", request.request_type);
    clear_socket_buffer(client_socket);
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent <= 0) {
        perror("Failed to send LIST request to NS");
        return;
    }
    //printf("LIST request sent to NS : %d\n", request.request_type);
    // 2. Receive and display file list from NS

    
    // Buffer for receiving raw data
    char buffer[MAX_DATA_LEN];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Ensure null termination
        
        if (strcmp(buffer, "No files or directories found.") == 0) {
            printf("  %s\n", buffer);
        } else {
            char *line = strtok(buffer, "\n");
            while (line != NULL) {
                if (strlen(line) > 0) {  // Only print non-empty lines
                    printf("  %s\n", line);
                }
                line = strtok(NULL, "\n");
            }
        }
    } else if (bytes_received == 0) {
        printf("Connection closed by naming server\n");
    } else {
        perror("Error receiving data from naming server");
    }
    
}


