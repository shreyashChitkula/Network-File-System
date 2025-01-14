#include "header.h"

void clear_socket_buffer(int socket_fd) {
    char temp_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    do {
        bytes_read = recv(socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT);
    } while (bytes_read > 0);
}
void delete_operation(int client_socket, char *path) {
    printf("Sending DELETE request to NS (path=%s)\n", path);

    // Prepare and send DELETE request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = DELETE_REQ;
    strcpy(request.path, path);

    clear_socket_buffer(client_socket);
    
    ssize_t bytes_sent = send(client_socket, &request, sizeof(st_request), 0);
    if (bytes_sent < 0) {
        perror("Failed to send DELETE request to NS");
        return;
    }

    // Get response from NS
    char *ns_response = (char *)malloc(sizeof(char) * 1024);
    ssize_t bytes_received = recv(client_socket, ns_response, sizeof(char) * 1024, 0);
    if (bytes_received > 0) {
        printf("Response from NS: %s\n", ns_response);
    } else {
        printf("Error receiving response from naming server\n");
    }
}