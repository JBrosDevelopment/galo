#ifndef Lexer_H
#define Lexer_H

#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LEXING_CODE_NONE 0
#define LEXING_CODE_WORD 1
#define LEXING_CODE_NUMBER 2
#define LEXING_CODE_STRING 3
#define LEXING_CODE_COMMENT 4
#define LEXING_CODE_END_LEXING 5

void lexer_add_word_or_number_token(const char* source_code, int i, int* start_of_token, char* lexing_code, int line, Token* token); // Forward declaration

void lexer(const char* source_code, TokenList* token_list) {
    printf("Lexing...\n");
    
    int start_of_token = -1;
    char lexing_code = LEXING_CODE_NONE;
    int line = 1;
    int i;
    for (i = 0; source_code[i] != '\0'; i++) {
        Token token;

        if (lexing_code == LEXING_CODE_STRING) {
            if (source_code[i] == '"') {
                int length = i - start_of_token;
                char* token_value = (char*)malloc(length + 1);
                strncpy(token_value, &source_code[start_of_token], length);
                token_value[length] = '\0';
                
                token.type = TOKEN_CONSTANT_STRING;
                token.value = token_value;
                add_token(token_list, token);
                
                lexing_code = LEXING_CODE_NONE;
                start_of_token = -1;
            }
            continue;
        } else if (lexing_code == LEXING_CODE_COMMENT) {
            if (source_code[i] == '\n') {
                lexing_code = LEXING_CODE_NONE;
                token.type = TOKEN_END_OF_LINE;
                token.value = "\n";
                add_token(token_list, token);
                line++;
            }
            continue;
        } else if (lexing_code == LEXING_CODE_NUMBER && (source_code[i] < '0' || source_code[i] > '9') && source_code[i] != '.') {
            lexer_add_word_or_number_token(source_code, i, &start_of_token, &lexing_code, line, &token);
            add_token(token_list, token);
            lexing_code = LEXING_CODE_NONE;
        } else if (lexing_code == LEXING_CODE_WORD && !((source_code[i] >= 'A' && source_code[i] <= 'Z') || (source_code[i] >= 'a' && source_code[i] <= 'z') || (source_code[i] >= '0' && source_code[i] <= '9')) && source_code[i] != '_') {
            lexer_add_word_or_number_token(source_code, i, &start_of_token, &lexing_code, line, &token);
            add_token(token_list, token);
            lexing_code = LEXING_CODE_NONE;
        }

        switch (source_code[i])
        {
        case '\n':
            token.type = TOKEN_END_OF_LINE;
            token.value = "\n";
            line++;
            break;
        case '#':
            lexing_code = LEXING_CODE_COMMENT;
            break;
        case '=': 
            if (source_code[i + 1] == '=') {
                token.type = TOKEN_OPERATOR_COMPARISON;
                token.value = "==";
                i++;
                break;
            }
            token.type = TOKEN_OPERATOR_ASSIGN;
            token.value = "=";
            break;
        case '>':
            if (source_code[i + 1] == '=') {
                token.type = TOKEN_OPERATOR_COMPARISON;
                token.value = ">=";
                i++;
                break;
            }
            token.type = TOKEN_OPERATOR_COMPARISON;
            token.value = ">";
            break;
        case '<':
            if (source_code[i + 1] == '=') {
                token.type = TOKEN_OPERATOR_COMPARISON;
                token.value = "<=";
                i++;
                break;
            }
            token.type = TOKEN_OPERATOR_COMPARISON;
            token.value = "<";
            break;
        case '!':
            if (source_code[i + 1] == '=') {
                token.type = TOKEN_OPERATOR_COMPARISON;
                token.value = "!=";
                i++;
                break;
            }
            printf("Lexer Error: Unexpected character '!' in line %d\n", line);
            exit(1);
            break;
        case '+': case '-': case '*': case '/': case '%':
            token.type = TOKEN_OPERATOR_ARITHMETIC;
            token.value = source_code[i] == '+' ? "+" :
                          source_code[i] == '-' ? "-" :
                          source_code[i] == '*' ? "*" :
                          source_code[i] == '/' ? "/" : "%";
            break;
        case '(':
            token.type = TOKEN_PARENTHESIS_OPEN;
            token.value = "(";
            break;
        case ')':
            token.type = TOKEN_PARENTHESIS_CLOSE;
            token.value = ")";
            break;
        case ',':
            token.type = TOKEN_COMMA;
            token.value = ",";
            break;
        case '\0': case '~':
            lexing_code = LEXING_CODE_END_LEXING;
            break;
        case '"':
            token.type = TOKEN_CONSTANT_STRING;
            lexing_code = LEXING_CODE_STRING;
            start_of_token = i + 1;
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':
            if (start_of_token == -1) {
                start_of_token = i;
                lexing_code = LEXING_CODE_NUMBER;
            }
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
            if (start_of_token == -1) {
                start_of_token = i;
                lexing_code = LEXING_CODE_WORD;
            }
            break;
        case ' ': case '\t': case '\r':
            continue;
        default:
            start_of_token = i;
            break;
        }
        
        if (lexing_code == LEXING_CODE_NONE) {
            add_token(token_list, token);
        } else if (lexing_code == LEXING_CODE_END_LEXING) {
            break;
        }
    }

    if (lexing_code == LEXING_CODE_STRING) {
        printf("Lexer Error: Unterminated string literal in line %d\n", line);
        exit(1);
    } else if (lexing_code == LEXING_CODE_NUMBER || lexing_code == LEXING_CODE_WORD) {
        Token token;
        lexer_add_word_or_number_token(source_code, i, &start_of_token, &lexing_code, line, &token);
        add_token(token_list, token);
    }

    Token eof_token;
    eof_token.type = TOKEN_EOF;
    eof_token.value = "\0";
    add_token(token_list, eof_token);
}

