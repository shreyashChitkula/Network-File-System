#include "two_threads.h"
#include "log.h"
#include "tries.h"
#include "lru.h"

extern StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
extern int storage_server_sockets[MAX_STORAGE_SERVERS];
extern int storage_server_count;
// int clients_writing_async[10];
client_async clients_waiting[10];
int clients_waiting_count = 0;
extern pthread_mutex_t lock;
extern TrieNode *trieRoot;

void add_client(int socket, int idx)
{
    // printf("adding client\n");
    char ip[INET_ADDRSTRLEN];
    get_ip_address(socket, ip);
    strcpy(clients_waiting[clients_waiting_count].ip, ip);
    clients_waiting[clients_waiting_count].storage_server = idx;
    clients_waiting_count++;
}
void delete_client(int idx)
{
    // printf("deleting client\n");
    if (clients_waiting_count > 0)
    {
        for (int i = idx; i < clients_waiting_count - 1; i++)
        {
            strcpy(clients_waiting[i].ip, clients_waiting[i + 1].ip);
            clients_waiting[i].storage_server = clients_waiting[i + 1].storage_server;
        }
        clients_waiting_count--;
    }
}

void copy(ClientMessage *Clientmessage, int client_socket)
{
    printf("Entered copying\n");
    char response[BUFFER_LENGTH];
    int source = 0;
    int sourceIndex = -1;
    int destination = 0;
    int destinationIndex = -1;

    for (int i = 0; i < storage_server_count; i++)
    {
        if (path_exists(&storage_servers[i], Clientmessage->path))
        {
            source = 1;
            sourceIndex = i;
        }
        if (path_exists(&storage_servers[i], Clientmessage->dest_path))
        {
            destination = 1;
            destinationIndex = i;
        }
    }
    if (source == 0 || destination == 0)
    {
        snprintf(response, BUFFER_LENGTH, "Error: Source or Destination Path not found");
        snprintf(response, BUFFER_LENGTH, "Error: Source or Destination Path not found");
        send(client_socket, response, BUFFER_LENGTH, 0);
        bookkeep(client_socket, 1, "Copy Request", response);
        return;
    }
    if (sourceIndex == destinationIndex)
    {
        clear_socket_buffer(storage_server_sockets[sourceIndex]);
        printf("Sending Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
               Clientmessage->request_type, Clientmessage->path, Clientmessage->name, Clientmessage->content, Clientmessage->dest_path);
        send(storage_server_sockets[sourceIndex], Clientmessage, sizeof(ClientMessage), 0);
        bookkeep(storage_server_sockets[sourceIndex], 0, "Copy Request", "Request sent to storage server");
        ACK ack_response;
        ssize_t bytes_read;

        bytes_read = recv(storage_server_sockets[sourceIndex], &ack_response, sizeof(ACK), 0);
        bookkeep(storage_server_sockets[sourceIndex], 0, "Copy Request", "Received Ack from storage server");
        if (bytes_read <= 0)
        {
            perror("Failed to receive acknowledgment from storage server");
            snprintf(response, BUFFER_LENGTH, "Failed to receive acknowledgment from storage server");
            send(client_socket, response, BUFFER_LENGTH, 0);
            bookkeep(client_socket, 1, "Copy Request", response);
            return;
        }
        else
        {
            printf("Received acknowledgement:%d\n", ack_response.status);
            if (ack_response.status == 10)
            {
                snprintf(response, BUFFER_LENGTH, "Success:Created a copy of the file/folder");
                send(client_socket, response, BUFFER_LENGTH, 0);
                bookkeep(client_socket, 1, "Copy Request", response);
                add_path(&storage_servers[sourceIndex], Clientmessage->dest_path, extract_file_name(Clientmessage->path));
            }
            else
            {
                snprintf(response, BUFFER_LENGTH, "Error:Failed to copy the file/folder");
                send(client_socket, response, BUFFER_LENGTH, 0);
                bookkeep(client_socket, 1, "Copy Request", response);
            }
        }
    }
    else
    {
        ClientMessage request;
        memset(&request, 0, sizeof(ClientMessage));
        request.request_type = READ_REQ;
        // char srcPath[MAX_PATH_LENGTH];
        // strcpy(srcPath, Clientmessage->path);
        strncpy(request.path, Clientmessage->path, sizeof(request.path) - 1);
        request.path[sizeof(Clientmessage->path) - 1] = '\0'; // Ensure null-termination
        clear_socket_buffer(storage_server_sockets[sourceIndex]);
        send(storage_server_sockets[sourceIndex], &request, sizeof(ClientMessage), 0);
        bookkeep(storage_server_sockets[sourceIndex], 1, "Copy Request", "Request sent to storage server");
        int flag = 1;
        while (1)
        {
            Packet dataPacket;
            memset(&dataPacket, 0, sizeof(Packet));

            ssize_t bytes_read;
            bytes_read = recv(storage_server_sockets[sourceIndex], &dataPacket, sizeof(Packet), 0);
            printf("Received a packet\n");
            if (bytes_read <= 0)
            {
                perror("Failed to receive data packets from storage server");
                snprintf(response, BUFFER_LENGTH, "Error:Failed to copy the file/folder");
                send(client_socket, response, BUFFER_LENGTH, 0);
                return;
            }
            else
            {
                if (flag)
                {
                    ClientMessage server_request;
                    memset(&server_request, 0, sizeof(ClientMessage));
                    server_request.request_type = WRITE_REQ;
                    // strcpy(server_request.content, dataPacket.data);
                    strcpy(server_request.path, Clientmessage->dest_path);
                    strcpy(server_request.name, extract_file_name(Clientmessage->path));
                    send(storage_server_sockets[destinationIndex], &server_request, sizeof(ClientMessage), 0);
                    bookkeep(storage_server_sockets[destinationIndex], 1, "Copy Request", "Request sent to storage server");
                    flag--;
                    printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
                           server_request.request_type, server_request.path, server_request.name, server_request.content, server_request.dest_path);
                }

                if (dataPacket.request_type == SUCCESS)
                {
                    printf("sending packet\n");
                    send(storage_server_sockets[destinationIndex], &dataPacket, sizeof(Packet), 0);
                }
                else if (dataPacket.request_type == STOP)
                {
                    printf("sending packet\n");
                    send(storage_server_sockets[destinationIndex], &dataPacket, sizeof(Packet), 0);
                    snprintf(response, BUFFER_LENGTH, "Success:Created a copy of the file/folder");
                    send(client_socket, response, BUFFER_LENGTH, 0);
                    bookkeep(client_socket, 1, "Copy Request", response);
                    add_path(&storage_servers[destinationIndex], Clientmessage->dest_path, extract_file_name(Clientmessage->path));
                    break;
                }
                else
                {
                    perror("Failed to receive data packets from storage server");
                    snprintf(response, BUFFER_LENGTH, "Error:Failed to copy the file/folder");
                    send(client_socket, response, BUFFER_LENGTH, 0);
                    bookkeep(client_socket, 1, "Copy Request", response);
                    break;
                }
            }
        }
    }

    // printf("Source Server: %s\n", source_server);
    // printf("Destination Server: %s\n", destination_server);
}

