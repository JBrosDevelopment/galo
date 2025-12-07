#ifndef Parser_H
#define Parser_H

#include "galo_headers.h"
#include <stdio.h>
#include <stdlib.h>

void parse_line(TokenList* tokens, ObjectList* object_list, int* index, Node* node); // Forward declaration
Node parse_expression(TokenList* tokens, ObjectList* object_list, int* index); // Forward declaration

void parser(TokenList* tokens, ObjectList* object_list, NodeList* ast, int* index) {
    for (; *index < tokens->size; (*index)++) {
        Token* token = get_token(tokens, *index);
        Node node;
        node.type = NODE_EMPTY;

        if (token->type == TOKEN_EOF) {
            break;
        }
        if (token->type == TOKEN_END_OF_LINE) {
            continue;
        }

        parse_line(tokens, object_list, index, &node);

        if (node.type == NODE_END) {
            break;
        }
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
        exit(1);
    }
    
    (*index)++;
    Token* var_name = get_token(tokens, *index);
    if (var_name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable name in line %d\n", var_name->line);
        exit(1);
    }
    var_decl->name = var_name;

    (*index)++;
    Token* var_type = get_token(tokens, *index);
    if (var_type->type != TOKEN_NATIVE_TYPE && var_type->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable type in line %d\n", var_type->line);
        exit(1);
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
        exit(1);
    }

    (*index)++;
    Node value = parse_expression(tokens, object_list, index);
    var_decl->value = value;
}