void lexer_add_word_or_number_token(const char* source_code, int i, int* start_of_token, char* lexing_code, int line, Token* token) {
    int length = i - *start_of_token;
    char* token_value = (char*)malloc(length + 1);
    strncpy(token_value, &source_code[*start_of_token], length);
    token_value[length] = '\0';
    
    token->value = token_value;
    *start_of_token = -1;

    if (*lexing_code == LEXING_CODE_NUMBER) {
        char is_float = 0;
        for (int j = 0; j < length; j++) {
            if (token_value[j] == '.') {
                is_float++;
                continue;
            }
            if (token_value[j] < '0' || token_value[j] > '9') {
                // Handle error: invalid number format
                printf("Lexer Error: Invalid number format: `%s` in line %d\n", token_value, line);
                exit(1);
            }
        }
        *lexing_code = LEXING_CODE_NONE;

        if (is_float > 0) {
            token->type = TOKEN_CONSTANT_FLOAT;
            return;
        } else if (is_float == 0) {
            token->type = TOKEN_CONSTANT_INTEGER;
            return;
        } else {
            // Handle error: multiple decimal points
            printf("Lexer Error: Invalid float format: `%s` in line %d\n", token_value, line);
            exit(1);
        }
    } 
    
    if (strcmp(token_value, "let") == 0) {
        token->type = TOKEN_KEYWORD_LET;
    } else if (strcmp(token_value, "fun") == 0) {
        token->type = TOKEN_KEYWORD_FUN;
    } else if (strcmp(token_value, "end") == 0) {
        token->type = TOKEN_KEYWORD_END;
    } else if (strcmp(token_value, "if") == 0) {
        token->type = TOKEN_KEYWORD_IF;
    } else if (strcmp(token_value, "elif") == 0) {
        token->type = TOKEN_KEYWORD_ELIF;
    } else if (strcmp(token_value, "else") == 0) {
        token->type = TOKEN_KEYWORD_ELSE;
    } else if (strcmp(token_value, "return") == 0) {
        token->type = TOKEN_KEYWORD_RETURN;
    } else if (strcmp(token_value, "while") == 0) {
        token->type = TOKEN_KEYWORD_WHILE;
    } else if (strcmp(token_value, "true") == 0) {
        token->type = TOKEN_CONSTANT_BOOLEAN;
    } else if (strcmp(token_value, "false") == 0) {
        token->type = TOKEN_CONSTANT_BOOLEAN;
    } else if (strcmp(token_value, "int") == 0) {
        token->type = TOKEN_NATIVE_TYPE;
    } else if (strcmp(token_value, "float") == 0) {
        token->type = TOKEN_NATIVE_TYPE;
    } else if (strcmp(token_value, "string") == 0) {
        token->type = TOKEN_NATIVE_TYPE;
    } else if (strcmp(token_value, "bool") == 0) {
        token->type = TOKEN_NATIVE_TYPE;
    } else if (strcmp(token_value, "byte") == 0) {
        token->type = TOKEN_NATIVE_TYPE;
    } else if (strcmp(token_value, "and") == 0) {
        token->type = TOKEN_OPERATOR_LOGICAL;
    } else if (strcmp(token_value, "or") == 0) {
        token->type = TOKEN_OPERATOR_LOGICAL;
    } else {
        token->type = TOKEN_IDENTIFIER;
    }
}

