#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define ALPHABET_SIZE 256 // For ASCII characters



typedef struct TrieNode {
    char *ip;         // Storage server IP address
    int port;         // Storage server port
    int is_leaf;  // Indicates end of a valid path
    char data;
    struct TrieNode *children[ALPHABET_SIZE];
} TrieNode;

TrieNode *createTrieNode(char data,char *ip, int port);
int deletePath(TrieNode *root,  char *path);
void freeTrie(TrieNode *node);
TrieNode* insertPath(TrieNode *root, char *path,  char *ip, int port);
int searchPath(TrieNode* root, char* path,char *ip,int *port);
TrieNode *getTrieNodeWithPath(TrieNode *root,  char *path);
void handle_list_all_files(int client_socket);
int is_leaf_node(TrieNode* root, char* word);