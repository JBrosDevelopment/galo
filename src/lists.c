#ifndef TokenList_H
#define TokenList_H

#include "galo_headers.h"
#include <stdlib.h>
#include <string.h>

TokenList* create_token_list() {
    TokenList* token_list = (TokenList*)malloc(sizeof(TokenList));
    token_list->size = 0;
    token_list->capacity = 250;
    token_list->tokens = (Token*)malloc(sizeof(Token) * token_list->capacity);
    return token_list;
}
void free_token_list(TokenList* token_list) {
    free(token_list->tokens);
    free(token_list);
}
void add_token(TokenList* token_list, Token token) {
    if (token_list->size >= token_list->capacity) {
        token_list->capacity *= 2;
        token_list->tokens = (Token*)realloc(token_list->tokens, sizeof(Token) * token_list->capacity);
    }
    token_list->tokens[token_list->size++] = token;
}
Token* get_token(TokenList* token_list, int index) {
    if (index < 0 || index >= token_list->size) {
        return NULL;
    }
    return &token_list->tokens[index];
}

NodeList* create_node_list() {
    NodeList* node_list = (NodeList*)malloc(sizeof(NodeList));
    node_list->size = 0;
    node_list->capacity = 50;
    node_list->nodes = (Node*)malloc(sizeof(Node) * node_list->capacity);
    return node_list;
}
void free_node_list(NodeList* node_list) {
    free(node_list->nodes);
    free(node_list);
}
void add_node(NodeList* node_list, Node node) {
    if (node_list->size >= node_list->capacity) {
        node_list->capacity *= 2;
        node_list->nodes = (Node*)realloc(node_list->nodes, sizeof(Node) * node_list->capacity);
    }
    node_list->nodes[node_list->size++] = node;
}
Node* get_node(NodeList* node_list, int index) {
    if (index < 0 || index >= node_list->size) {
        return NULL;
    }
    return &node_list->nodes[index];
}


IntList* create_int_list() {
    IntList* int_list = (IntList*)malloc(sizeof(IntList));
    int_list->size = 0;
    int_list->capacity = 50;
    int_list->int_list = (int*)malloc(sizeof(int) * int_list->capacity);
    return int_list;
}
void free_int_list(IntList* int_list) {
    free(int_list->int_list);
    free(int_list);
}
void add_int(IntList* int_list, int value) {
    if (int_list->size >= int_list->capacity) {
        int_list->capacity *= 2;
        int_list->int_list = (int*)realloc(int_list->int_list, sizeof(int) * int_list->capacity);
    }
    int_list->int_list[int_list->size++] = value;
}
int* get_int(IntList* int_list, int index) {
    if (index < 0 || index >= int_list->size) {
        return NULL;
    }
    return &int_list->int_list[index];
}


ObjectList* create_object_list() {
    ObjectList* ol = malloc(sizeof(ObjectList));
    ol->size = 0;
    ol->capacity = 50;
    ol->objects = malloc(1);
    ol->object_sizes = *create_int_list(); 
    return ol;
}
void free_object_list(ObjectList* object_list) {
    if (object_list == NULL) return;
    free(object_list);
}
void* add_object(ObjectList* object_list, void* object, int size) {
    int offset = 0;
    for (int i = 0; i < object_list->size; i++) {
        offset += object_list->object_sizes.int_list[i];
    }
    object_list->objects = realloc(object_list->objects, offset + size);
    
    void* address = memcpy(object_list->objects + offset, object, size);
    
    add_int(&object_list->object_sizes, size);
    object_list->size++;

    return address;
}

void* get_object(ObjectList* object_list, int index, int* size_out) {
    if (index < 0 || index >= object_list->size)
        return NULL;

    int offset = 0;
    for (int i = 0; i < index; i++) {
        offset += object_list->object_sizes.int_list[i];
    }

    int size = object_list->object_sizes.int_list[index];
    if (size_out) *size_out = size;

    return object_list->objects + offset;
}


#endif // TokenList_H