// Function to update storage server details

// Function to add or update a storage server

// Function to clear the socket buffer

int handle_delete_request(ClientMessage *request, int client_socket)
{
    char response[BUFFER_LENGTH];
    memset(response, 0, sizeof(response));

    // Step 1: Validate the path in the request
    if (strlen(request->path) == 0)
    {
        snprintf(response, BUFFER_LENGTH, "Error: Invalid path");
        send(client_socket, response, strlen(response), 0);
        bookkeep(client_socket, 1, "Delete Request", response);
        return -1;
    }

    // Step 2: Find the storage server responsible for the path
    int server_index = -1;
    // int path_index = -1;

    // pthread_mutex_lock(&lock);
    // for (int i = 0; i < storage_server_count; i++)
    // {
    //     for (int j = 0; j < storage_servers[i].pathscount; j++)
    //     {
    //         if (strcmp(storage_servers[i].paths[j], request->path) == 0)
    //         {
    //             server_index = i;
    //             path_index = j;
    //             break;
    //         }
    //     }
    //     if (server_index != -1)
    //         break;
    // }
    // pthread_mutex_unlock(&lock);
    // search the path in our trie
    char ip[INET_ADDRSTRLEN];
    int port;
    if (searchPath(trieRoot, request->path, ip, &port) == 0)
    {
        snprintf(response, BUFFER_LENGTH, "Error: Path not found");
        send(client_socket, response, strlen(response), 0);
        bookkeep(client_socket, 1, "Delete Request", response);
        return -1;
    }
    // check which socert is owned by the server
    for (int i = 0; i < storage_server_count; i++)
    {
        if (strcmp(storage_servers[i].ip, ip) == 0 && storage_servers[i].client_port == port)
        {
            server_index = i;
            break;
        }
    }
    // get the path index
    // int path_index = -1;
    // for (int i = 0; i < storage_servers[server_index].pathscount; i++)
    // {
    //     if (strcmp(storage_servers[server_index].paths[i], request->path) == 0)
    //     {
    //         path_index = i;
    //         break;
    //     }
    // }

    // Step 3: Send the delete request to the appropriate storage server
    // request->request_type = DELETE_REQUEST;

    int server_socket = storage_server_sockets[server_index];
    bookkeep(server_socket, 1, "Delete Request", "Sending delete request to storage server");
    if (send(server_socket, request, sizeof(ClientMessage), 0) <= 0)
    {
        perror("Failed to send delete request to storage server");
        snprintf(response, BUFFER_LENGTH, "Error: Communication with storage server failed");
        send(client_socket, response, strlen(response), 0);
        bookkeep(client_socket, 1, "Delete Request", response);
        return -1;
    }
    clear_socket_buffer(server_socket);

    // Step 4: Wait for acknowledgment from the storage server
    ACK ack_response;
    memset(&ack_response, 0, sizeof(ACK));
    ssize_t bytes_read = recv(server_socket, &ack_response, sizeof(ACK), 0);
    bookkeep(server_socket, 0, "Delete Request", "Received acknowledgment from storage server");
    if (bytes_read <= 0)
    {
        perror("Failed to receive acknowledgment from storage server");
    }
    else if (bytes_read != sizeof(ACK))
    {
        printf("Mismatch in acknowledgment size: expected %lu, received %ld\n", sizeof(ACK), bytes_read);
    }
    else
    {
        // Convert from network to host byte order
        ack_response.status = ntohl(ack_response.status);
        printf("Acknowledgment received: Status = %d\n", ack_response.status);
    }

    // Step 5: Check acknowledgment and update the array
    if (ack_response.status == 10)
    {
        // Remove the path from the storage server's paths
        // pthread_mutex_lock(&lock);
        // for (int i = path_index; i < storage_servers[server_index].pathscount - 1; i++)
        // {
        //     strcpy(storage_servers[server_index].paths[i], storage_servers[server_index].paths[i + 1]);
        // }
        // storage_servers[server_index].pathscount--;
        // deletePath(trieRoot, request->path);
        // also delete in the trie data structure
        // also check if the path is a folder and delete all the files in it

        deletePath(trieRoot, request->path);
        for (int i = 0; i < storage_server_count; i++)
        {
            printf("storage server %d\n", i);
            // edit <  to <= for full checking , see if we get segmentation fault blame prajas
            for (int j = storage_servers[i].pathscount - 1; j >= 0; j--)
            {
                // if(strncmp(storage_servers[i].paths[j], request->path) == 0)
                // in yellow checking path
                printf("\033[0;33m");
                printf("Checking path %s\n", storage_servers[i].paths[j]);
                printf("\033[0m");
                if (strncmp(request->path, storage_servers[i].paths[j], strlen(request->path)) == 0)
                {
                    // printf("entered\n");
                    // print in pink what you're deleating
                    //    if(searchPath(trieRoot,storage_servers[i].paths[j],NULL,NULL)==1){
                    printf("entered\n");
                    // if(strcmp(request->path,storage_servers[i].paths[j])!=0){
                    printf("\033[1;35m");
                    printf("Deleting path %s\n", storage_servers[i].paths[j]);
                    printf("\033[0m");
                    printf("request path:%s\n", request->path);
                    // deletePath(trieRoot, storage_servers[i].paths[j]);
                    // }
                    //    }
                    for (int k = j; k < storage_servers[i].pathscount - 1; k++)
                    {
                        memset(storage_servers[i].paths[k], 0, sizeof(storage_servers[i].paths[k]));
                        strcpy(storage_servers[i].paths[k], storage_servers[i].paths[k + 1]);
                    }
                    storage_servers[i].pathscount--;
                    // j++;
                }
            }
        }
        printf("Updated paths:\n");
        for (int i = 0; i < storage_server_count; i++)
        {
            for (int j = 0; j < storage_servers[i].pathscount; j++)
            {
                printf("%s\n", storage_servers[i].paths[j]);
            }
        }

        // deletePath(trieRoot, request->path);

        // free the memory
        // freeTrie(getTrieNodeWithPath(trieRoot, request->path));

        // pthread_mutex_unlock(&lock);

        snprintf(response, BUFFER_LENGTH, "Success: Path %s deleted successfully", request->path);
    }
    else
    {
        snprintf(response, BUFFER_LENGTH, "Error: Unable to delete path %s", request->path);
    }

    // Send the final response to the client
    send(client_socket, response, strlen(response), 0);
    bookkeep(client_socket, 1, "Delete Request", response);
    return 0;
}

