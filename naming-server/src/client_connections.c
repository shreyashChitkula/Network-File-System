#include "two_threads.h"
#include "log.h"
#include "tries.h"
#include "lru.h"

extern StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
extern int storage_server_sockets[MAX_STORAGE_SERVERS];
extern int storage_server_count;
extern pthread_mutex_t lock;
extern TrieNode *trieRoot;

void *handle_client_request(void *arg)
{
    int socket_fd = *(int *)arg;
    free(arg); // Free the memory allocated for the socket file descriptor
    char response[BUFFER_LENGTH];

    while (1)
    {
        ClientMessage request;
        memset(&request, 0, sizeof(ClientMessage));

        // Read client request
        ssize_t bytes_read = read(socket_fd, &request, sizeof(ClientMessage));
        bookkeep(socket_fd, 0, "Client Request", "Request Received");
        clear_socket_buffer(socket_fd);
        if (bytes_read <= 0)
        {
            perror("Failed to read from client socket");
            close(socket_fd);
            pthread_exit(NULL);
            return NULL;
        }
        printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
               request.request_type, request.path, request.name, request.content, request.dest_path);

        pthread_mutex_lock(&lock);
        printf("cache before handling request\n");
        printCache();
        log_client_request(socket_fd, &request);
        switch (request.request_type)
        {
        case 0:
            break;
        case 1:
            handle_list_all_files(socket_fd);
            break;
        case 4:
            handle_create_request(socket_fd, &request);
            break;
        case 5:
            handle_delete_request(&request, socket_fd);
            break;
        case 6:
            copy(&request, socket_fd);
            break;
        default:
            // StorageServerInfo *result = hashmap_search(request.path);
            // int flag = 0;
            // for (int i = 0; i < storage_server_count; i++)
            // {
            //     for (int j = 0; j < storage_servers[i].pathscount; j++)
            //     {
            //         if (strcmp(storage_servers[i].paths[j], request.path))
            //         {
            //             flag = 1;
            //             snprintf(response, BUFFER_LENGTH, "IP: %s Port: %d", storage_servers[i].ip, storage_servers[i].client_port);
            //             break;
            //         }
            //     }
            // }

            // Send a response to the client
            // if (!flag)
            // {
            //     snprintf(response, BUFFER_LENGTH, "no such files found");
            // }
            // search fom trie
            char ip[INET_ADDRSTRLEN];
            int port;
            if (searchPath(trieRoot, request.path, ip, &port) == 0)
            {
                snprintf(response, BUFFER_LENGTH, "Error: Path not found");
            }
            else
            {
                snprintf(response, BUFFER_LENGTH, "IP: %s Port: %d", ip, port);
            }
            printf("%s\n", response);
            send(socket_fd, response, strlen(response), 0);
            char buffer[BUFFER_LENGTH];
            snprintf(buffer, BUFFER_LENGTH, "%d type request", request.request_type);
            if (request.request_type == 3 && request.flag == 0)
            {
                int idx = -1;
                for (int i = 0; i < storage_server_count; i++)
                {
                    if (strcmp(storage_servers[i].ip, ip) == 0)
                    {
                        idx = i;
                        break;
                    }
                }
                if (idx != -1)
                {
                    add_client(socket_fd, idx);
                }
            }
            bookkeep(socket_fd, 1, buffer, response);
            break;
        }

        pthread_mutex_unlock(&lock);

        // if (result == NULL)
        // {
        //     snprintf(response, BUFFER_LENGTH, "No such files found");
        // }
        // else
        // {
        //     snprintf(response, BUFFER_LENGTH, "IP: %s Port: %d", result->ip, result->client_port);
        // }

        // Send response to the client
        // printf("%s\n", response);
        // send(socket_fd, response, strlen(response), 0);
        printf("sent successfully\n");
    }

    close(socket_fd);
    pthread_exit(NULL);
}

void *handle_client_connections(void *arg)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket for client connections
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed for client");
        pthread_exit(NULL);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed for client");
        close(server_fd);
        pthread_exit(NULL);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(CLIENT_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed for client");
        close(server_fd);
        pthread_exit(NULL);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed for client");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("Client listener started on port %d\n", CLIENT_PORT);

    // Accept client requests in a loop
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed for client");
            continue;
        }
        bookkeep(new_socket, 0, "Client Connection", "Connection Connected");

        pthread_t thread_id;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = new_socket;

        pthread_create(&thread_id, NULL, handle_client_request, socket_ptr);
        pthread_detach(thread_id);
    }
    close(server_fd);
    pthread_exit(NULL);
}
