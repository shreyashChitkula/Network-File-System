#ifndef TWO_THREADS_H
#define TWO_THREADS_H

#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define STORAGE_SERVER_PORT 8080 // Port for Storage Server connections
#define CLIENT_PORT 8081         // Port for Client connections
#define TEMP_SS_PORT 8082
#define MAX_STORAGE_SERVERS 10 // Maximum number of storage servers
#define MAX_PATHS 100          // Maximum number of accessible paths
#define MAX_PATH_LENGTH 512    // Maximum length of each path
#define BUFFER_LENGTH 1024
#define FILENAME_LENGTH 64

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

typedef struct
{
    char ip[INET_ADDRSTRLEN];
    int nm_port;
    int client_port;
    int pathscount;                         // Number of accessible paths
    char paths[MAX_PATHS][MAX_PATH_LENGTH]; // 2D array for paths
} StorageServerInfo;

#pragma pack(1)
typedef struct
{
    int request_type;                // LIST_REQ, READ_REQ, etc.
    char path[MAX_PATH_LENGTH];      // For operations that need a path
    char name[FILENAME_LENGTH];      // For CREATE operation
    char content[BUFFER_LENGTH];     // For WRITE operation
    char dest_path[MAX_PATH_LENGTH]; // For COPY operation destination
    int flag;
} ClientMessage;
#pragma pack()

// Acknowledgment structure
#pragma pack(1)
typedef struct
{
    int status;
} ACK;
#pragma pack()
#pragma pack(push, 1) // Prevent padding
typedef struct
{
    int request_type;
    char data[BUFFER_LENGTH];
} Packet;
#pragma pack(pop)

typedef struct
{
    char string[BUFFER_LENGTH];
    int err_code;
    char ip[INET_ADDRSTRLEN];
    int port;
    char fpath[MAX_PATH_LENGTH];
} Async_Ack;

typedef struct
{
    char ip[INET_ADDRSTRLEN];
    int storage_server;
} client_async;

// Function to create and start the two threads
int start_two_threads();

// Function prototypes for thread functions
void clear_socket_buffer(int socket_fd);
void *handle_storage_server(void *arg);
void *handle_client_request(void *arg);
void *handle_storage_server_connections(void *arg);
void *handle_client_connections(void *arg);
int send_create_request_to_storage_server(int server_socket, ClientMessage *request);
void handle_create_request(int client_socket, ClientMessage *request);
void *handle_temp_server_connections(void *arg);
int send_message_to_host(char *ip, int port, Packet *packet);
char *extract_file_name(char *path);
int find_storage_server(const char *address, const int clientPort);
int path_exists(StorageServerInfo *server, const char *path);
void add_path(StorageServerInfo *storageServer, char *directory, char *fileName);
void copy(ClientMessage *Clientmessage, int client_socket);
void update_storage_server(int index, int new_socket_fd, StorageServerInfo *storageServer);
int compare_2d_arrays(const char arr1[][MAX_PATH_LENGTH], const char arr2[][MAX_PATH_LENGTH], size_t row_count);
int add_or_update_storage_server(StorageServerInfo *storageServer, int socket_fd);
int handle_delete_request(ClientMessage *request, int client_socket);
void add_client(int socket, int idx);
void delete_client(int idx);

#endif // TWO_THREADS_H
