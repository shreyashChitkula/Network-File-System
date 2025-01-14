#include "header.h"

void stream_operation(int ns_socket, char *path) {
    // Send stream request to NS
    st_request request;
    memset(&request, 0, sizeof(st_request));
    request.request_type = STREAM_REQ;
    strncpy(request.path, path, MAX_PATH - 1);
    
    printf("Sending STREAM request to NS for file: %s\n", path);
    
    if (send(ns_socket, &request, sizeof(st_request), 0) < 0) {
        perror("Failed to send STREAM request to NS");
        return;
    }

    // Receive SS details from NS
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(ns_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        printf("Error receiving response from naming server\n");
        return;
    }
    buffer[bytes_received] = '\0';

    // Parse IP and Port from NS response
    char ip[16];
    int port;
    if (sscanf(buffer, "IP: %15s Port: %d", ip, &port) != 2) {
        printf("Invalid response from naming server\n");
        return;
    }

    // Connect to Storage Server
    int ss_socket = connect_with_ss(ip, port);
    if (ss_socket < 0) {
        printf("Failed to connect to storage server\n");
        return;
    }

    // Send stream request to SS
    if (send(ss_socket, &request, sizeof(st_request), 0) < 0) {
        perror("Failed to send STREAM request to SS");
        close(ss_socket);
        return;
    }

    // Create a temporary file to store the streamed data
    char temp_file[] = "/tmp/nfs_stream_XXXXXX";
    int temp_fd = mkstemp(temp_file);
    if (temp_fd < 0) {
        perror("Failed to create temporary file");
        close(ss_socket);
        return;
    }

    // Receive and write audio data to temporary file
    char data_buffer[BUFFER_SIZE];
    while ((bytes_received = recv(ss_socket, data_buffer, BUFFER_SIZE, 0)) > 0) {
        if (write(temp_fd, data_buffer, bytes_received) != bytes_received) {
            perror("Failed to write to temporary file");
            close(temp_fd);
            unlink(temp_file);
            close(ss_socket);
            return;
        }
    }

    close(temp_fd);
    close(ss_socket);

    // Play the audio file using mpv
    char command[MAX_BUFFER];
    snprintf(command, sizeof(command), "mpv %s", temp_file);
    printf("Starting playback...\n");
    
    int result = system(command);
    if (result != 0) {
        printf("Failed to play audio file. Please ensure mpv is installed.\n");
        printf("You can install it using:\n");
        printf("  - On MacOS: brew install mpv\n");
        printf("  - On Ubuntu: sudo apt-get install mpv\n");
    }

    // Clean up temporary file
    unlink(temp_file);
}