// Function to send create request to a storage server
int send_create_request_to_storage_server(int server_socket, ClientMessage *request)
{
    clear_socket_buffer(server_socket);
    printf("Entered sending\n");
    // Send the create request
    if (send(server_socket, request, sizeof(ClientMessage), 0) <= 0)
    {
        perror("Failed to send create request to storage server");
        return -1;
    }
    bookkeep(server_socket, 1, "Create Request", "Create request sent");
    // Listen for acknowledgment from storage server
    ACK ack_response;
    memset(&ack_response, 0, sizeof(ACK));

    ssize_t bytes_read = recv(server_socket, &ack_response, sizeof(ACK), 0);
    if (bytes_read <= 0)
    {
        perror("Failed to receive acknowledgment from storage server");
    }
    else if (bytes_read != sizeof(ACK))
    {
        printf("Mismatch in acknowledgment size: expected %lu, received %ld\n", sizeof(ACK), bytes_read);
    }
    else
    {
        // Convert from network to host byte order
        ack_response.status = ntohl(ack_response.status);
        printf("Acknowledgment received: Status = %d\n", ack_response.status);
    }

    // check if a word exists in a string
    // if (strstr(ack_response, "Success") != NULL)
    bookkeep(server_socket, 0, "Create Request", ack_response.status == 10 ? "Success" : "Failed");
    return ack_response.status == 10 ? 0 : -1;
}

