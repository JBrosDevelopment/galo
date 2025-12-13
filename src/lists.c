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
void remove_int(IntList* int_list, int index) {
    if (index < 0 || index >= int_list->size) {
        return;
    }
    for (int i = index; i < int_list->size - 1; i++) {
        int_list->int_list[i] = int_list->int_list[i + 1];
    }
    int_list->size--;
}

char contains_int(IntList* int_list, int value) {
    for (int i = 0; i < int_list->size; i++) {
        if (int_list->int_list[i] == value) {
            return 1;
        }
    }
    return 0;
}


ObjectList* create_object_list() {
    ObjectList* list = malloc(sizeof(ObjectList));
    list->size = 0;
    list->capacity = 32;
    list->objects = malloc(sizeof(void*) * list->capacity);
    return list;
}

void free_object_list(ObjectList* list) {
    for (int i = 0; i < list->size; i++)
        free(list->objects[i]);
    free(list->objects);
    free(list);
}

void* add_object(ObjectList* list, void* data, int size) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->objects = realloc(list->objects, sizeof(void*) * list->capacity);
    }

    void* ptr = malloc(size);
    memcpy(ptr, data, size);

    list->objects[list->size++] = ptr;
    return ptr; // stable pointer
}

void* get_object(ObjectList* list, int index) {
    if (index < 0 || index >= list->size) return NULL;
    return list->objects[index];
}

#endif // TokenList_H