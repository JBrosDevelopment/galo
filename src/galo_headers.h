enum TokenType {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT_INTEGER,
    TOKEN_CONSTANT_FLOAT,
    TOKEN_CONSTANT_STRING,
    TOKEN_CONSTANT_BOOLEAN,
    TOKEN_NATIVE_TYPE,
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
    TOKEN_OPERATOR_COMPARISON,
    TOKEN_OPERATOR_LOGICAL,
    TOKEN_PARENTHESIS_OPEN,
    TOKEN_PARENTHESIS_CLOSE,
    TOKEN_COMMA,
    TOKEN_END_OF_LINE
};

typedef struct Token_t {
    int line;
    int column;
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

enum NodeType {
    NODE_VARIABLE_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_STRUCT_DECLARATION,
    NODE_VARIABLE_ASSIGNMENT,
    NODE_FUNCTION_CALL,
    NODE_WHILE_LOOP,
    NODE_IF_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_OPERATION,
    NODE_IDENTIFIER,
    NODE_CONSTANT,
    NODE_BODY
};

typedef struct Node_t {
    enum NodeType type;
    void* data;
} Node;

typedef struct Body_t {
    Node* statements;
    int statement_count;
} Body;

typedef struct NodeList_t {
    Node* nodes;
    int size;
    int capacity;
} NodeList;

typedef struct VariableDeclaration_t {
    Token* name;
    Token* var_type;
    Node* value;
} VariableDeclaration;

typedef struct Parameter_t {
    Token* name;
    Token* param_type;
} Parameter;

typedef struct FunctionDeclaration_t {
    Token* name;
    Parameter* parameters;
    int parameter_count;
    Token* return_type;
    Token* struct_implementation;
    Body body;
} FunctionDeclaration;

typedef struct StructDeclaration_t {
    Token* name;
    Parameter* fields;
    int field_count;
} StructDeclaration;

typedef struct VariableAssignment_t {
    Token* name;
    Node* value;
} VariableAssignment;

typedef struct FunctionCall_t {
    Token* name;
    Node* arguments;
} FunctionCall;

typedef struct WhileLoop_t {
    Node* condition;
    Body body;
} WhileLoop;

typedef struct ElifIfStatement_t {
    Node* condition;
    Body body;
    struct ElifIfStatement_t* next;
} ElifIfStatement;

typedef struct IfStatement_t {
    Node* condition;
    Body body;
    ElifIfStatement* elif;
    Body else_body;
} IfStatement;

typedef struct ReturnStatement_t {
    Node* value;
} ReturnStatement;

typedef struct Operation_t {
    Token* operator;
    Node* left;
    Node* right;
} Operation;

typedef struct Identifier_t {
    Token* name;
} Identifier;

typedef struct Constant_t {
    Token* value;
} Constant;

NodeList* create_node_list();
void free_node_list(NodeList* node_list);
void add_node(NodeList* node_list, Node node);
Node* get_node(NodeList* node_list, int index);

const char* read_file(const char* filename);

void debug_lexer(TokenList* token_list);
void debug_lexer_reshape(TokenList* token_list);
const char* get_token_type_name(enum TokenType type);
void lexer(const char* source_code, TokenList* token_list);

void parser(TokenList* tokens, NodeList* ast);
void validator();
void run();