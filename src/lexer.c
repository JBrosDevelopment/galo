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

void lexer_add_word_or_number_token(const char* source_code, char* file_name, int i, int* start_of_token, char* lexing_code, int line, Token* token); // Forward declaration

void lexer_linker(StringList* source_code_file_names, StringList* source_code_files, TokenList* token_list) {
    for (int i = 0; i < source_code_files->size; i++) {
        char* source_code = get_string(source_code_files, i);
        char* file_name = get_string(source_code_file_names, i);
        lexer(source_code, file_name, token_list);

        // Insert END_OF_LINE token between files
        Token eol;
        eol.type = TOKEN_END_OF_LINE;
        eol.value = "\n";
        eol.line = -1;
        eol.column = -1;

        add_token(token_list, eol);
    }

    // Insert EOF token
    Token eof;
    eof.type = TOKEN_EOF;
    eof.value = "";
    eof.line = -1;
    eof.column = -1;

    add_token(token_list, eof);
}

void lexer(const char* source_code, char* file_name, TokenList* token_list) {
    int start_of_token = -1;
    char lexing_code = LEXING_CODE_NONE;
    int line = 1;
    int column = 1;
    int i;
    for (i = 0; source_code[i] != '\0'; i++) {
        Token token;
        token.line = line;
        token.column = column;
        column++;

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
                column = 1;
            }
            continue;
        } else if (lexing_code == LEXING_CODE_NUMBER && (source_code[i] < '0' || source_code[i] > '9') && source_code[i] != '.') {
            lexer_add_word_or_number_token(source_code, file_name, i, &start_of_token, &lexing_code, line, &token);
            add_token(token_list, token);
            lexing_code = LEXING_CODE_NONE;
            token.column = column;
        } else if (lexing_code == LEXING_CODE_WORD && !((source_code[i] >= 'A' && source_code[i] <= 'Z') || (source_code[i] >= 'a' && source_code[i] <= 'z') || (source_code[i] >= '0' && source_code[i] <= '9')) && source_code[i] != '_') {
            lexer_add_word_or_number_token(source_code, file_name, i, &start_of_token, &lexing_code, line, &token);
            add_token(token_list, token);
            lexing_code = LEXING_CODE_NONE;
            token.column = column;
        }

        switch (source_code[i])
        {
        case '\n':
            token.type = TOKEN_END_OF_LINE;
            token.value = "\n";
            line++;
            column = 1;
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
            printf("Lexer Error: Unexpected character '!' in line %d in file %s\n", line, file_name);
            exit(1);
            break;
        case '+': case '-': case '*': case '/': case '%':
            if (source_code[i] == '-' && start_of_token == -1 && (source_code[i + 1] == '0' || source_code[i + 1] == '1' || source_code[i + 1] == '2' || source_code[i + 1] == '3' || source_code[i + 1] == '4' || source_code[i + 1] == '5' || source_code[i + 1] == '6' || source_code[i + 1] == '7' || source_code[i + 1] == '8' || source_code[i + 1] == '9' || source_code[i + 1] == '.')) {
                start_of_token = i;
                lexing_code = LEXING_CODE_NUMBER;
                break;
            }
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
            token.value = "\0";
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
        printf("Lexer Error: Unterminated string literal in line %d in file %s\n", line, file_name);
        exit(1);
    } else if (lexing_code == LEXING_CODE_NUMBER || lexing_code == LEXING_CODE_WORD) {
        Token token;
        token.column = column;
        token.line = line;
        lexer_add_word_or_number_token(source_code, file_name, i, &start_of_token, &lexing_code, line, &token);
        add_token(token_list, token);
    }

    // taken care of by the lexer_linker
    // Token eof_token;
    // eof_token.column = column;
    // eof_token.line = line;
    // eof_token.type = TOKEN_EOF;
    // eof_token.value = "\0";
    // add_token(token_list, eof_token);
}

