#include "two_threads.h"
#include "log.h"
#include "tries.h"
#include "lru.h"

extern StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
extern int storage_server_sockets[MAX_STORAGE_SERVERS];
extern int storage_server_count;
extern pthread_mutex_t lock;
extern TrieNode *trieRoot;


void *handle_storage_server(void *arg)
{
    int socket_fd = *(int *)arg;
    free(arg); // Free the memory allocated for the socket file descriptor
    // clear_socket_buffer(socket_fd);
    StorageServerInfo info;
    memset(&info, 0, sizeof(StorageServerInfo));

    // Read storage server information
    ssize_t bytes_read = recv(socket_fd, &info, sizeof(StorageServerInfo), 0);

    if (bytes_read <= 0)
    {
        perror("Failed to read from storage server socket");
        close(socket_fd);
        pthread_exit(NULL);
    }
    clear_socket_buffer(socket_fd);
    // Clear any remaining data in the socket buffer
    // char temp_buffer[1024]; // Temporary buffer for clearing
    // while (recv(socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT) > 0)
    // {
    //     // Discard any additional data in the buffer
    // }

    // if (errno != EAGAIN && errno != EWOULDBLOCK)
    // {
    //     perror("Error while clearing the socket buffer");
    // }
    // Lock before modifying the shared storage server list
    pthread_mutex_lock(&lock);
    if (storage_server_count < MAX_STORAGE_SERVERS)
    {
        // for (int i = 0; i < info.pathscount; i++)
        // {
        //     if (hashmap_insert(info.paths[i], &info) != 0)
        //     {
        //         fprintf(stderr, "Failed to insert path into HashMap: %s\n", info.paths[i]);
        //     }
        // }

        printf("Storage Server Registered:\n IP: %s, NM Port: %d, Client Port: %d, Paths Count: %d\n",
               info.ip, info.nm_port, info.client_port, info.pathscount);
        for (int i = 0; i < info.pathscount; i++)
        {
            // insertion into our trie
            // insertPath(trieRoot, info.paths[i], info.ip, info.client_port);
            printf(" Path[%d]: %s\n", i, info.paths[i]);
        }
        add_or_update_storage_server(&info, socket_fd);

        // for (int i = 0; i < info.pathscount; i++)
        // {
        //     // insertion into our trie
        //     insertPath(trieRoot, info.paths[i], info.ip, info.client_port);
        //     printf(" Path[%d]: %s\n", i, info.paths[i]);
        // }
        // storage_servers[storage_server_count] = info;
        // storage_server_sockets[storage_server_count++] = socket_fd;
        printf("Stored storage server socket :%d\n", socket_fd);
    }
    else
    {
        printf("Maximum storage server limit reached. Cannot register more servers.\n");
    }

    pthread_mutex_unlock(&lock);

    // close(socket_fd);
    pthread_exit(NULL);
}


void *handle_storage_server_connections(void *arg)
{
    memset(storage_servers, 0, sizeof(storage_servers));

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
    address.sin_port = htons(STORAGE_SERVER_PORT);

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

    printf("Storage Server listener started on port %d\n", STORAGE_SERVER_PORT);

    // Accept storage server registrations in a loop
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed for storage server");
            continue;
        }
        bookkeep(new_socket, 0, "Storage Server Connection", "Connection accepted");

        pthread_t thread_id;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = new_socket;
        // clear_socket_buffer(*socket_ptr);
        pthread_create(&thread_id, NULL, handle_storage_server, socket_ptr);
        pthread_detach(thread_id);
    }
    close(server_fd);
    pthread_exit(NULL);
}