const char* get_token_type_name(enum TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "TOKEN_EOF"; 
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER"; 
        case TOKEN_CONSTANT_INTEGER: return "TOKEN_CONSTANT_INTEGER"; 
        case TOKEN_CONSTANT_FLOAT: return "TOKEN_CONSTANT_FLOAT"; 
        case TOKEN_CONSTANT_STRING: return "TOKEN_CONSTANT_STRING"; 
        case TOKEN_CONSTANT_BOOLEAN: return "TOKEN_CONSTANT_BOOLEAN"; 
        case TOKEN_NATIVE_TYPE: return "TOKEN_NATIVE_TYPE"; 
        case TOKEN_KEYWORD_LET: return "TOKEN_KEYWORD_LET"; 
        case TOKEN_KEYWORD_FUN: return "TOKEN_KEYWORD_FUN"; 
        case TOKEN_KEYWORD_END: return "TOKEN_KEYWORD_END"; 
        case TOKEN_KEYWORD_IF: return "TOKEN_KEYWORD_IF"; 
        case TOKEN_KEYWORD_ELIF: return "TOKEN_KEYWORD_ELIF"; 
        case TOKEN_KEYWORD_ELSE: return "TOKEN_KEYWORD_ELSE"; 
        case TOKEN_KEYWORD_RETURN: return "TOKEN_KEYWORD_RETURN"; 
        case TOKEN_KEYWORD_WHILE: return "TOKEN_KEYWORD_WHILE"; 
        case TOKEN_OPERATOR_ASSIGN: return "TOKEN_OPERATOR_ASSIGN"; 
        case TOKEN_OPERATOR_ARITHMETIC: return "TOKEN_OPERATOR_ARITHMETIC"; 
        case TOKEN_OPERATOR_COMPARISON: return "TOKEN_OPERATOR_COMPARISON"; 
        case TOKEN_OPERATOR_LOGICAL: return "TOKEN_OPERATOR_LOGICAL"; 
        case TOKEN_PARENTHESIS_OPEN: return "TOKEN_PARENTHESIS_OPEN"; 
        case TOKEN_PARENTHESIS_CLOSE: return "TOKEN_PARENTHESIS_CLOSE"; 
        case TOKEN_COMMA: return "TOKEN_COMMA"; 
        case TOKEN_END_OF_LINE: return "TOKEN_END_OF_LINE";
    }
    return "ERROR_UNKNOWN_TOKEN_TYPE";
}

void debug_lexer(TokenList* token_list) {
    printf("Tokens:\n");
    for (int i = 0; i < token_list->size; i++) {
        Token* token = &token_list->tokens[i];
        char* type_name = (char*)get_token_type_name(token->type);
        printf("Type: %s, Value: %s\n", type_name, token->value);
    }
}

void debug_lexer_reshape(TokenList* token_list) {
    printf("Tokens Reshaped:\n");
    for (int i = 0; i < token_list->size; i++) {
        Token* token = &token_list->tokens[i];
        if (token->type == TOKEN_CONSTANT_STRING) {
            printf("\"%s\" ", token->value);
            continue;
        }
        printf("%s ", token->value);
    }
}

#endif // Lexer_H