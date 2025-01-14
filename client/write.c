#include "header.h"

void write_operation(int client_socket, char *path, char *content,int bit) {
    // No need to connect to NS as we're using passed socket
    printf("Sending WRITE request to NS (path=%s)\n", path);

    // 1. Send WRITE request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = WRITE_REQ;
    strcpy(request.path, path);
    strcpy(request.content, content);
    request.sync_bit=bit;
    
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send WRITE request to NS");
        return;
    }

    // 2. Get SS details from NS
    char ns_response[BUFFER_SIZE] = {0};
    ssize_t bytes_received = recv(client_socket, ns_response, sizeof(ns_response) - 1, 0);
    if (bytes_received <= 0) {
        printf("Error: Failed to receive response from naming server\n");
        return;
    }
    ns_response[bytes_received] = '\0';

    // 3. Parse SS details
    char ss_ip[16];
    int ss_port;
    if (sscanf(ns_response, "IP: %s Port: %d", ss_ip, &ss_port) != 2) {
        // printf("Error: Invalid storage server details received\n");
        printf("%s\n",ns_response);
        return;
    }

    // 4. Connect to Storage Server
    printf("Connecting to Storage Server at %s:%d\n", ss_ip, ss_port);
    int ss_socket = connect_with_ss(ss_ip, ss_port);
    if (ss_socket < 0) {
        printf("Error: Could not connect to storage server\n");
        return;
    }

    // 5. Send WRITE request to SS
    bytes_sent = send(ss_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send WRITE request to SS");
        close(ss_socket);
        return;
    }

    // 6. Wait for acknowledgment from SS
    t_request ss_response;
    bytes_received = recv(ss_socket, &ss_response, sizeof(t_request), 0);
    if (bytes_received > 0) {
        if (ss_response.request_type == ERROR) {
            printf("Error from storage server: %s\n", ss_response.data);
        } else if (ss_response.request_type == SUCCESS) {
            printf("Write operation successful!\n");
        } else {
            printf("Unexpected response type: %d\n", ss_response.request_type);
        }
    } else {
        printf("Error receiving response from storage server\n");
    }

    // 7. Cleanup SS connection (keep NS connection open)
    close(ss_socket);
}