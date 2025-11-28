enum TokenType {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT_INTEGER,
    TOKEN_CONSTANT_FLOAT,
    TOKEN_CONSTANT_STRING,
    TOKEN_CONSTANT_BOOLEAN,
    TOKEN_BUILT_INT_TYPE,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_FUN,
    TOKEN_KEYWORD_END,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_WHILE,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_ARITHMETIC,
    TOKEN_OPERATOR_LOGICAL,
    TOKEN_PARENTHESIS_OPEN,
    TOKEN_PARENTHESIS_CLOSE,
    TOKEN_COMMA,
    TOKEN_END_OF_LINE
};

typedef struct Token_t {
    enum TokenType type;
    const char* value;
} Token;

typedef struct TokenList_t {
    Token* tokens;
    int size;
    int capacity;
} TokenList;

TokenList* create_token_list();
void free_token_list(TokenList* token_list);
void add_token(TokenList* token_list, Token token);
Token* get_token(TokenList* token_list, int index);



void lexer(const char* source_code, TokenList* token_list);
void parser();
void validator();
void run();