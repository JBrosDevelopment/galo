#ifndef Lexer_H
#define Lexer_H

#include "galo_headers.h"
#include <stdio.h>

void lexer(const char* source_code, TokenList* token_list) {
    printf("Lexing...\n");

    for (int i = 0; source_code[i] != '\0'; i++) {
        Token token;

        switch (source_code[i])
        {
        case ' ': case '\t': 
            continue; // Skip whitespace
        case '\n':
            token.type = TOKEN_END_OF_LINE;
            token.value = "\\n";
            break;
        case '=': 
        
        default:
            break;
        }
        
        add_token(token_list, token);
    }

    Token eof_token;
    eof_token.type = TOKEN_EOF;
    eof_token.value = "\0";
    add_token(token_list, eof_token);
}

#endif // Lexer_H