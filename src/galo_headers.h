#pragma once
#include <stdbool.h>

#define GALO_VERSION "0.0.1"

/////////////////////////////////////////////////////////////////////
// ARGUMENT PROCESSOR
/////////////////////////////////////////////////////////////////////

typedef struct StringList_t {
    char** strings;
    int size;
    int capacity;
} StringList;

enum BuildOptions {
    OPTION_COMPILE, // requires input file: `galo compile file.galo`
    OPTION_TRANSPILE, // requires input file: `galo transpile file.galo`
    OPTION_INTERPRET, // requires input file: `galo interpret file.galo`
    OPTION_BUILD, // requires `build.galo`: `galo build`
    OPTION_RUN, // requires `main.galo` (with build options): `galo run`
    OPTION_NEW, // requires name: `galo new [NAME]`
    OPTION_VERSION, // `galo version`
    OPTION_HELP // `galo help`
};

typedef struct BuildArguments_t {
    enum BuildOptions build_option;

    // Universal Arguments
    StringList* input_files; // [FILE] or -i [PATH]
    char* output_file; // -o [PATH]
    StringList* input_dirs; // -I [PATH]
    StringList* passthrough_flags; // -- [EVERYTHING AFTER `--`]

    // Universal Flags
    bool emit_tokens; // --emit-tokens
    bool emit_ast; // --emit-ast
    bool emit_validator; // --emit-validator

    // Compiler Flags/Arguments
    char* compiler; // --compiler [PATH OR NAME OF COMPILER]
    bool emit_c; // --emit-c
    bool emit_obj; // --emit-obj
    bool debug_symbols; // --debug-symbols

    // Transpiler Flags/Arguments
    char* target_lang; // -t [LANGUAGE NAME]
    bool emit_headers; // --emit-header
    bool single_file; // --single-file

    // Interpreter Flags/Arguments
    int max_steps; // --max-steps [NUMBER]
    bool interpret_debug; // --interpret-debug

    // Miscellaneous
    char* project_name; // NAME for `new` option
    char* build_file_path; // path to build.galo file for `build` option
    char* main_file_path; // path to main.galo file for `run` option
    StringList* file_build_options; // build options extracted from shebangs in main.galo
} BuildArguments;


/////////////////////////////////////////////////////////////////////
// LEXER
/////////////////////////////////////////////////////////////////////

