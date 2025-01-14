#include "header.h"

void copy_operation(int client_socket, char *source, char *dest) {
    printf("Sending COPY request to NS (source=%s, dest=%s)\n", source, dest);

    // Prepare and send COPY request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = COPY_REQ;
    strcpy(request.path, source);
    strcpy(request.dest_path, dest);
    
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send COPY request to NS");
        return;
    }

    // Get response directly from NS
    char ns_response[BUFFER_SIZE] = {0};
    ssize_t bytes_received = recv(client_socket, ns_response, sizeof(ns_response) - 1, 0);
    if (bytes_received <= 0) {
        printf("Error: Failed to receive response from naming server\n");
        return;
    }
    ns_response[bytes_received] = '\0';

    // Print the response from NS
    printf("%s\n", ns_response);
}