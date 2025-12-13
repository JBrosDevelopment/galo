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
    TOKEN_KEYWORD_STRUCT,
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
    NODE_SCOPED_IDENTIFIER,
    NODE_CONSTANT,
    NODE_BODY,
    NODE_EMPTY,
    NODE_END
};

typedef struct Node_t {
    enum NodeType type;
    void* data;
} Node;

typedef struct NodeList_t {
    Node* nodes;
    int size;
    int capacity;
} NodeList;

typedef struct VariableDeclaration_t {
    Token* name;
    Token* type;
    Node value;
    int id;
} VariableDeclaration;

typedef struct Parameter_t {
    Token* name;
    Token* type;
} Parameter;

typedef struct FunctionDeclaration_t {
    Token* name;
    Parameter* parameters;
    int parameter_count;
    Token* return_type;
    Token* struct_implementation;
    NodeList* body;
    int id;
} FunctionDeclaration;

typedef struct StructDeclaration_t {
    Token* name;
    Parameter* fields;
    int field_count;
    int id;
} StructDeclaration;

typedef struct ScopedIdentifier_t {
    Token** scope;
    int size;
} ScopedIdentifier;

typedef struct VariableAssignment_t {
    ScopedIdentifier identifier;
    Node value;
} VariableAssignment;

typedef struct FunctionCall_t {
    Token** scope;
    int scope_size;
    Node* arguments;
    int argument_count;
} FunctionCall;

typedef struct WhileLoop_t {
    Node condition;
    NodeList* body;
} WhileLoop;

typedef struct ElifIfStatement_t {
    Node condition;
    NodeList* body;
} ElifIfStatement;

typedef struct IfStatement_t {
    Node condition;
    NodeList* body;
    ElifIfStatement* elifs;
    int elif_count;
    char has_else;
    NodeList* else_body;
} IfStatement;

typedef struct ReturnStatement_t {
    Node value;
} ReturnStatement;

typedef struct Operation_t {
    Token* operator;
    Node* left;
    Node* right;
} Operation;

NodeList* create_node_list();
void free_node_list(NodeList* node_list);
void add_node(NodeList* node_list, Node node);
Node* get_node(NodeList* node_list, int index);

typedef struct IntList_t {
    int* int_list;
    int size;
    int capacity;
} IntList;

IntList* create_int_list();
void free_int_list(IntList* int_list);
void add_int(IntList* int_list, int value);
int* get_int(IntList* int_list, int index);
void remove_int(IntList* int_list, int index);
char contains_int(IntList* int_list, int value);

typedef struct ObjectList_t {
    void** objects;
    int size;
    int capacity;
} ObjectList;

typedef struct Validator_Object_t {
    ObjectList* functions;
    ObjectList* structs;
    ObjectList* variables;
    IntList* active_variables;
    int last_id;
} Validator_Object;

ObjectList* create_object_list();
void free_object_list(ObjectList* object_list);
void* add_object(ObjectList* object_list, void* object, int size);
void* get_object(ObjectList* object_list, int index);

const char* read_file(const char* filename);

void debug_lexer(TokenList* token_list);
void debug_lexer_reshape(TokenList* token_list);
const char* get_token_type_name(enum TokenType type);
void lexer(const char* source_code, TokenList* token_list);

void parser(TokenList* tokens, ObjectList* object_list, NodeList* ast, int* index);
const char* get_node_type_name(enum NodeType type);
void debug_parser(NodeList* ast);
void debug_parser_node(Node* node);

Validator_Object create_validator_object();
void free_validator_object(Validator_Object* validator_object);
void validator(NodeList* ast, Validator_Object* validator_object);
void debug_validator(Validator_Object* validator_object);

void run();