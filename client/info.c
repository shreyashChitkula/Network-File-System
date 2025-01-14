#include "header.h"



void info_operation(int client_socket, char *path) {
    printf("Sending INFO request to NS (path=%s)\n", path);

    // Use st_request for sending request
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = INFO_REQ;
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
         printf("%s\n",ns_response);
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


    
    t_request ss_response;
    memset(&ss_response, 0, sizeof(t_request));  // Clear the response structure
    
    bytes_received = recv(ss_socket, &ss_response, sizeof(t_request), MSG_WAITALL);
    if (bytes_received > 0) {
        printf("\n\n%s", ss_response.data);
    } 
    else if (bytes_received == 0) {
        printf("No data received from storage server\n");
        } 
        else {
            perror("Error receiving data from storage server");
        }
    close(ss_socket);
}