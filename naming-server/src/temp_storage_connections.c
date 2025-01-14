#include "two_threads.h"
#include "log.h"
#include "tries.h"
#include "lru.h"

extern StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
extern int storage_server_sockets[MAX_STORAGE_SERVERS];
extern int storage_server_count;
extern pthread_mutex_t lock;
extern TrieNode *trieRoot;

extern int clients_waiting_count;
extern client_async clients_waiting[10];

void *handle_tempSS_server(void *arg)
{
    printf("Entered tempss server\n");
    int socket_fd = *(int *)arg;
    free(arg); // Free the memory allocated for the socket file descriptor

    while (1)
    {

        Async_Ack response;
        memset(&response, 0, sizeof(Async_Ack));
        printf("Waiting for packet\n");

        // Read client request
        ssize_t bytes_read = read(socket_fd, &response, sizeof(Async_Ack));
        clear_socket_buffer(socket_fd);
        printf("packet read\n");
        clear_socket_buffer(socket_fd);
        char ip[INET_ADDRSTRLEN];
        memset(ip, 0, INET_ADDRSTRLEN);
        get_ip_address(socket_fd, ip);
        if (bytes_read <= 0)
        {
            perror("Failed to read from client socket");

            Packet status;
            memset(&status, 0, sizeof(Packet));
            snprintf(status.data, BUFFER_LENGTH, "Status of Writing :Partial write");
            status.request_type = ERROR;
            printf("%s\n", ip);
            int index = -1;
            for (int i = 0; i < storage_server_count; i++)
            {
                // printf("%s\n", storage_servers[i].ip);
                if (strcmp(storage_servers[i].ip, ip) == 0)
                {
                    index = i;
                    break;
                }
            }

            for (int i = 0; i < clients_waiting_count; i++)
            {
                printf("%d\n", i);
                if (clients_waiting[i].storage_server == index)
                {
                    // printf("Ip client:%s\n", clients_waiting[i].ip);
                    send_message_to_host(ip, 8083, &status);
                    delete_client(i);
                }
            }

            close(socket_fd);
            pthread_exit(NULL);
            return NULL;
        }
        printf("Received acknowledgement : %s : %d,ip: %s,file: %s\n", response.string, response.err_code, response.ip, response.fpath);
        Packet status;
        memset(&status, 0, sizeof(Packet));
        snprintf(status.data, BUFFER_LENGTH, "Status of Writing %s", response.fpath);
        status.request_type = response.err_code;

        send_message_to_host(response.ip, 8083, &status);
        // delete_client()
        int index = -1;
        for (int i = 0; i < storage_server_count; i++)
        {
            if (strcmp(storage_servers[i].ip, ip) == 0)
            {
                index = i;
                break;
            }
        }

        for (int i = 0; i < clients_waiting_count; i++)
        {
            if (index != -1 && clients_waiting[i].storage_server == index)
            {
                // printf("Ip client:%s\n", clients_waiting[i].ip);
                // send_message_to_host(ip, 8083, &status);
                delete_client(i);
            }
        }
    }

    close(socket_fd);
    pthread_exit(NULL);
}

void *handle_temp_server_connections(void *arg)
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
    address.sin_port = htons(TEMP_SS_PORT);

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

    printf("Temporary Server listener started on port %d\n", TEMP_SS_PORT);

    // Accept storage server registrations in a loop
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed for storage server");
            continue;
        }

        pthread_t thread_id;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = new_socket;
        // clear_socket_buffer(*socket_ptr);
        pthread_create(&thread_id, NULL, handle_tempSS_server, socket_ptr);
        pthread_detach(thread_id);
    }
    close(server_fd);
    pthread_exit(NULL);
}
