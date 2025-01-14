#ifndef LRU_H
#define LRU_H


// Define the maximum size of the LRU cache
#define LRU_CACHE_SIZE 3

// Structure for each LRU node in the doubly-linked list
typedef struct LRUNode {
    TrieNode *trieNode;       // Pointer to the TrieNode
    char path[256];           // Path associated with the TrieNode
    struct LRUNode *prev;     // Pointer to the previous node
    struct LRUNode *next;     // Pointer to the next node
} LRUNode;

// Structure for the LRU cache
typedef struct {
    LRUNode *head;            // Head of the doubly-linked list (most recently used)
    LRUNode *tail;            // Tail of the doubly-linked list (least recently used)
    int size;                 // Current size of the cache
} LRUCache;

// Global LRU Cache
extern LRUCache cache;

// Function prototypes
void addToCache(TrieNode *node, const char *path);
TrieNode *searchCache(const char *path);
void freeCache();
void remove_from_cache(const char *path);
void printCache();

#endif // LRU_H
