#include "headers.h"

pthread_mutex_t file_locks[MAX_PATHS];
int nm_sock;
int temp_nm_sock;


void *handle_req(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg); // Free the memory allocated for the client socket pointer

    MSGtoSS msg;
    ssize_t bytes_received;

    // Receive the MSGtoSS structure from the client
    bytes_received = recv(client_socket, &msg, sizeof(MSGtoSS), 0);
    if (bytes_received <= 0)
    {
        perror(RED "Failed to receive message from client" RESET);
        close(client_socket);
        pthread_exit(NULL);
    }

    t_request response;
    memset(&response, 0, sizeof(t_request)); // Initialize response struct
    response.request_type = ERROR;           // Default to ERROR; set to SUCCESS if an operation completes successfully

    int flag = 0;

    async msgforw;

    // Handle the request based on the request_type
    switch (msg.request_type)
    {
    case READ_REQ:
    {
        printf(YELLOW "READ REQ received\n" RESET);
        FILE *file = fopen(msg.path, "r");
        if (file == NULL)
        {
            perror("File read error");
            snprintf(response.data, sizeof(response.data), "Failed to read file: %s", strerror(errno));
            send(client_socket, &response, sizeof(t_request), 0);
        }
        else
        {
            response.request_type = SUCCESS;
            size_t bytes_read;
            // Read and send file data in chunks until the end of the file
            while ((bytes_read = fread(response.data, 1, MAX_DATA_LEN, file)) > 0)
            {
                // printf("%s",response.data);
                if (send(client_socket, &response, sizeof(t_request), 0) == -1)
                {
                    perror("Failed to send file data to client");
                    break;
                }
                memset(response.data, 0, MAX_DATA_LEN); // Clear the data buffer
                response.request_type = SUCCESS;
                usleep(1000);
            }

            response.request_type = STOP;
            send(client_socket, &response, sizeof(t_request), 0);
            fclose(file);
        }
        break;
    }

    case INFO_REQ:
    {
        printf(YELLOW "INFO REQ received\n" RESET);

        get_file_info(msg.path, response.data, sizeof(response.data));

        response.request_type = (strstr(response.data, "Failed to get file info") != NULL) ? ERROR : SUCCESS;

        // Send the metadata to the client
        if (send(client_socket, &response, sizeof(t_request), 0) == -1)
        {
            perror("Failed to send file info to client");
        }
        break;
    }

    case WRITE_REQ:
    {
        flag = 0;
        printf(YELLOW "WRITE REQ received\n" RESET);

        int lock_index = get_lock_index(msg.path);

        // Lock associated with the file
        pthread_mutex_lock(&file_locks[lock_index]);

        FILE *file = fopen(msg.path, "w");
        if (file == NULL)
        {
            perror("File write error");
            snprintf(response.data, sizeof(response.data), "Failed to write to file: %s", strerror(errno));
            response.request_type = ERROR;
            send(client_socket, &response, sizeof(t_request), 0);
            pthread_mutex_unlock(&file_locks[lock_index]);
        }
        else
        {
            size_t content_length = strlen(msg.content);
            size_t chunk_size = 1024; // Adjust this value based on your needs (size of each chunk to write)
            size_t bytes_written = 0;

            // Write in chunks and send acknowledgment after the first chunk
            while (bytes_written < content_length)
            {
                // Calculate the remaining bytes to write
                size_t bytes_to_write = (content_length - bytes_written > chunk_size) ? chunk_size : (content_length - bytes_written);

                // Write the chunk of data to the file
                size_t result = fwrite(msg.content + bytes_written, 1, bytes_to_write, file);
                if (result < bytes_to_write)
                {
                    if (ferror(file))
                    {
                        perror(RED "Error writing to file");
                        // snprintf(response.data, sizeof(response.data), "Error writing to file.");
                        flag = 1;
                        break;
                    }
                    // else
                    // {
                    //     printf(RED "Partial write occurred.\n");
                    //     snprintf(response.data, sizeof(response.data), "Partial write occurred.");
                    //     flag = 2;
                    //     break;
                    // }
                }
                bytes_written += bytes_to_write;

                // Send acknowledgment after the first chunk of data is written
                if (bytes_written <= chunk_size && !msg.sync_bit) // Check if we are in the first chunk
                {
                    response.request_type = SUCCESS;
                    snprintf(response.data, sizeof(response.data), "First chunk written successfully.");
                    send(client_socket, &response, sizeof(t_request), 0);
                }
            }

            pthread_mutex_unlock(&file_locks[lock_index]);

            // Finalize and send the success message

            printf("value of flag %d\n", flag);
            strcpy(msgforw.fpath, msg.path);
            printf("copied %s to message\n", msgforw.fpath);

            if (!msg.sync_bit)
            {
                if (flag)
                {
                    msgforw.err_code = ERROR;
                    snprintf(msgforw.string, sizeof(msgforw.string), "Error writing file.");

                    set_socket_info(client_socket, &msgforw, 1);

                    ssize_t bytes_sent = send(temp_nm_sock, &msgforw, sizeof(async), 0);

                    if (bytes_sent <= 0)
                    {
                        perror("send failed");
                    }
                    else
                    {
                        printf("Successfully sent error writing %ld bytes msgforw_type %d\n", bytes_sent, msgforw.err_code);
                    }
                }
                else
                {
                    msgforw.err_code = SUCCESS;
                    snprintf(msgforw.string, sizeof(msgforw.string), "File written successfully.");

                    set_socket_info(client_socket, &msgforw, 1);

                    ssize_t bytes_sent = send(temp_nm_sock, &msgforw, sizeof(async), 0);

                    if (bytes_sent <= 0)
                    {
                        perror("send failed");
                    }
                    else
                    {
                        printf("Successfully sent %ld bytes msgforw_type %d\n", bytes_sent, msgforw.err_code);
                    }
                }
            }
            else if (flag)
            {
                response.request_type = ERROR;
                snprintf(response.data, sizeof(response.data), "Error writing file.");
                send(client_socket, &response, sizeof(t_request), 0);
            }
            else
            {
                response.request_type = SUCCESS;
                snprintf(response.data, sizeof(response.data), "written successfully.");
                send(client_socket, &response, sizeof(t_request), 0);
            }
            // else
            // {
            //     response.request_type = ERROR;
            //     snprintf(response.data, sizeof(response.data), "Partial Write occured.");

            //     ssize_t bytes_sent = send(temp_nm_sock, &response, sizeof(async), 0);

            //     if (bytes_sent <= 0)
            //     {
            //         perror("send failed");
            //     }
            //     else
            //     {
            //         printf("Successfully sent partial write %ld bytes response_type %d\n", bytes_sent, response.request_type);
            //     }
            // }
            fclose(file);
        }
        break;
    }

    case STREAM_REQ:
    {
        printf(YELLOW "STREAM REQ received\n" RESET);
        FILE *music_file = fopen(msg.path, "rb"); // Open the music file in binary mode
        if (music_file == NULL)
        {
            perror("Music stream error");
            snprintf(response.data, sizeof(response.data), "Failed to open music file: %s", strerror(errno));
            send(client_socket, &response, sizeof(t_request), 0);
        }
        else
        {
            response.request_type = SUCCESS;
            size_t bytes_read;

            printf(GREEN "Streaming music to client...\n" RESET);

            // Send data in chunks
            while ((bytes_read = fread(response.data, 1, MAX_DATA_LEN, music_file)) > 0)
            {
                if (send(client_socket, response.data, bytes_read, 0) == -1)
                {
                    perror("Failed to send music data to client");
                    break;
                }
                memset(response.data, 0, MAX_DATA_LEN); // Clear the data buffer
            }

            printf(GREEN "Finished streaming music to client.\n" RESET);
            fclose(music_file);
        }
        break;
    }

    default:
        snprintf(response.data, sizeof(response.data), "Unknown request type: %d", msg.request_type);
        send(client_socket, &response, sizeof(t_request), 0);
        break;
    }

    // Close the client connection
    close(client_socket);
    pthread_exit(NULL);
}

