#ifndef Parser_H
#define Parser_H

#include "galo_headers.h"
#include <stdio.h>

void parse_line(TokenList* tokens, ObjectList* object_list, int* index, int end, Node* node); // Forward declaration
Node parse_expression(TokenList* tokens, ObjectList* object_list, int* index); // Forward declaration

void parser(TokenList* tokens, ObjectList* object_list, NodeList* ast, int* index) {
    int beginning_of_line_index = 0;
    for (; *index < tokens->size; (*index)++) {
        Token* token = get_token(tokens, *index);
        Node node;
        node.type = NODE_EMPTY;

        if (token->type == TOKEN_EOF) {
            break;
        }
        if (token->type != TOKEN_END_OF_LINE) {
            continue;
        }

        int end = *index;
        *index = beginning_of_line_index;
        beginning_of_line_index = end + 1;
        parse_line(tokens, object_list, index, end, &node);

        if (node.type != NODE_EMPTY) {
            add_node(ast, node);
        }
    }
}

char if_list_contains_token(TokenList* tokens, int start, int end, enum TokenType type) {
    for (int i = start; i < end; i++) {
        Token* token = get_token(tokens, i);
        if (token->type == type) {
            return 1;
        }
    }
    return 0;
}

void parse_var_decl(TokenList* tokens, ObjectList* object_list, int* index, VariableDeclaration* var_decl) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_LET) {
        printf("Error: Invalid variable declaration in line %d\n", get_token(tokens, *index)->line);
    }
    
    (*index)++;
    Token* var_name = get_token(tokens, *index);
    if (var_name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable name in line %d\n", var_name->line);
        return;
    }
    var_decl->name = var_name;

    (*index)++;
    Token* var_type = get_token(tokens, *index);
    if (var_type->type != TOKEN_NATIVE_TYPE && var_type->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable type in line %d\n", var_type->line);
        return;
    }
    var_decl->type = var_type;

    (*index)++;

    if (get_token(tokens, *index)->type == TOKEN_END_OF_LINE) {
        Node n;
        n.type = NODE_EMPTY;
        var_decl->value = n;
        return;
    } else if (get_token(tokens, *index)->type != TOKEN_OPERATOR_ASSIGN) {
        printf("Error: Invalid variable declaration in line %d\n", get_token(tokens, *index)->line);
        return;
    }

    (*index)++;
    Node value = parse_expression(tokens, object_list, index);
    var_decl->value = value;
}

void parse_var_assign(TokenList* tokens, ObjectList* object_list, int* index, VariableAssignment* var_assign) {
    Token* var_name = get_token(tokens, *index);
    if (var_name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable name in line %d\n", var_name->line);
        return;
    }
    var_assign->name = var_name;

    (*index)++;

    if (get_token(tokens, *index)->type != TOKEN_OPERATOR_ASSIGN) {
        printf("Error: Invalid variable declaration in line %d\n", get_token(tokens, *index)->line);
    }

    (*index)++;
    Node value = parse_expression(tokens, object_list, index);
    var_assign->value = value;
}

void parse_line(TokenList* tokens, ObjectList* object_list, int* index, int end, Node* node) {
    char contains_assign = if_list_contains_token(tokens, *index, end, TOKEN_OPERATOR_ASSIGN);
    Token* first_token = get_token(tokens, *index);
    
    if (first_token->type == TOKEN_KEYWORD_LET) {
        VariableDeclaration var_decl;
        parse_var_decl(tokens, object_list, index, &var_decl);
        node->type = NODE_VARIABLE_DECLARATION;
        node->data = add_object(object_list, &var_decl, sizeof(VariableDeclaration));
    }
    else if (contains_assign == 1 && first_token->type == TOKEN_IDENTIFIER) {
        VariableAssignment var_assign;
        parse_var_assign(tokens, object_list, index, &var_assign);
        node->type = NODE_VARIABLE_ASSIGNMENT;
        node->data = add_object(object_list, &var_assign, sizeof(VariableAssignment));
    }
    else if (contains_assign == 1) {
        printf("Error: Invalid variable declaration or assignment in line %d\n", first_token->line);
    }
}
Node parse_expression(TokenList* tokens, ObjectList* object_list, int* index) {
    Token* token = get_token(tokens, *index);
    Node node;

    if (token->type == TOKEN_CONSTANT_BOOLEAN || token->type == TOKEN_CONSTANT_INTEGER || token->type == TOKEN_CONSTANT_FLOAT || token->type == TOKEN_CONSTANT_STRING) {
        node.type = NODE_CONSTANT;
        node.data = (void*)token;
        (*index)++;
    } else if (token->type == TOKEN_IDENTIFIER) {
        node.type = NODE_IDENTIFIER;
        node.data = (void*)token;
        (*index)++;
    }
    return node;
}

const char* get_node_type_name(enum NodeType type) {
    switch (type) {
        case NODE_VARIABLE_DECLARATION: return "NODE_VARIABLE_DECLARATION";
        case NODE_FUNCTION_DECLARATION: return "NODE_FUNCTION_DECLARATION";
        case NODE_STRUCT_DECLARATION: return "NODE_STRUCT_DECLARATION";
        case NODE_VARIABLE_ASSIGNMENT: return "NODE_VARIABLE_ASSIGNMENT";
        case NODE_FUNCTION_CALL: return "NODE_FUNCTION_CALL";
        case NODE_WHILE_LOOP: return "NODE_WHILE_LOOP";
        case NODE_IF_STATEMENT: return "NODE_IF_STATEMENT";
        case NODE_RETURN_STATEMENT: return "NODE_RETURN_STATEMENT";
        case NODE_OPERATION: return "NODE_OPERATION";
        case NODE_IDENTIFIER: return "NODE_IDENTIFIER";
        case NODE_CONSTANT: return "NODE_CONSTANT";
        case NODE_BODY: return "NODE_BODY";
        case NODE_EMPTY: return "NODE_EMPTY";
    }
    static char error_message[50];
    sprintf(error_message, "Error: Invalid node type: %d", type);
    return error_message;

}

void debug_parser_node(Node* node) {
    if (node->type == NODE_CONSTANT) {
        Token* token = (Token*)node->data;
        printf("%s ", token->value);
    }
    else if (node->type == NODE_IDENTIFIER) {
        Token* token = (Token*)node->data;
        printf("%s ", token->value);
    }
    else if (node->type == NODE_VARIABLE_DECLARATION) {
        VariableDeclaration* var_decl = (VariableDeclaration*)node->data;
        if (var_decl->value.type == NODE_EMPTY) {
            printf("let %s %s", var_decl->name->value, var_decl->type->value);
        } else {
            printf("let %s %s = ", var_decl->name->value, var_decl->type->value);
            debug_parser_node(&var_decl->value);
        }
    }
    else if (node->type == NODE_VARIABLE_ASSIGNMENT) {
        VariableAssignment* var_assign = (VariableAssignment*)node->data;
        printf("%s = ", var_assign->name->value);
        debug_parser_node(&var_assign->value);
    }
    else {
        printf("[Error, TYPE: %s]\n", get_node_type_name(node->type));
    }
}

void debug_parser(NodeList* ast) {
    printf("AST:\n");
    for (int i = 0; i < ast->size; i++) {
        Node* node = get_node(ast, i);
        debug_parser_node(node);
        printf("\n");
    }
    printf("Finished debugging AST\n");
}

#endif // Parser_H