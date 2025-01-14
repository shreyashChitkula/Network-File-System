#include "header.h"

int connect_with_ns() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NS_PORT);  // Using NS_PORT from header.h
    server_addr.sin_addr.s_addr = inet_addr(NS_IP); // IP address of the naming server

    printf("Attempting to connect to %s:%d\n", 
           inet_ntoa(server_addr.sin_addr), 
           ntohs(server_addr.sin_port));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to naming server");
        close(client_socket);
        return -1;
    }
    return client_socket;
}



int connect_with_ss(char *ss_ip, int ss_port) {
    int ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket < 0) {
        perror("Error creating socket for SS");
        return -1;
    }

    struct sockaddr_in ss_addr;
    memset(&ss_addr, 0, sizeof(ss_addr));
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(ss_port);
    ss_addr.sin_addr.s_addr = inet_addr(ss_ip);

    printf("Attempting to connect to Storage Server at %s:%d\n", 
           ss_ip, ss_port);

    if (connect(ss_socket, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) < 0) {
        perror("Error connecting to storage server");
        close(ss_socket);
        return -1;
    }

    return ss_socket;
}