#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "two_threads.h"
#include "log.h"

#define MAX_FIELD_LENGTH 256

void truncate_string(char *dest, const char *src, size_t max_len) {
    strncpy(dest, src, max_len - 1);
    dest[max_len - 1] = '\0';  // Ensure null-termination
}

// Function to retrieve the IP address of a client socket
void get_ip_address(int client_socket, char *ip_addr) {
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Get the client's address using getpeername() (for peer's address)
    if (getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_size) == -1) {
        perror("getpeername failed");
        strcpy(ip_addr, "Unknown");
        return;
    }

    // Convert the IP address to a string and store it in ip_addr
    if (inet_ntop(AF_INET, &(client_addr.sin_addr), ip_addr, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop failed");
        strcpy(ip_addr, "Invalid IP");
        return;
    }
}

void log_client_request(int socket_fd, ClientMessage *request) {
    char asdf[BUFFER_LENGTH];
    char zcxv[BUFFER_LENGTH];
    // initialize it
    memset(asdf, 0, BUFFER_LENGTH);
    memset(zcxv, 0, BUFFER_LENGTH);
    // Format the request type
    snprintf(asdf, BUFFER_LENGTH, "Request_type:%d", request->request_type);

    // Truncate each field to a safe length
    char path[MAX_FIELD_LENGTH], name[MAX_FIELD_LENGTH], content[MAX_FIELD_LENGTH], dest_path[MAX_FIELD_LENGTH];
    truncate_string(path, request->path, MAX_FIELD_LENGTH);
    truncate_string(name, request->name, MAX_FIELD_LENGTH);
    truncate_string(content, request->content, MAX_FIELD_LENGTH);
    truncate_string(dest_path, request->dest_path, MAX_FIELD_LENGTH);

    // Initialize offset and remaining buffer size
    int offset = 0;
    size_t remaining = BUFFER_LENGTH;

    // Append only if there's space available
    if (remaining > 0 && strlen(path) > 0) {
        int written = snprintf(zcxv + offset, remaining, "path:%s ", path);
        if (written > 0 && written < remaining) {
            offset += written;
            remaining -= written;
        }
    }

    if (remaining > 0 && strlen(name) > 0) {
        int written = snprintf(zcxv + offset, remaining, "name:%s ", name);
        if (written > 0 && written < remaining) {
            offset += written;
            remaining -= written;
        }
    }

    if (remaining > 0 && strlen(content) > 0) {
        int written = snprintf(zcxv + offset, remaining, "content:%s ", content);
        if (written > 0 && written < remaining) {
            offset += written;
            remaining -= written;
        }
    }

    if (remaining > 0 && strlen(dest_path) > 0) {
        int written = snprintf(zcxv + offset, remaining, "destination_path:%s", dest_path);
        if (written > 0 && written < remaining) {
            offset += written;
            remaining -= written;
        }
    }

    // if everything is empty
    if (strlen(zcxv) == 0) {
        snprintf(zcxv, BUFFER_LENGTH, "<no data>");
    }

    // Print the log message for demonstration (replace with bookkeep)
    bookkeep(socket_fd, 0, asdf, zcxv);   
}

// Function to log information with the client's IP and port
void bookkeep(int client_socket, int flag, const char *type, const char *data) {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    char ip_address[INET_ADDRSTRLEN + 6];  // Space for IP and port (e.g., "127.0.0.1:12345")
    get_ip_address(client_socket, ip_address);

    // Get the port number using getsockname (since we want the local port, not peer)
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    if (getsockname(client_socket, (struct sockaddr*)&client_addr, &addr_size) == -1) {
        perror("getsockname failed");
        strcpy(ip_address, "Unknown");
    } else {
        // Append port number to the IP address
        char port[7];  // Enough space for port number, e.g., "12345"
        snprintf(port, 7, ":%d", ntohs(client_addr.sin_port));  // Convert port from network byte order
        strcat(ip_address, port);  // Append the port to the IP address
    }

    // Log the information based on the flag
    if (flag == 0) {
        // 0 means recive
        fprintf(log_file, "FROM: %s\n", ip_address);
    } else if (flag == 1) {
        // 1 means send 
        fprintf(log_file, "TO: %s\n", ip_address);
    } else {
        fprintf(log_file, "UNKNOWN: %s\n", ip_address);
    }

    // Print the type and data
    fprintf(log_file, "TYPE: %s\n", type);
    fprintf(log_file, "DATA: %s\n", data);
    fprintf(log_file, "\n");

    // Close the log file
    fclose(log_file);
}

void init_bookkeep() {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }
    fclose(log_file);
}