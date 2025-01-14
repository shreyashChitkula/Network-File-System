#define HEADERS_H
#ifdef HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"

// Request Types
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

#define NAMING_SERVER_PORT 8080      // Port for Naming Server connection (change as needed)
#define TEMP_NS_PORT 8082
#define NAMING_SERVER_IP "10.42.0.1" // Naming Server IP (change as needed)
#define CLIENT_PORT 2000

#define MAX_PATHS 100    // Max number of paths we can store in the array
#define MAX_PATH_LEN 512 // Increased length of each path to reduce truncation risk
#define MAX_STR_LEN 64
#define MAX_BUFFER 1024
#define MAX_DATA_LEN 1024

typedef struct
{
    char ip[INET_ADDRSTRLEN];                       // IP address of the storage server
    int port_nm;                                    // Port for Naming Server connection
    int port_client;                                // Port for Client connection
    int path_count;                                 // Number of accessible paths
    char accessible_paths[MAX_PATHS][MAX_PATH_LEN]; // List of accessible paths

} SStoNM;

#pragma pack(1)

typedef struct
{
    int request_type;             // LIST_REQ, READ_REQ, etc.
    char path[MAX_PATH_LEN];      // For operations that need a path
    char name[MAX_STR_LEN];       // For CREATE operation
    char content[MAX_BUFFER];     // For WRITE operation
    char dest_path[MAX_PATH_LEN]; // For COPY operation destination
    int sync_bit;
} MSGtoSS;

#pragma pack()

typedef struct
{
    int request_type;
    char data[MAX_DATA_LEN];
} t_request;

typedef struct {
    MSGtoSS *msg_ptr;
    int server_sock;
} ThreadArgs;

#pragma pack(1)

typedef struct{
    int ack_no;
}ack;

typedef struct{
    char string[MAX_DATA_LEN];
    int err_code;
    char ip[INET_ADDRSTRLEN];
    int port;
    char fpath[MAX_PATH_LEN];
}async;

#pragma pack()



// char* extract_file_name(const char *path);

char *extract_file_name(char *path);

void clear_socket_buffer(int socket_fd);

void initialize_locks();

int get_lock_index(const char *path);

int delete_directory_recursively(const char *path);

int get_all_paths(char *base_dir, char paths[][MAX_PATH_LEN], int *path_count);

int set_socket_info(int socket_fd, async *info, int remote);

void get_file_info(char *file_path, char *response_data, size_t response_size);



#endif