void parse_var_assign(TokenList* tokens, ObjectList* object_list, int* index, VariableAssignment* var_assign) {
    int size = 0;
    Token** var_scope = malloc(sizeof(Token*) * 16); // max scope depth is 16 
    
    while (get_token(tokens, *index)->type == TOKEN_IDENTIFIER) {
        Token* name = get_token(tokens, *index);
        var_scope[size] = name;
        if (size == 16) {
            printf("Error: Invalid variable assignment, max scope depth is 16 in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        size++;
        (*index)++;
    }

    Token** var_scope_address = add_object(object_list, var_scope, sizeof(Token*) * size);

    var_assign->scope_size = size;
    var_assign->scope = var_scope_address;

    if (var_scope != NULL) {
        free(var_scope);
    }

    if (get_token(tokens, *index)->type != TOKEN_OPERATOR_ASSIGN) {
        printf("Error: Invalid variable assignment in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }

    (*index)++;
    Node value = parse_expression(tokens, object_list, index);
    var_assign->value = value;
}

Parameter* parse_parameter_list(TokenList* tokens, ObjectList* object_list, int* index, int max_parameters, enum TokenType Seperator, enum TokenType End, Parameter* parameters, int* parameter_count) {
    Token* param_name;
    Token* param_type;
    int expected = 1; // 1 for name, 2 for type, 3 for comma
    while (get_token(tokens, *index)->type != End) {
        if (expected == 1) {
            param_name = get_token(tokens, *index);
            if (param_name->type != TOKEN_IDENTIFIER) {
                printf("Error: Invalid parameter name in line %d\n", param_name->line);
                exit(1);
            }
            expected = 2;
        } else if (expected == 2) {
            param_type = get_token(tokens, *index);
            if (param_type->type != TOKEN_NATIVE_TYPE && param_type->type != TOKEN_IDENTIFIER) {
                printf("Error: Invalid parameter type in line %d\n", param_type->line);
                exit(1);
            }
            expected = 3;
        } else if (expected == 3) {
            if (get_token(tokens, *index)->type != Seperator) {
                printf("Error: Invalid parameter declaration in line %d\n", get_token(tokens, *index)->line);
                exit(1);
            }
            if (*parameter_count == max_parameters) {
                printf("Error: Max of %d parameters in line %d\n", max_parameters, get_token(tokens, *index)->line);
                exit(1);
            }
            expected = 1;
            parameters[*parameter_count].name = param_name;
            parameters[*parameter_count].type = param_type;
            (*parameter_count)++;
        } else {
            printf("Error: Invalid parameter declaration in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        (*index)++;
    } 

    if (expected == 3) {
        parameters[*parameter_count].name = param_name;
        parameters[*parameter_count].type = param_type;
        (*parameter_count)++;
    }

    if (expected != 1 && expected != 3) {
        printf("Error: Invalid parameter declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }

    Parameter* parameter_address = add_object(object_list, parameters, sizeof(Parameter) * *parameter_count);

    return parameter_address;
}

void parse_function(TokenList* tokens, ObjectList* object_list, int* index, FunctionDeclaration* func_decl) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_FUN) {
        printf("Error: Invalid function declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;
    Token* func_name = get_token(tokens, *index);
    Token* struct_implementation = NULL;
    if (func_name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid function name in line %d\n", func_name->line);
        exit(1);
    }

    (*index)++;
    if (get_token(tokens, *index)->type == TOKEN_IDENTIFIER) {
        struct_implementation = func_name;
        func_name = get_token(tokens, *index);
        (*index)++;
    }

    func_decl->name = func_name;
    func_decl->struct_implementation = struct_implementation;

    if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_OPEN) {
        printf("Error: Invalid function declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;

    int parameter_count = 0;
    Parameter* parameters = malloc(sizeof(Parameter) * 16); // Maximum of 16 parameters per function
    Parameter* parameter_address = parse_parameter_list(tokens, object_list, index, 16, TOKEN_COMMA, TOKEN_PARENTHESIS_CLOSE, parameters, &parameter_count);

    if (parameters != NULL) {
        free(parameters);
    } 

    func_decl->parameters = parameter_address;
    func_decl->parameter_count = parameter_count;

    (*index)++;

    Token* return_type = get_token(tokens, *index);
    if (get_token(tokens, *index)->type != TOKEN_IDENTIFIER && return_type->type != TOKEN_NATIVE_TYPE) {
        printf("Error: Invalid function return type `%s` in line %d\n", return_type->value, func_name->line);
        exit(1);
    }
    
    func_decl->return_type = return_type;

    (*index)++;

    if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE) {
        printf("Error: Invalid function declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;
    
    NodeList* body = create_node_list();
    parser(tokens, object_list, body, index);
    func_decl->body = body;
}

void parse_struct(TokenList* tokens, ObjectList* object_list, int* index, StructDeclaration* struct_decl) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_STRUCT) {
        printf("Error: Invalid struct declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;
    Token* struct_name = get_token(tokens, *index);
    if (struct_name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid struct name in line %d\n", struct_name->line);
        exit(1);
    }
    
    struct_decl->name = struct_name;

    (*index)++;
    if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE) {
        printf("Error: Invalid struct declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;
    
    int field_count = 0;
    Parameter* fields = malloc(sizeof(Parameter) * 32); // Maximum of 32 fields per struct
    Parameter* field_address = parse_parameter_list(tokens, object_list, index, 32, TOKEN_END_OF_LINE, TOKEN_KEYWORD_END, fields, &field_count);

    if (fields != NULL) {
        free(fields);
    } 

    struct_decl->fields = field_address;
    struct_decl->field_count = field_count;
}

void parse_line(TokenList* tokens, ObjectList* object_list, int* index, Node* node) {
    Token* first_token = get_token(tokens, *index);

    if (first_token->type == TOKEN_KEYWORD_LET) {
        VariableDeclaration var_decl;
        parse_var_decl(tokens, object_list, index, &var_decl);
        node->type = NODE_VARIABLE_DECLARATION;
        node->data = add_object(object_list, &var_decl, sizeof(VariableDeclaration));
    }
    else if (first_token->type == TOKEN_IDENTIFIER) {
        VariableAssignment var_assign;
        parse_var_assign(tokens, object_list, index, &var_assign);
        node->type = NODE_VARIABLE_ASSIGNMENT;
        node->data = add_object(object_list, &var_assign, sizeof(VariableAssignment));
    }
    else if (first_token->type == TOKEN_KEYWORD_FUN) {
        FunctionDeclaration func_decl;
        parse_function(tokens, object_list, index, &func_decl);
        node->type = NODE_FUNCTION_DECLARATION;
        node->data = add_object(object_list, &func_decl, sizeof(FunctionDeclaration));
    }
    else if (first_token->type == TOKEN_KEYWORD_STRUCT) {
        StructDeclaration struct_decl;
        parse_struct(tokens, object_list, index, &struct_decl);
        node->type = NODE_STRUCT_DECLARATION;
        node->data = add_object(object_list, &struct_decl, sizeof(StructDeclaration));
    }
    else if (first_token->type == TOKEN_KEYWORD_RETURN) {
        ReturnStatement return_stmt;
        Node value;
        (*index)++;
        if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE && get_token(tokens, *index)->type != TOKEN_EOF) {
            value = parse_expression(tokens, object_list, index);
        } else {
            value.type = NODE_EMPTY;
        }
        return_stmt.value = value;
        node->type = NODE_RETURN_STATEMENT;
        node->data = add_object(object_list, &return_stmt, sizeof(ReturnStatement));
    }
    else if (first_token->type == TOKEN_KEYWORD_END) {
        node->type = NODE_END;
        (*index)++;
    }
    else if (first_token->type == TOKEN_EOF) {
        node->type = NODE_END;
    }
    else if (first_token->type == TOKEN_END_OF_LINE) {
        node->type = NODE_EMPTY;
        (*index)++;
    }
    else {
        printf("Error: Invalid token `%s` in line %d\n", first_token->value, first_token->line);
        exit(1);
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
    } else if (token->type == TOKEN_PARENTHESIS_OPEN) {
        (*index)++;
        Node left = parse_expression(tokens, object_list, index);
        if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_CLOSE) {
            Token* op = get_token(tokens, *index);
            if (op->type != TOKEN_OPERATOR_ARITHMETIC && op->type != TOKEN_OPERATOR_COMPARISON && op->type != TOKEN_OPERATOR_LOGICAL) {
                printf("Error: Invalid operator `%s` in line %d\n", op->value, op->line);
                exit(1);
            }
            (*index)++;
            Node right = parse_expression(tokens, object_list, index);
            
            if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_CLOSE) {
                printf("Error: Invalid token expression `%s` expected `)` in line %d\n", token->value, token->line);
                exit(1);
            }

            (*index)++;

            Operation operation;
            operation.left = add_object(object_list, &left, sizeof(Node));
            operation.right = add_object(object_list, &right, sizeof(Node));
            operation.operator = op;

            node.type = NODE_OPERATION;
            node.data = add_object(object_list, &operation, sizeof(Operation));
        } else {
            (*index)++;
            node = left;
        }
    } else {
        printf("Error: Invalid token expression `%s` in line %d\n", token->value, token->line);
        exit(1);
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
        case NODE_END: return "NODE_END";
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
        for (int i = 0; i < var_assign->scope_size; i++) {
            printf("%s ", var_assign->scope[i]->value);
        }
        printf("= ");
        debug_parser_node(&var_assign->value);
    }
    else if (node->type == NODE_FUNCTION_DECLARATION) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)node->data;
        printf("fun");
        if (func_decl->struct_implementation) {
            printf(" %s", func_decl->struct_implementation->value);
        }
        printf(" %s(", func_decl->name->value);
        for (int i = 0; i < func_decl->parameter_count; i++) {
            if (i > 0) {
                printf(", ");
            }
            printf("%s %s", func_decl->parameters[i].name->value, func_decl->parameters[i].type->value);
        }
        printf(") %s\n", func_decl->return_type->value);

        for (int i = 0; i < func_decl->body->size; i++) {
            debug_parser_node(get_node(func_decl->body, i));
            printf("\n");
        }
        printf("end");
    } else if (node->type == NODE_RETURN_STATEMENT) {
        ReturnStatement* return_stmt = (ReturnStatement*)node->data;
        printf("return ");
        if (return_stmt->value.type != NODE_EMPTY) {
            debug_parser_node(&return_stmt->value);            
        }
    } else if(node->type == NODE_STRUCT_DECLARATION) {
        StructDeclaration* struct_decl = (StructDeclaration*)node->data;
        printf("struct %s\n", struct_decl->name->value);
        for (int i = 0; i < struct_decl->field_count; i++) {
            printf("    %s %s\n", struct_decl->fields[i].name->value, struct_decl->fields[i].type->value);
        }
        printf("end");
    } else if (node->type == NODE_OPERATION) {
        Operation* operation = (Operation*)node->data;
        printf("(");
        debug_parser_node(operation->left);
        printf("%s ", operation->operator->value);
        debug_parser_node(operation->right);
        printf(")");
    }
    else {
        printf("[Error, TYPE: %s] ", get_node_type_name(node->type));
    }
}

void debug_parser(NodeList* ast) {
    printf("AST: %d\n", ast->size);
    for (int i = 0; i < ast->size; i++) {
        Node* node = get_node(ast, i);
        debug_parser_node(node);
        printf("\n");
    }
    printf("Finished debugging AST\n");
}

#endif // Parser_H