#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tries.h"
#include "lru.h"
// Define the maximum size of the LRU cache

// Global LRU Cache
LRUCache cache = {NULL, NULL, 0};
extern TrieNode *trieRoot;
// Helper function to create a new LRU node
LRUNode *createLRUNode(TrieNode *node, const char *path)
{
    if (!node)
    {
        fprintf(stderr, "Error: Attempting to create LRUNode with NULL TrieNode\n");
        return NULL;
    }
    LRUNode *newNode = (LRUNode *)malloc(sizeof(LRUNode));
    if (!newNode)
    {
        fprintf(stderr, "Error: Memory allocation failed for LRUNode\n");
        return NULL;
    }
    newNode->trieNode = node;
    strncpy(newNode->path, path, sizeof(newNode->path));
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

// Move an LRU node to the head of the cachef
void moveToHead(LRUNode *node)
{
    if (!node || !cache.head)
        return; // Validate the node and cache
    if (cache.head == node)
        return; // Already at head, no need to move

    // Safe removal and update of pointers
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;

    if (cache.tail == node)
        cache.tail = node->prev; // Update tail if needed

    node->next = cache.head;
    node->prev = NULL;
    if (cache.head)
        cache.head->prev = node;
    cache.head = node;
}

// Add a new TrieNode to the LRU cache
void addToCache(TrieNode *node, const char *path)
{
    // Check if the cache is full
    printf("cache size:%d/%d\n", cache.size, LRU_CACHE_SIZE);
    if (cache.size == LRU_CACHE_SIZE)
    {
        // Remove the least recently used node
        LRUNode *toRemove = cache.tail;
        if (toRemove)
        {
            if (toRemove->prev)
                toRemove->prev->next = NULL;
            cache.tail = toRemove->prev;

            // Free the associated TrieNode
            free(toRemove);
            cache.size--;
        }
    }

    // search if it exits in path
    // Add the new node to the head of the cache
    LRUNode *newNode = createLRUNode(node, path);
    newNode->next = cache.head;
    if (cache.head)
        cache.head->prev = newNode;
    cache.head = newNode;

    if (!cache.tail)
        cache.tail = newNode;
    
    cache.size++;
}

// Search for a path in the LRU cache
TrieNode *searchCache(const char *path)
{
    LRUNode *current = cache.head;
    while (current)
    {
        if (strcmp(current->path, path) == 0)
        {
            // Move the accessed node to the head
            moveToHead(current);
            return current->trieNode;
        }
        current = current->next;
    }
    // if not found in cache add it to cache

    return NULL; // Path not found in the cache
}

// Free the LRU cache
void freeCache()
{
    LRUNode *current = cache.head;
    while (current)
    {
        LRUNode *toFree = current;
        current = current->next;

        if (toFree->trieNode)
        {
            freeTrie(toFree->trieNode);
        }
        free(toFree);
    }
    cache.head = NULL;
    cache.tail = NULL;
    cache.size = 0;
}

// Remove a path from the LRU cache
void remove_from_cache(const char *path)
{
    LRUNode *current = cache.head;
    while (current)
    {
        // print in pink colour
        printf("\033[1;35mchecking %s vs %s\n\033[0m",current->path,path);
        if (strncmp(current->path, path,strlen(path)) == 0)
        {
            // Remove the node from the cache
            if (current->prev)
                current->prev->next = current->next;
            if (current->next)
                current->next->prev = current->prev;

            if (cache.head == current)
                cache.head = current->next;
            if (cache.tail == current)
                cache.tail = current->prev;

            // Free the associated TrieNode
            free(current);
            cache.size--;
            // return;
        }
        current = current->next;
    }
}

void printCache()
{
    if (!cache.head)
    {
        printf("LRU Cache is empty.\n");
        return;
    }

    printf("\033[1;33mLRU Cache Contents (Most Recently Used -> Least Recently Used):\033[0m\n");
    LRUNode *current = cache.head;
    while (current)
    {
        printf("\033[1;33mPath: %s, TrieNode Address: %p\033[0m\n", current->path, (void *)current->trieNode);
        current = current->next;
    }
}