// Function to handle creation of file/folder
void handle_create_request(int client_socket, ClientMessage *request)
{
    printf("Handling create\n");
    char response[BUFFER_LENGTH];
    memset(response, 0, sizeof(response));
    // int destination_found = 0; // Flag to track if the destination path is found
    StorageServerInfo *target_server = NULL;
    int target_socket = -1;

    // pthread_mutex_lock(&lock); // Lock the storage_servers array for thread-safe access

    // Search for the destination path in the registered storage servers
    // for (int i = 0; i < storage_server_count; i++)
    // {
    //     StorageServerInfo *server = &storage_servers[i];
    //     for (int j = 0; j < server->pathscount; j++)
    //     {
    //         if (strcmp(server->paths[j], request->path) == 0)
    //         {
    //             destination_found = 1;
    //             target_server = server;
    //             target_socket = storage_server_sockets[i];
    //             break;
    //         }
    //     }
    //     if (destination_found)
    //         break;
    // }
    // find the storage sever
    char ip[INET_ADDRSTRLEN];
    int port;
    if (searchPath(trieRoot, request->path, ip, &port) == 0)
    {
        printf("No storage server found\n");
        snprintf(response, BUFFER_LENGTH, "Error: Destination path not found.");
        send(client_socket, response, strlen(response), 0);
        // pthread_mutex_unlock(&lock);
        return;
    }
    // check which socert is owned by the server
    for (int i = 0; i < storage_server_count; i++)
    {
        if (strcmp(storage_servers[i].ip, ip) == 0 && storage_servers[i].client_port == port)
        {
            target_server = &storage_servers[i];
            target_socket = storage_server_sockets[i];
            break;
        }
    }

    // search in our trie
    printf("Found storage server\n");

    // Construct the full path for the new file or folder
    char full_path[BUFFER_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", request->path, request->name);

    printf("Full path:%s", full_path);
    // Check if the path already exists in the target server
    for (int j = 0; j < target_server->pathscount; j++)
    {
        if (strcmp(target_server->paths[j], full_path) == 0)
        {
            printf("Another file found with name same\n");
            snprintf(response, BUFFER_LENGTH, "Error: Path already exists.");
            send(client_socket, response, strlen(response), 0);
            // pthread_mutex_unlock(&lock);
            return;
        }
    }

    // Send the create request to the storage server
    printf("Trying to send to socket %d\n", target_socket);
    if (send_create_request_to_storage_server(target_socket, request) != 0)
    {
        snprintf(response, BUFFER_LENGTH, "Error: Failed to create on storage server.");
        send(client_socket, response, strlen(response), 0);
        bookkeep(client_socket, 0, "Create Request", "Error: Failed to create on storage server.");
        // pthread_mutex_unlock(&lock);
        return;
    }

    printf("Sent to storage server\n");

    // Add the new file/folder to the target server
    if (target_server->pathscount < 100) // Ensure there is space for more paths
    {
        // add to our trie
        printf("Inserting into trie\n");
        insertPath(trieRoot, full_path, target_server->ip, target_server->client_port);
        printf("Succesfully Inserted into trie {%s,%d}\n", target_server->ip, target_server->client_port);
        strncpy(target_server->paths[target_server->pathscount], full_path, BUFFER_LENGTH);
        target_server->pathscount++;
        bookkeep(client_socket, 0, "Create Request", "Success:  created on server");
        snprintf(response, BUFFER_LENGTH, "Success:  created on server");
    }
    else
    {
        bookkeep(client_socket, 0, "Create Request", "Error: Storage server is full.");
        snprintf(response, BUFFER_LENGTH, "Error: Storage server is full.");
    }

    // pthread_mutex_unlock(&lock); // Unlock the storage_servers array

    // Send response to the client
    send(client_socket, response, strlen(response), 0);
}
