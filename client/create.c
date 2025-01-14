#include "header.h"

void create_operation(int client_socket, char *path, char *name) {
    printf("Sending CREATE request to NS (path=%s, name=%s)\n", path, name);

    // Prepare and send CREATE request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = CREATE_REQ;
    strcpy(request.path, path);
    strcpy(request.name, name);
    
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send CREATE request to NS");
        return;
    }

    // Get response from NS
    char buffer[MAX_DATA_LEN];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Ensure null termination
        
        if (strcmp(buffer, "No files or directories found.") == 0) {
            printf("RESPONSE FROM THE NAMING SERVER:  %s\n", buffer);
        } else {
            char *line = strtok(buffer, "\n");
            while (line != NULL) {
                if (strlen(line) > 0) {  // Only print non-empty lines
                    printf(" %s\n", line);
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