void *listen_client()
{
    // SStoNM *info_ss = (SStoNM *)args;
    int server_fd, *new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror(RED "Socket creation failed" RESET);
        pthread_exit(NULL);
    }

    // Configure the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(CLIENT_PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        pthread_exit(NULL);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf(CYAN "Server listening on port %d for client connections...\n" RESET, CLIENT_PORT);

    while (1)
    {
        // Accept an incoming connection
        new_socket = malloc(sizeof(int)); // Allocate memory for the client socket descriptor
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        printf(GREEN "Connected to client!\n" RESET);

        // Create a thread to handle the client's request
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_req, (void *)new_socket) != 0)
        {
            perror(RED "Failed to create client handler thread" RESET);
            free(new_socket);
            continue;
        }

        // Detach the thread since we don't need to join it later
        pthread_detach(client_thread);
    }

    close(server_fd);
    pthread_exit(NULL);
}
void *listen_naming_server(void *arg)
{
    int server_sock = *(int *)arg;
    free(arg);

    printf(CYAN "Listening for requests from the Naming Server...\n" RESET);

    printf("sock in lns:%d  and nm sock :%d\n", server_sock, nm_sock);

    while (1)
    {
        clear_socket_buffer(server_sock);

        MSGtoSS msg;
        ssize_t bytes_received;

        // Receive a request from the Naming Server
        bytes_received = recv(server_sock, &msg, sizeof(MSGtoSS), 0);
        if (bytes_received <= 0)
        {
            perror(RED "Failed to receive message from Naming Server" RESET);
            close(server_sock);
            pthread_exit(NULL);
        }

        // Allocate memory for the request and create a thread to handle it
        // MSGtoSS *msg_ptr = malloc(sizeof(MSGtoSS));
        // if (msg_ptr == NULL)
        // {
        //     perror(RED "Memory allocation failed for message" RESET);
        // }
        // memcpy(msg_ptr, &msg, sizeof(MSGtoSS));

        printf("sock hrn:%d\n", server_sock);

        printf(GREEN "Handling request from Naming Server...\n" RESET);
        printf("Request type: %d\n", msg.request_type);
        printf("Name: %s\n", msg.name);
        printf("Content: %s\n", msg.content);
        printf("Destination Path: %s\n", msg.dest_path);
        printf("Path: %s\n", msg.path);

        // Process the request
        // t_request response;
        // memset(&response, 0, sizeof(t_request)); // Initialize response struct
        // response.request_type = ERROR;           // Default to ERROR; set to SUCCESS if an operation completes successfully
        // ack acknowledgment;
        // memset(&acknowledgment, 0, sizeof(ack));

        t_request response;
        memset(&response, 0, sizeof(t_request)); // Initialize response struct
        response.request_type = ERROR;           // Default to ERROR; set to SUCCESS if an operation completes successfully

        ack acknowledgment;
        memset(&acknowledgment, 0, sizeof(ack));

        // int flag = 0;

        char dest_path[MAX_PATH_LEN * 2];

        switch (msg.request_type)
        {
        case COPY_REQ:
        {

            acknowledgment.ack_no = SUCCESS;
            printf(YELLOW "COPY REQ RECEIVED\n");

            char *source = extract_file_name(msg.path);

            snprintf(dest_path, MAX_PATH_LEN * 2, "%s/%s", msg.dest_path, source);

            printf("source %s and destination %s\n", msg.path, dest_path);

            FILE *src = fopen(msg.path, "r");
            if (src == NULL)
            {
                perror(RED "Error opening source file source");
                printf(RED "source:%s\n", msg.path);
                acknowledgment.ack_no = ERROR;
            }

            FILE *dest = fopen(dest_path, "w");
            if (dest == NULL)
            {
                perror("Error opening destination file");
                printf(RED "Destination:%s\n", msg.dest_path);
                fclose(src);
                acknowledgment.ack_no = ERROR;
            }

            char buffer[1024];
            size_t bytes;

            // Copy data from source to destination
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0)
            {
                if (fwrite(buffer, 1, bytes, dest) != bytes)
                {
                    perror("Error writing to destination file");
                    fclose(src);
                    fclose(dest);
                    acknowledgment.ack_no = ERROR;
                }
            }

            if (ferror(src))
            {
                perror("Error reading from source file");
                fclose(src);

                fclose(dest);
                acknowledgment.ack_no = ERROR;
            }

            if (acknowledgment.ack_no != ERROR)
            {
                fclose(src);
                fclose(dest);
            }

            send(server_sock, &acknowledgment, sizeof(ack), 0);
            break;
        }
        case CREATE_REQ:
        {
            printf(YELLOW "CREATE REQ received\n" RESET);

            // Construct the full path
            char full_path[MAX_PATH_LEN * 2];

            // Initialize the acknowledgment struct, default ack_no = 0
            acknowledgment.ack_no = 0;

            // Check if the name is null or empty
            if (strchr(msg.name, '.') == NULL)
            {
                // Construct the full path for the directory
                snprintf(full_path, sizeof(full_path), "%s/%s", msg.path, msg.name);

                // Check if the directory already exists
                struct stat statbuf;
                if (stat(full_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
                {
                    printf(RED "Directory already exists\n" RESET);
                    acknowledgment.ack_no = ERROR;
                }
                else
                {
                    // Create the folder specified by the path
                    if (mkdir(full_path, 0777) == 0) // 0777 permissions for the new directory
                    {
                        printf(GREEN "Created a folder!!\n" RESET);
                        acknowledgment.ack_no = SUCCESS; // Success
                    }
                    else
                    {
                        perror(RED "Directory creation error" RESET);
                        acknowledgment.ack_no = ERROR;
                    }
                }
            }
            else
            {
                // Construct the full path for the file
                snprintf(full_path, sizeof(full_path), "%s/%s", msg.path, msg.name);

                // Check if the file already exists
                struct stat statbuf;
                if (stat(full_path, &statbuf) == 0)
                {
                    printf(RED "File already exists\n" RESET);
                    acknowledgment.ack_no = ERROR;
                }
                else
                {
                    // Create the file
                    FILE *file = fopen(full_path, "w");
                    if (file == NULL)
                    {
                        perror(RED "File creation error" RESET);
                        acknowledgment.ack_no = ERROR;
                    }
                    else
                    {
                        printf(GREEN "Created file!!\n" RESET);
                        acknowledgment.ack_no = SUCCESS; // Success

                        fclose(file);
                    }
                }
            }
            // Send acknowledgment back (as a struct)
            printf("ack here %d\n", acknowledgment.ack_no);

            acknowledgment.ack_no = htonl(acknowledgment.ack_no);

            ssize_t bytes_sent = send(server_sock, &acknowledgment, sizeof(ack), 0);
            if (bytes_sent < 0)
            {
                perror(RED "Failed to send acknowledgment" RESET);
            }
            else
            {
                printf(GREEN "Acknowledgment sent (size %ld): %d\n" RESET, bytes_sent, acknowledgment.ack_no);
            }
            break;
        }

        case DELETE_REQ:
        {
            char ackstring[MAX_BUFFER];
            printf(YELLOW "DELETE REQ received\n" RESET);

            struct stat path_stat;

            // Check if the path exists and get its information
            if (stat(msg.path, &path_stat) == 0)
            {
                if (S_ISREG(path_stat.st_mode))
                {
                    // It's a regular file
                    if (remove(msg.path) == 0)
                    {
                        strcpy(ackstring, "File deleted successfully :1");
                        acknowledgment.ack_no = SUCCESS;
                    }
                    else
                    {
                        perror(RED "File deletion error" RESET);
                        strcpy(ackstring, "Failed to delete file :0");
                        acknowledgment.ack_no = ERROR;
                    }
                }
                else if (S_ISDIR(path_stat.st_mode))
                {
                    // It's a directory
                    if (delete_directory_recursively(msg.path) == 0)
                    {
                        strcpy(ackstring, "Directory deleted successfully :1");
                        acknowledgment.ack_no = SUCCESS;
                    }
                    else
                    {
                        strcpy(ackstring, "Failed to delete directory :0");
                        acknowledgment.ack_no = ERROR;
                    }
                }
                else
                {
                    // Not a regular file or directory
                    fprintf(stderr, RED "Path is neither a file nor a directory\n" RESET);
                    strcpy(ackstring, "Invalid path :0");
                    acknowledgment.ack_no = ERROR;
                }
            }
            else
            {
                perror(RED "Error getting path information" RESET);
                strcpy(ackstring, "Path does not exist or inaccessible :0");
                acknowledgment.ack_no = ERROR;
            }

            // Send acknowledgment
            acknowledgment.ack_no = htonl(acknowledgment.ack_no);
            ssize_t bytes_sent = send(server_sock, &acknowledgment, sizeof(acknowledgment), 0);
            if (bytes_sent < 0)
            {
                perror("Failed to send acknowledgment");
            }
            else
            {
                printf("acknowledgment (size %ld): %d   %d\n", bytes_sent, acknowledgment.ack_no, ntohl(acknowledgment.ack_no));
            }

            break;
        }
        case READ_REQ:
        {
            printf(YELLOW "READ REQ TO COPY received\n" RESET);
            printf("Reading %s\n", msg.path);
            FILE *file = fopen(msg.path, "rb");
            if (file == NULL)
            {
                perror("File read error");
                snprintf(response.data, sizeof(response.data), "Failed to read file: %s", strerror(errno));
                send(server_sock, &response, sizeof(t_request), 0);
            }
            else
            {
                response.request_type = SUCCESS;
                size_t bytes_read;
                // Read and send file data in chunks until the end of the file
                while ((bytes_read = fread(response.data, 1, MAX_DATA_LEN, file)) > 0)
                {
                    // printf("%s",response.data);
                    if (send(server_sock, &response, sizeof(t_request), 0) == -1)
                    {
                        perror("Failed to send file data to client");
                        break;
                    }
                    memset(response.data, 0, MAX_DATA_LEN); // Clear the data buffer
                    response.request_type = SUCCESS;
                    usleep(1000);
                }

                response.request_type = STOP;
                send(server_sock, &response, sizeof(t_request), 0);
                fclose(file);
            }
            break;
        }

        case WRITE_REQ:
        {
            printf(YELLOW "write recevied for copy\n");

            snprintf(dest_path, MAX_PATH_LEN * 2, "%s/%s", msg.path, msg.name);

            printf("wrinting in %s\n", dest_path);
            t_request response;
            FILE *output_file = fopen(dest_path, "wb");
            if (!output_file)
            {
                perror("Error opening output file");
                break;
            }

            while (1)
            {
                // Receive data from the server
                clear_socket_buffer(server_sock);
                printf("Receiving file data...\n");
                ssize_t bytes_received = recv(server_sock, &response, sizeof(t_request), 0);
                if (bytes_received <= 0)
                {
                    perror("Error receiving data from server");
                    break;
                }

                // Check the request type
                if (response.request_type == STOP)
                {
                    printf("STOP signal received. File transfer complete.\n");
                    break;
                }
                else if (response.request_type == SUCCESS)
                {
                    // Write received data to the output file
                    size_t written = fwrite(response.data, 1, strlen(response.data), output_file);
                    if (written != strlen(response.data))
                    {
                        perror("Error writing to file");
                    }
                    printf("Chunk received: %s\n", response.data);
                }
                else
                {
                    printf("Unknown response type: %d\n", response.request_type);
                    break;
                }

                // Clear the response data buffer for the next chunk
                memset(response.data, 0, MAX_DATA_LEN);
            }

            fclose(output_file);
            printf("File saved as 'received_file.txt'.\n");

            break;
        }
        default:
            // snprintf(response.data, sizeof(response.data), "Unknown request type: %d", msg.request_type);
            break;
        }

        // free(msg. // Free the dynamically allocated memory for the request
        // free(arg);
    }
    pthread_exit(NULL);
}
// CONNECT FOR ASYNCHRONOUS MSG
int connect_to_naming_server_async()
{
    int sock = 0;
    struct sockaddr_in server_address;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TEMP_NS_PORT);

    // Convert IPv4/IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, NAMING_SERVER_IP, &server_address.sin_addr) <= 0)
    {
        perror("Invalid address or address not supported");
        close(sock);
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    temp_nm_sock = sock;

    printf("Connected to naming server at %s:%d\n", NAMING_SERVER_IP, TEMP_NS_PORT);

    // Send and receive data example (optional)
    // const char *message = "Hello, naming server!";
    // send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);

    return 0;
}

// Function to send registration data to the Naming Server
int register_with_naming_server(SStoNM *info_ss)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Initialize socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(RED "Socket creation error" RESET);
        return -1;
    }

    nm_sock = sock;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NAMING_SERVER_PORT);

    // Convert IP addresses from text to binary form
    if (inet_pton(AF_INET, NAMING_SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        perror(RED "Invalid address or address not supported" RESET);
        close(sock);
        return -1;
    }

    // Connect to the Naming Server
    while (1)
    {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            // Could not connect
            continue;
        }
        // Connected
        break;
    }

    int access = 0;                                                   // Initialize as integer, not a pointer
    get_all_paths("./SS_folder", info_ss->accessible_paths, &access); // Pass address of access
    info_ss->path_count = access;

    // Send the registration message
    if (send(sock, info_ss, sizeof(*info_ss), 0) < 0)
    {
        perror(RED "Failed to send registration message" RESET);
        close(sock);
        return -1;
    }
    printf(BLUE "Registration message sent to Naming Server.\n" RESET);

    for (int i = 0; i < access; i++)
    {
        printf("%s\n", info_ss->accessible_paths[i]);
    }

    // Keep the socket open and listen for requests
    pthread_t listen_thread;
    int *sock_ptr = malloc(sizeof(int));
    *sock_ptr = sock;

    printf("sock in register:%d\n", sock);

    if (pthread_create(&listen_thread, NULL, listen_naming_server, (void *)sock_ptr) != 0)
    {
        perror(RED "Failed to create Naming Server listening thread" RESET);
        free(sock_ptr);
        close(sock);
        return -1;
    }

    pthread_detach(listen_thread); // Detach the thread to allow it to run independently
    return 0;
}
int main(int argc, char *argv[])
{
    // Uncomment this block if you want to use command line arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <storage_ip> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (connect_to_naming_server_async() == -1)
    {
        printf(RED "Error connecting for asycn with naming\n");
    }

    char *storage_ip = argv[1];
    int port_nm = NAMING_SERVER_PORT;
    int port_client = CLIENT_PORT;

    SStoNM *info_ss;
    info_ss = (SStoNM *)malloc(sizeof(SStoNM));
    if (info_ss == NULL)
    {
        perror(RED "Failed to allocate memory" RESET);
        exit(EXIT_FAILURE);
    }

    // Use memset to zero out the allocated memory
    memset(info_ss, 0, sizeof(SStoNM));

    info_ss->port_nm = port_nm;
    info_ss->port_client = port_client;
    strcpy(info_ss->ip, storage_ip);

    // Register with the Naming Server
    if (register_with_naming_server(info_ss) == 0)
    {
        printf(GREEN "Storage Server initialized and registered successfully.\n" RESET);
    }
    else
    {
        printf(RED "Failed to register Storage Server with Naming Server.\n" RESET);
        exit(EXIT_FAILURE);
    }

    // Create a thread to listen for client connections
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, listen_client, NULL) != 0)
    {
        perror(RED "Failed to create client listening thread" RESET);
        free(info_ss);
        exit(EXIT_FAILURE);
    }
    // pthread_t naming_thread;
    // if (pthread_create(&naming_thread, NULL, listen_naming_server, NULL) != 0)
    // {
    //     perror(RED "Failed to create client listening thread" RESET);
    //     free(info_ss);
    //     exit(EXIT_FAILURE);
    // }

    // Optionally join the thread if you need to wait for it, or you can detach it
    pthread_join(client_thread, NULL);
    // pthread_join(naming_thread, NULL);

    free(info_ss); // Don't forget to free the allocated memory
    return 0;
}

// I have added the read req and write req
// Now the naming server should implement read and write for the paths given by client
// it will check which storage server has source and read from it
// and it will check which storage server has destination path and then write to it