void update_storage_server(int index, int new_socket_fd, StorageServerInfo *storageServer)
{
    // Close the old socket
    // printf("closing socket %d\n", storage_server_sockets[index]);
    close(storage_server_sockets[index]);

    // Update socket and port
    storage_server_sockets[index] = new_socket_fd;
    // storageServer[index].nm_port=storageServer->nm_port;

    // Update paths
    printf("Updating paths for server %s:%d\n", storage_servers[index].ip, storage_servers[index].client_port);
    printf("Old paths:\n");
    for (int i = storage_servers[index].pathscount - 1; i >= 0; i--)
    {
        // printf("i:%d path %s\n", i, storage_servers[index].paths[i]);
        if (strlen(storage_servers[index].paths[i]) > 0)
        { // Check if the path is non-empty
            printf("  %s\n", storage_servers[index].paths[i]);
            deletePath(trieRoot, storage_servers[index].paths[i]); // Ensure it doesn't free paths[i]
            // printf("next is going to delete\n");
        }
    }

    printf("New paths:\n");
    for (int i = 0; i < storageServer->pathscount; i++)
    {
        printf("  %s\n", storageServer->paths[i]);
        strcpy(storageServer[index].paths[i], storageServer->paths[i]);
        insertPath(trieRoot, storageServer->paths[i], storageServer->ip, storageServer->client_port);
    }

    // Remove paths that no longer exist
    // for (int i = 0; i < storageServer->pathscount; i++)
    // {
    //     strcpy(storageServer[index].paths[i], storageServer->paths[i]);
    // }

    // // Add new paths
    // int count = 0;
    // for (int i = 0; i < new_path_count; i++)
    // {
    //     if (!path_exists(&storage_servers[index], new_paths[i]))
    //     {
    //         strncpy(storage_servers[index].paths[count++], new_paths[i], MAX_PATH_LENGTH);
    //     }
    // }

    storage_servers[index].pathscount = storageServer->pathscount;
    printf("Paths updated successfully.\n");
}

int add_or_update_storage_server(StorageServerInfo *storageServer, int socket_fd)
{
    int index = find_storage_server(storageServer->ip, storageServer->client_port);

    if (index >= 0)
    {
        // Storage server exists, check for changes
        printf("Storage server already exists: %s:%d\n", storage_servers[index].ip, storage_servers[index].client_port);

        // Update paths and socket if necessary
        // if (memcmp(storage_servers[index].paths, storageServer->paths, storage_servers[index].pathscount * MAX_PATH_LENGTH) != 0)
        // {
        //     update_storage_server(index, socket_fd, storageServer);
        //     return 1; // Updated
        // }
        if (storage_servers[index].pathscount != storageServer->pathscount || !compare_2d_arrays(storage_servers[index].paths, storageServer->paths, storage_servers[index].pathscount))
        {
            update_storage_server(index, socket_fd, storageServer);
            bookkeep(socket_fd, 0, "Storage Server Update", "Storage server updated");
            return 1; // Updated
        }

        bookkeep(socket_fd, 0, "Storage Server Update", "Storage server No changes detected");
        printf("No changes detected for storage server.\n");
        printf("Closing socket %d\n", storage_server_sockets[index]);
        close(storage_server_sockets[index]); // Close the new socket as it's not needed
        storage_server_sockets[index] = socket_fd;
        return 0; // No update needed
    }

    // New storage server, add it
    if (storage_server_count >= MAX_STORAGE_SERVERS)
    {
        printf("Error: Maximum storage server limit reached.\n");
        printf("Closing socket %d\n", socket_fd);
        close(socket_fd); // Close the new socket as we can't add it
        return -1;
    }

    // Add new server and socket
    strncpy(storage_servers[storage_server_count].ip, storageServer->ip, INET_ADDRSTRLEN);
    storage_servers[storage_server_count].client_port = storageServer->client_port;
    storage_servers[storage_server_count].nm_port = storageServer->nm_port;
    storage_servers[storage_server_count].pathscount = storageServer->pathscount;
    printf("Checking if paths %d inserted at %d\n", storage_servers[storage_server_count].pathscount, storage_server_count);

    for (int i = 0; i < storageServer->pathscount; i++)
    {
        strncpy(storage_servers[storage_server_count].paths[i], storageServer->paths[i], MAX_PATH_LENGTH);
        printf("%s\n", storage_servers[storage_server_count].paths[i]);
    }

    storage_server_sockets[storage_server_count] = socket_fd;
    bookkeep(socket_fd, 0, "Storage Server Connection", "Storage server added");
    // // storage_server_count++;

    printf("New storage server added at index %d: %s:%d with %d paths\n",
           storage_server_count, storageServer->ip, storageServer->client_port, storageServer->pathscount);

    printf("checking loop condition storage server count:%d   at %d\n", storage_servers[storage_server_count].pathscount, storage_server_count);

    for (int i = 0; i < storage_servers[storage_server_count].pathscount; i++)
    {
        // insertion into our trie
        insertPath(trieRoot, storageServer->paths[i], storageServer->ip, storageServer->client_port);
        printf(" Path[%d]: %s\n", i, storage_servers[storage_server_count].paths[i]);
    }
    storage_server_count++;
    printf("Updated count:%d\n", storage_server_count);
    // storage_servers[storage_server_count] = info;

    return 0; // Added successfully
}


