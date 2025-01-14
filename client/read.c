#include "header.h"


// void clear_socket_buffer(int socket_fd) {
//     char temp_buffer[BUFFER_SIZE];
//     ssize_t bytes_read;
//     do {
//         bytes_read = recv(socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT);
//     } while (bytes_read > 0);
// }



void read_operation(int client_socket, char *path) {
    printf("Sending READ request to NS (path=%s)\n", path);
    
    // Use st_request for sending request
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = READ_REQ;
    strcpy(request.path, path);
    
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send READ request to NS");
        return;
    }

    // Get SS details from NS
    char ns_response[BUFFER_SIZE] = {0};
    ssize_t bytes_received = recv(client_socket, ns_response, sizeof(ns_response) - 1, 0);
    if (bytes_received <= 0) {
        printf("Error: Failed to receive response from naming server\n");
        return;
    } 
    ns_response[bytes_received] = '\0';
    //printf("Raw response from NS (%zd bytes): '%s'\n", bytes_received, ns_response);

    // Parse SS details
    char ss_ip[16];
    int ss_port;
    if (sscanf(ns_response, "IP: %s Port: %d", ss_ip, &ss_port) != 2) {
        printf("Path does not exist\n");
        return;
    }

    // Connect to Storage Server
    int ss_socket = connect_with_ss(ss_ip, ss_port);
    if (ss_socket < 0) {
        printf("Error: Could not connect to storage server\n");
        return;
    }

    // Send READ request to SS
    //clear_socket_buffer(ss_socket);
    bytes_sent = send(ss_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send READ request to SS");
        close(ss_socket);
        return;
    }

    printf("\nFile content:\n");
    printf("---------------\n");
    
    char previous_chunk[BUFFER_SIZE] = {0};  // Add buffer for previous chunk
    while(1) {
        t_request ss_response;
        memset(&ss_response, 0, sizeof(t_request));  // Clear the response structure
        
        bytes_received = recv(ss_socket, &ss_response, sizeof(t_request), MSG_WAITALL);
        if (bytes_received > 0) {
            if (ss_response.request_type == STOP) {
                // Print any remaining data in previous chunk
                if (strlen(previous_chunk) > 0) {
                    printf("%s", previous_chunk);
                }
                break;
            }
            if (ss_response.request_type == ERROR) {
                printf("Error from storage server: %s\n", ss_response.data);
            } else if (ss_response.request_type == SUCCESS) {
                // Store current chunk and print previous chunk
                printf("%s", previous_chunk);
                strncpy(previous_chunk, ss_response.data, BUFFER_SIZE - 1);
                previous_chunk[BUFFER_SIZE - 1] = '\0';
            }
        } 
        else if (bytes_received == 0) {
            // Print any remaining data before closing
            if (strlen(previous_chunk) > 0) {
                printf("%s", previous_chunk);
            }
            printf("\nConnection closed by storage server\n");
            break; 
        } 
        else {
            perror("Error receiving data from storage server");
            break;
        }
    }
    printf("\n---------------\n");
    close(ss_socket);
}