#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "two_threads.h"
#include "tries.h"
#include "log.h"

extern pthread_mutex_t lock;
TrieNode *trieRoot;

// void initialize()
// {
//     storage_servers = malloc(sizeof(storage_servers));
// }

int main()
{
    // Initialize mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Mutex initialization failed");
        return 1;
    }
    // if (hashmap_init() != 0) {
    //     fprintf(stderr, "HashMap initialization failed.\n");
    //     exit(1);
    // }
    trieRoot = createTrieNode('\0', NULL, -1);

    pthread_t storage_server_thread, client_thread, temp_thread;
    // extern storage_servers;
    // memset(storage_servers, 0, sizeof(storage_servers));

    // Start storage server listener thread
    if (pthread_create(&storage_server_thread, NULL, handle_storage_server_connections, NULL) != 0)
    {
        perror("Failed to create storage server thread");
        return 1;
    }

    // Start client listener thread
    if (pthread_create(&client_thread, NULL, handle_client_connections, NULL) != 0)
    {
        perror("Failed to create client thread");
        return 1;
    }
    if (pthread_create(&temp_thread, NULL, handle_temp_server_connections, NULL) != 0)
    {
        perror("Failed to create storage server thread");
        return 1;
    }

    // Wait for threads to complete
    pthread_join(storage_server_thread, NULL);
    pthread_join(client_thread, NULL);

    // Destroy mutex
    pthread_mutex_destroy(&lock);
    freeTrie(trieRoot);

    return 0;
}
