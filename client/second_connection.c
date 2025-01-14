// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <arpa/inet.h>
// #include <pthread.h>
#include "header.h"

// #pragma pack(push, 1)
// typedef struct
// {
//     int request_type;
//     char data[1024];
// } t_request;
// #pragma pack(pop)

void *receiver()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket for storage server connections
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed for storage server");
        pthread_exit(NULL);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed for storage server");
        close(server_fd);
        pthread_exit(NULL);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8083);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed for storage server");
        close(server_fd);
        pthread_exit(NULL);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed for storage server");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("Server listener started on port %d\n", 8083);

    // Accept storage server registrations in a loop
    while (1)
    {

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed for storage server");
        }

        t_request info;
        memset(&info, 0, sizeof(t_request));

        // Read storage server information
        ssize_t bytes_read = recv(new_socket, &info, sizeof(t_request), 0);

        if (bytes_read <= 0)
        {
            perror("Failed to read from storage server socket");
            close(new_socket);
            pthread_exit(NULL);
        }

        printf("\n\n%s : %d\n\n", info.data, info.request_type);
    }
    // clear_socket_buffer(new_socket);

    close(server_fd);
    close(new_socket);
    pthread_exit(NULL);
}
