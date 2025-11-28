#ifndef TokenList_H
#define TokenList_H

#include "galo_headers.h"
#include <stdlib.h>

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

#endif // TokenList_H