void lexer_add_word_or_number_token(const char* source_code, char* file_name, int i, int* start_of_token, char* lexing_code, int line, Token* token) {
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
            if (token_value[j] == '-') {
                if (j == 0) {
                    continue;
                }
                else {
                    printf("Lexer Error: Invalid number format: `%s` in line %d in file %s\n", token_value, line, file_name);
                    exit(1);
                }
            }
            if (token_value[j] < '0' || token_value[j] > '9') {
                // Handle error: invalid number format
                printf("Lexer Error: Invalid number format: `%s` in line %d in file %s\n", token_value, line, file_name);
                exit(1);
            }
        }
        *lexing_code = LEXING_CODE_NONE;

        if (is_float == 1) {
            token->type = TOKEN_CONSTANT_FLOAT;
            return;
        } else if (is_float == 0) {
            token->type = TOKEN_CONSTANT_INTEGER;
            return;
        } else {
            // Handle error: multiple decimal points
            printf("Lexer Error: Invalid float format: `%s` in line %d in file %s\n", token_value, line, file_name);
            exit(1);
        }
    } 
    
    if (strcmp(token_value, "let") == 0) {
        token->type = TOKEN_KEYWORD_LET;
    } else if (strcmp(token_value, "fun") == 0) {
        token->type = TOKEN_KEYWORD_FUN;
    } else if (strcmp(token_value, "struct") == 0) {
        token->type = TOKEN_KEYWORD_STRUCT;
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
    } else if (strcmp(token_value, "break") == 0) {
        token->type = TOKEN_KEYWORD_BREAK;
    } else if (strcmp(token_value, "continue") == 0) {
        token->type = TOKEN_KEYWORD_CONTINUE;
    } else if (strcmp(token_value, "while") == 0) {
        token->type = TOKEN_KEYWORD_WHILE;
    } else if (strcmp(token_value, "true") == 0) {
        token->type = TOKEN_CONSTANT_BOOLEAN;
    } else if (strcmp(token_value, "false") == 0) {
        token->type = TOKEN_CONSTANT_BOOLEAN;
    } else if (strcmp(token_value, "int") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "float") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "string") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "bool") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "byte") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "void") == 0) {
        token->type = TOKEN_IDENTIFIER;
    } else if (strcmp(token_value, "and") == 0) {
        token->type = TOKEN_OPERATOR_LOGICAL;
    } else if (strcmp(token_value, "or") == 0) {
        token->type = TOKEN_OPERATOR_LOGICAL;
    } else if (strcmp(token_value, "not") == 0) {
        token->type = TOKEN_KEYWORD_NOT;
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
        case TOKEN_KEYWORD_LET: return "TOKEN_KEYWORD_LET"; 
        case TOKEN_KEYWORD_FUN: return "TOKEN_KEYWORD_FUN"; 
        case TOKEN_KEYWORD_STRUCT: return "TOKEN_KEYWORD_STRUCT";
        case TOKEN_KEYWORD_END: return "TOKEN_KEYWORD_END"; 
        case TOKEN_KEYWORD_IF: return "TOKEN_KEYWORD_IF"; 
        case TOKEN_KEYWORD_ELIF: return "TOKEN_KEYWORD_ELIF"; 
        case TOKEN_KEYWORD_ELSE: return "TOKEN_KEYWORD_ELSE"; 
        case TOKEN_KEYWORD_RETURN: return "TOKEN_KEYWORD_RETURN"; 
        case TOKEN_KEYWORD_BREAK: return "TOKEN_KEYWORD_BREAK"; 
        case TOKEN_KEYWORD_CONTINUE: return "TOKEN_KEYWORD_CONTINUE";
        case TOKEN_KEYWORD_WHILE: return "TOKEN_KEYWORD_WHILE"; 
        case TOKEN_KEYWORD_NOT: return "TOKEN_KEYWORD_NOT";
        case TOKEN_OPERATOR_ASSIGN: return "TOKEN_OPERATOR_ASSIGN"; 
        case TOKEN_OPERATOR_ARITHMETIC: return "TOKEN_OPERATOR_ARITHMETIC"; 
        case TOKEN_OPERATOR_COMPARISON: return "TOKEN_OPERATOR_COMPARISON"; 
        case TOKEN_OPERATOR_LOGICAL: return "TOKEN_OPERATOR_LOGICAL"; 
        case TOKEN_PARENTHESIS_OPEN: return "TOKEN_PARENTHESIS_OPEN"; 
        case TOKEN_PARENTHESIS_CLOSE: return "TOKEN_PARENTHESIS_CLOSE"; 
        case TOKEN_COMMA: return "TOKEN_COMMA"; 
        case TOKEN_END_OF_LINE: return "TOKEN_END_OF_LINE";
    }
    static char error_message[50];
    sprintf(error_message, "Error: Invalid token type: %d", type);
    return error_message;
}

void debug_lexer(TokenList* token_list, FILE* out) {
    fprintf(out, "Tokens:\n");
    for (int i = 0; i < token_list->size; i++) {
        Token* token = &token_list->tokens[i];
        char* type_name = (char*)get_token_type_name(token->type);
        fprintf(out, "Index: %d, Line: %d, Column: %d, Type: %s, Value: %s\n", i, token->line, token->column, type_name, token->value);
    }
    fprintf(out, "End Tokens.\n");
}

void debug_lexer_reshape(TokenList* token_list, FILE* out) {
    fprintf(out, "Tokens Reshaped:\n");
    for (int i = 0; i < token_list->size; i++) {
        Token* token = &token_list->tokens[i];
        if (token->type == TOKEN_CONSTANT_STRING) {
            fprintf(out, "\"%s\" ", token->value);
            continue;
        }
        fprintf(out, "%s ", token->value);
    }
    fprintf(out, "\n");
    fprintf(out, "End Tokens Reshaped.\n");
}

void emit_tokens(TokenList* token_list, char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        printf("Error: Could not open file %s for writing tokens.\n", output_file);
        return;
    }

    debug_lexer(token_list, file);
    fprintf(file, "\n");
    debug_lexer_reshape(token_list, file);

    fclose(file);
}
