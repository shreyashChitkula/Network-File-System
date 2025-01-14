
#include "two_threads.h"
#include "tries.h"
#include "lru.h"
#include "log.h"
// #include "hashmap.h"


StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
int storage_server_sockets[MAX_STORAGE_SERVERS];
int storage_server_count = 0;
pthread_mutex_t lock;
