#include "header.h"

int ns_socket; // Global NS connection

int main()
{
    // Establish connection with NS at startup
    pthread_t temp_receiver_thread;

    ns_socket = connect_with_ns();
    pthread_create(&temp_receiver_thread, NULL, receiver, NULL);

    if (ns_socket < 0)
    {
        printf("Error: Could not connect to naming server\n");
        return 1;
    }
    printf("Successfully connected to Naming Server!\n");
    while (1)
    {

        clear_socket_buffer(ns_socket);
        char input[1024];
        char *argv[4];
        int argc = 0;

        printf("\nEnter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < 4)
        {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        if (argc < 1)
            continue; // Empty input

        char *command = argv[0];
        // print("command %s\n",command);

        if (strcmp(command, "read") == 0)
        {
            if (argc != 2)
            {
                printf("Usage: read <filepath>\n");
                continue;
            }
            read_operation(ns_socket, argv[1]);
        }
        else if (strcmp(command, "info") == 0)
        {
            if (argc != 2)
            {
                printf("Usage: info <filepath>\n");
                continue;
            }
            info_operation(ns_socket, argv[1]);
        }
        else if (strcmp(command, "write") == 0)
        {
            if (argc != 2)
            {
                printf("Usage: write <filepath>\n");
                continue;
            }

            char content[1024]; // Adjust size as needed
            char temp[256];     // Temporary buffer for each line of input
            content[0] = '\0';  // Initialize content as an empty string

            printf("Enter content (Ctrl+D to finish):\n");

            while (fgets(temp, sizeof(temp), stdin) != NULL)
            {
                // Append the input to the content buffer
                if (strlen(content) + strlen(temp) >= sizeof(content))
                {
                    printf("Buffer overflow. Cannot add more content.\n");
                    break;
                }
                strcat(content, temp); // Append the current input to the main content
            }

            if (feof(stdin))
            {
                clearerr(stdin); // Clear the EOF flag to allow the main loop to continue
                printf("\nFinal content:\n%s\n", content);
            }
            else
            {
                printf("Error reading content.\n");
                continue;
            }

            printf("SYNC(1) or ASYNC(0):");
            int bit = 0;
            scanf("%d", &bit);
            // if (!bit)
            // {
            //     pthread_create(&temp_receiver_thread, NULL, receiver, NULL);
            // }
            // content[strcspn(content, "\n")] = 0;
            write_operation(ns_socket, argv[1], content, bit);
        }
        else if (strcmp(command, "list") == 0)
        {
            list_operation(ns_socket);
        }
        else if (strcmp(command, "exit") == 0)
        {
            printf("Closing connections with Naming Server...\n");
            close(ns_socket);
            return 0;
        }
        else if (strcmp(command, "create") == 0)
        {
            if (argc != 3)
            {
                printf("Usage: create <dirpath> <name>\n");
                printf("Examples:\n");
                printf("  create ./folder1 newfile.txt    # Create a file\n");
                printf("  create ./folder1 newfolder      # Create a directory\n");
                continue;
            }
            create_operation(ns_socket, argv[1], argv[2]);
        }
        else if (strcmp(command, "delete") == 0)
        {
            if (argc != 2)
            {
                printf("Usage: delete <filepath>\n");
                continue;
            }

            delete_operation(ns_socket, argv[1]);
        }
        else if (strcmp(command, "copy") == 0)
        {
            if (argc != 3)
            {
                printf("Usage: copy <source> <destination>\n");
                continue;
            }
            copy_operation(ns_socket, argv[1], argv[2]);
        }
        else if (strcmp(command, "stream") == 0)
        {
            if (argc != 2)
            {
                printf("Usage: stream <filepath>\n");
                continue;
            }
            stream_operation(ns_socket, argv[1]);
        }
        else
        {
            printf("Unknown command: %s\n", command);
            continue;
        }
    }
    return 0;
    pthread_join(temp_receiver_thread, NULL);
}
