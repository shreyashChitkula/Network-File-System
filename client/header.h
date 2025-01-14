#ifndef HEADERS_H
#define HEADERS_H

// Required standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
// Constants for network communication
#define NS_PORT 8081        // Port for Naming Server
#define TEMP_RECEIVER_PORT 8083
#define NS_IP "10.42.0.1"   // IP address of the naming server
#define SS_PORT 8000        // Port for Storage Server
#define MAX_BUFFER 1024     // Maximum buffer size
#define MAX_PATH 512       // Maximum path length
#define MAX_FILENAME 64     // Maximum filename length
#define BUFFER_SIZE 1024    // Buffer size for sending and receiving messages
#define MAX_DATA_LEN 10240

// Request types
#define LIST_REQ 1
#define READ_REQ 2
#define WRITE_REQ 3
#define CREATE_REQ 4
#define DELETE_REQ 5
#define COPY_REQ 6
#define INFO_REQ 7
#define STREAM_REQ 8
#define ERROR 9
#define SUCCESS 10
#define STOP 11
#define RES 12

// Structure for request/response packets
typedef struct {
    int request_type;      // LIST_REQ, READ_REQ, etc.
    char path[MAX_PATH];   // For operations that need a path
    char name[MAX_FILENAME]; // For CREATE operation
    char content[MAX_BUFFER]; // For WRITE operation
    char dest_path[MAX_PATH]; // For COPY operation destination
    int sync_bit;
} ClientMessage;  // This should match exactly with NS's structure

// Rename our request structure to use ClientMessage
typedef ClientMessage st_request;  // Keep backward compatibility

#pragma pack(push, 1)
typedef struct{
    int request_type;
    char data[BUFFER_SIZE];
}t_request;
#pragma pack(pop)


// Function declarations for network operations
int connect_with_ns(void);
int connect_with_ss(char *ip, int port);
void* temp_receiver(void* arg);
// Function declarations for file operations
void list_operation(int ns_socket);
void read_operation(int ns_socket, char *path);
void write_operation(int ns_socket, char *path, char *content,int bit);
void create_operation(int ns_socket, char *path, char *name);
void delete_operation(int ns_socket, char *path);
void copy_operation(int ns_socket, char *source, char *dest);
void info_operation(int ns_socket, char *path);
void stream_operation(int ns_socket,char *path);
void clear_socket_buffer(int socket_fd);
void *receiver();
#endif // HEADERS_H