enum TokenType {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT_INTEGER,
    TOKEN_CONSTANT_FLOAT,
    TOKEN_CONSTANT_STRING,
    TOKEN_CONSTANT_BOOLEAN,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_FUN,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_END,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_NOT,
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

/////////////////////////////////////////////////////////////////////
// PARSER
/////////////////////////////////////////////////////////////////////

enum NodeType {
    NODE_VARIABLE_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_STRUCT_DECLARATION,
    NODE_VARIABLE_ASSIGNMENT,
    NODE_FUNCTION_CALL,
    NODE_WHILE_LOOP,
    NODE_IF_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_BREAK_STATEMENT,
    NODE_CONTINUE_STATEMENT,
    NODE_OPERATION,
    NODE_SCOPED_IDENTIFIER,
    NODE_CONSTANT,
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

typedef struct Identifier_t {
    Token* name;
    int id;
} Identifier;

typedef struct VariableDeclaration_t {
    Token* name;
    Token* type;
    Node value;
    int id;
    int type_id;
} VariableDeclaration;

typedef struct Parameter_t {
    Identifier name;
    Identifier type;
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
    Identifier* scope;
    int size;
} ScopedIdentifier;

typedef struct VariableAssignment_t {
    ScopedIdentifier identifier;
    Node value;
} VariableAssignment;

typedef struct FunctionCall_t {
    Identifier* scope;
    int scope_size;
    Node* arguments;
    int argument_count;
    int id;
} FunctionCall;

typedef struct WhileLoop_t {
    Node condition;
    NodeList* body;
    int line;
} WhileLoop;

typedef struct ElifIfStatement_t {
    Node condition;
    NodeList* body;
    int line;
} ElifIfStatement;

typedef struct IfStatement_t {
    Node condition;
    NodeList* body;
    ElifIfStatement* elifs;
    int elif_count;
    bool has_else;
    NodeList* else_body;
    int line;
} IfStatement;

typedef struct ReturnStatement_t {
    Node value;
    int line;
} ReturnStatement;

typedef struct Operation_t {
    Token* operator;
    Node* left;
    Node* right;
    bool is_not_operator;
} Operation;

/////////////////////////////////////////////////////////////////////
// LISTS 
/////////////////////////////////////////////////////////////////////

typedef struct IntList_t {
    int* int_list;
    int size;
    int capacity;
} IntList;

typedef struct ObjectList_t {
    void** objects;
    int size;
    int capacity;
} ObjectList;

TokenList* create_token_list();
void free_token_list(TokenList* token_list);
void add_token(TokenList* token_list, Token token);
Token* get_token(TokenList* token_list, int index);

IntList* create_int_list();
void free_int_list(IntList* int_list);
void add_int(IntList* int_list, int value);
int* get_int(IntList* int_list, int index);
void remove_int_index(IntList* int_list, int index);
void remove_int_value(IntList* int_list, int value);
bool contains_int(IntList* int_list, int value);

NodeList* create_node_list();
void free_node_list(NodeList* node_list);
void add_node(NodeList* node_list, Node node);
Node* get_node(NodeList* node_list, int index);

ObjectList* create_object_list();
void free_object_list(ObjectList* object_list);
void* add_object(ObjectList* object_list, void* object, int size);
void* get_object(ObjectList* object_list, int index);

StringList* create_string_list();
void free_string_list(StringList* string_list);
void free_string_owned_list(StringList* string_list);
void add_string(StringList* string_list, char* string);
char* get_string(StringList* string_list, int index); 
bool contains_string(StringList* string_list, char* string);
void string_list_to_owned_array(StringList* string_list, char*** array, int* count);
void free_owned_string_array(char** array, int count);

/////////////////////////////////////////////////////////////////////
// VALIDATOR
/////////////////////////////////////////////////////////////////////

#define NO_PARENT -1
#define INFINTE_PARAMETERS 0

#define NO_EXPECTED_NODE -67
#define VOID_TYPE -1
#define INT_TYPE -2
#define STRING_TYPE -3
#define BOOLEAN_TYPE -4
#define FLOAT_TYPE -5
#define BYTE_TYPE -6
#define LIST_TYPE -7
#define TYPE_AS_TYPE -8 /*used in `list init(type)`*/
#define ANY_TYPE -9 /*used in `list get(list, index)` as return type*/

typedef struct PredefinedFunction_t {
    char* name;
    int id;
    int* parameter_ids;
    int parameter_count;
    bool infinite_parameters;
    int return_id;
    int parent_id;
} PredefinedFunction;

typedef struct Validator_Object_t {
    ObjectList* predefined_functions;
    ObjectList* functions;
    ObjectList* structs;
    ObjectList* variables;
    IntList* active_variables;
    int last_variable_id;
    int last_struct_id;
    int last_function_id;
    bool is_inside_while_loop;
    bool is_inside_function;
} Validator_Object;

void add_function(Validator_Object* validator_object, char* name, int return_id, int parameter_count, int* parameter_ids, int parent_id);
int* predefined_function_parameters(int parameter_count, ...);

Validator_Object create_validator_object();
void free_validator_object(Validator_Object* validator_object);
void add_predefined_functions(Validator_Object* validator_object);
VariableDeclaration* get_variable_from_id(int id, Validator_Object* validator_object);
StructDeclaration* get_struct_from_id(int id, Validator_Object* validator_object);
FunctionDeclaration* get_function_from_id(int id, Validator_Object* validator_object);

/////////////////////////////////////////////////////////////////////
// INTERPRETER
/////////////////////////////////////////////////////////////////////

typedef struct LValue_t {
    void* address;
    int size;
    int variable_id;
    int type_id;
} LValue;

typedef struct GaloObject_t {
    int type_id;
    int size;
    void* data;
} GaloObject;

typedef struct ScopeFrame_t {
    int* variable_ids;
    int  count;
    int  capacity;
} ScopeFrame;

typedef struct Interpreter_Object_t {
    // Program data
    Validator_Object* validator_object;
    NodeList* ast;

    // Runtime storage
    GaloObject* variables;
    int variable_count;
    bool did_break;
    bool did_continue;
    bool did_return;
    bool did_exit;
    int exit_code;

    // Scoping
    ScopeFrame* scope_stack;
    int scope_depth;
    int scope_capacity;

    // Execution state
    Node* current_node;
    GaloObject return_value;
    bool has_returned;

    // Builtins
    GaloObject (**builtins)(struct Interpreter_Object_t*, GaloObject* args, int arg_count);
    int builtin_count;
} Interpreter_Object;

Interpreter_Object* create_interpreter_object(NodeList* ast, Validator_Object* validator_object);
void free_interpreter_object(Interpreter_Object* interpreter_object);
GaloObject predefined_function_call(Interpreter_Object* interp, PredefinedFunction* predefined_function, int argument_count, GaloObject* arguments);
GaloObject function_call(Interpreter_Object* interp, FunctionDeclaration* function, int argument_count, GaloObject* arguments);
void add_builtin_function(Interpreter_Object* interp, int id, GaloObject (*function)(Interpreter_Object* interp, GaloObject* args, int arg_count));
GaloObject get_field_in_struct(Interpreter_Object* interp, GaloObject* struct_object, int struct_id, Token* field_name);
GaloObject string_object_value(char* string);
GaloObject int_object_value(int value);
GaloObject float_object_value(float value);
GaloObject bool_object_value(bool value);
GaloObject byte_object_value(unsigned char value);
GaloObject list_object_value(ObjectList* list);
GaloObject void_object_value();
void print_out_variable_values(Interpreter_Object* interp);
void print_galo_object(Interpreter_Object* interp, GaloObject* object);
GaloObject function_call(Interpreter_Object* interp, FunctionDeclaration* function, int argument_count, GaloObject* arguments);
GaloObject predefined_function_call(Interpreter_Object* interp, PredefinedFunction* predefined_function, int argument_count, GaloObject* arguments);

/////////////////////////////////////////////////////////////////////
// FUNCTIONS AND THEIR DEBUGGER FUNCTIONS
/////////////////////////////////////////////////////////////////////

BuildArguments process_args(int argc, char** argv);
void debug_build_arguments(BuildArguments* build_arguments);
void free_build_arguments(BuildArguments* build_arguments);

void trim_trailing_whitespace(char* s);
const char* read_file(char* filename);
void preprocess(char* filename, StringList* source_code_file_names, StringList* source_code_files);
void preprocess_with_build_options(char* file_name, StringList* source_code_file_names, StringList* source_code_files, StringList* file_build_options);

const char* get_token_type_name(enum TokenType type);
void lexer(const char* source_code, char* file_name, TokenList* token_list);
void lexer_linker(StringList* source_code_file_names, StringList* source_code_files, TokenList* token_list);
void emit_tokens(TokenList* token_list, char* output_file);

void parser(TokenList* tokens, ObjectList* object_list, NodeList* ast, int* index);
const char* get_node_type_name(enum NodeType type);
void emit_ast(NodeList* ast, char* output_file);

void validator(NodeList* ast, Validator_Object* validator_object);
void emit_validator(Validator_Object* validator_object, char* output_file);

void build_option_new(char* project_name);
void build_option_version();
void build_option_help();

int interpret(Interpreter_Object* interpreter_object, int input_argc, char** input_argv);
GaloObject interpret_node(Interpreter_Object* interpreter_object, Node* node);

void transpile();
void compile();