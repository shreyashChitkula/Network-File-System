#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdbool.h>
#include "tries.h"
#include "two_threads.h"
#include "lru.h"
#include "log.h"

extern TrieNode *trieRoot;

TrieNode *createTrieNode(char data,char *ip, int port)
{
    TrieNode* node = (TrieNode*) calloc (1, sizeof(TrieNode));
    for (int i=0; i<ALPHABET_SIZE; i++)
        node->children[i] = NULL;
    node->is_leaf = 0;
    node->data = data;
    node->port = port;
    if(ip == NULL)
        node->ip = NULL;
    else{
        node->ip = (char*) calloc (INET_ADDRSTRLEN, sizeof(char));
        strcpy(node->ip, ip);
    }
    return node;
}
void freeTrie(TrieNode *node) {
    // printf("entered free to free\n");
    for(int i=0; i<ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) {
            // free the nodes
            freeTrie(node->children[i]);
            // from bottom up keep freeing the ip
            free(node->ip);
        }
        else {
            continue;
        }
    }
    free(node);
    // printf(" free to free done\n");
}

int deletePath(TrieNode *root, char *path)
{
    if (!root)
        return -1;
    if (!path || path[0] == '\0')
        return -1;

    TrieNode* temp = root;
    TrieNode* prev = root;
    int position=-1;

    for(int i=0; path[i]!='\0' && temp; i++)
    {
        position = path[i];
        // printf("in delete check %c \n",position);
        if (temp->children[position] == NULL)
            // not exist
            return 0;
        prev=temp;
        temp = temp->children[position];
    }
    if(position!=-1 && prev){
        prev->children[position]=NULL;
    }
    if (temp != NULL ){
        // printf("path found and deleted\n");
        freeTrie(temp);
        remove_from_cache(path);
        return 1;
    }
    // run through the cache and remove the path and all paths within it
    return 0;
}

TrieNode* insertPath(TrieNode *root, char *path,  char *ip, int port)
{
    TrieNode* temp = root;

    for (int i=0; path[i] != '\0'; i++) {
        // Get the relative position in the alphabet list
        int idx = (int) path[i];
        // printf("inserting at %c\n",idx);
        if (temp->children[idx] == NULL) {
            // If the corresponding child doesn't exist,
            // simply create that child!
            temp->children[idx] = createTrieNode(path[i],NULL,-1);
        }
        else {
            // Do nothing. The node already exists
        }
        // Go down a level, to the child referenced by idx
        // since we have a prefix match
        temp = temp->children[idx];
    }
    // At the end of the path, mark this node as the leaf node
    temp->is_leaf = 1;
    temp->ip = (char*) calloc (INET_ADDRSTRLEN, sizeof(char));
    strcpy(temp->ip, ip);
    temp->port = port;
    addToCache(temp, path);
    return root;
}

int searchPath(TrieNode* root, char* path,char *ip, int *port)
{
    // Searches for path in the Trie
    TrieNode* temp;

    if((temp = searchCache(path)) != NULL){
        // printf("path found in cache\n");
        strcpy(ip, temp->ip);
        *port = temp->port;
        return 1;
    }

    temp = root;
    
    for(int i=0; path[i]!='\0'; i++)
    {
        int position = path[i];
        // printf("search at %c\n",position);
        if (temp->children[position] == NULL)
            return 0;
        temp = temp->children[position];
    }
    if (temp != NULL && temp->is_leaf == 1){
        // path found and send ip and port data
        // printf("path found %s %d\n",temp->ip,temp->port);
        strcpy(ip, temp->ip);
        *port = temp->port;
        return 1;
    }
    return 0;
}


// TrieNode *getTrieNodeWithPath(TrieNode *root,  char *path){
//     TrieNode* temp = root;

//     for(int i=0; path[i]!='\0'; i++)
//     {
//         int position = path[i];
//         if (temp->children[position] == NULL)
//             return NULL;
//         temp = temp->children[position];
//     }
//     if (temp != NULL && temp->is_leaf == 1)
//         return temp;
//     return NULL;
// }


// Helper function to safely append to a string buffer
void string_append(char* buffer, size_t* current_pos, size_t max_size,  char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Calculate remaining space in buffer
    size_t remaining = max_size - *current_pos;
    if (remaining > 0) {
        // Add to buffer and update position
        int written = vsnprintf(buffer + *current_pos, remaining, format, args);
        if (written > 0) {
            *current_pos += (written < remaining) ? written : remaining - 1;
        }
    }
    
    va_end(args);
}

void get_path_recursive(TrieNode* node, char* current_path, int depth, 
                       char* output_buffer, size_t* buffer_pos, size_t max_size) {
    if (node == NULL) {
        return;
    }
    
    // If this is a leaf node, add the path info to output buffer
    if (node->is_leaf) {
        current_path[depth] = '\0';  // Null terminate the current path
        string_append(output_buffer, buffer_pos, max_size, 
                     "%s\n", 
                     current_path);
    }
    
    // Recursively traverse all possible children
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) {
            current_path[depth] = (char)i;  // Add current character to path
            // printf(" depth %d char  %c\n",depth,current_path[depth]);
            get_path_recursive(node->children[i], current_path, depth + 1,
                             output_buffer, buffer_pos, max_size);
        }
    }
}

// Main function to get all paths as a string
// Returns 0 on success, -1 if buffer is too small
int get_all_paths(TrieNode* root, char* output_buffer, size_t buffer_size) {
    if (output_buffer == NULL || buffer_size == 0) {
        return -1;
    }
    
    // Initialize buffer
    output_buffer[0] = '\0';
    size_t buffer_pos = 0;
    
    if (root == NULL) {
        string_append(output_buffer, &buffer_pos, buffer_size, "Trie is empty\n");
        return 0;
    }
    
    // Buffer to store the current path (assuming maximum path length of 1024)
    char current_path[1024];
    
    // Add header to output buffer
    // string_append(output_buffer, &buffer_pos, buffer_size, 
                //  "All paths in the trie:\n--------------------\n");
    
    // Get all paths recursively
    get_path_recursive(root, current_path, 0, output_buffer, &buffer_pos, buffer_size);
    
    return 0;
}

void handle_list_all_files(int socket_fd){
    char all_paths[MAX_PATH_LENGTH * MAX_PATHS];
    memset(all_paths, 0, sizeof(all_paths));
    get_all_paths(trieRoot, all_paths, sizeof(all_paths));
    // print all paths
    printf("%s\n", all_paths);
    bookkeep(socket_fd, 1, "List All Files", all_paths);
    send(socket_fd, all_paths, strlen(all_paths), 0);
}

int is_leaf_node(TrieNode* root, char* word) {
    // Checks if the prefix match of word and root
    // is a leaf node
    TrieNode* temp = root;
    for (int i=0; word[i]; i++) {
        int position = (int) word[i] - 'a';
        if (temp->children[position]) {
            temp = temp->children[position];
        }
    }
    return temp->is_leaf;
}
