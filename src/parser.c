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

bool if_list_contains_token(TokenList* tokens, int start, int end, enum TokenType type) {
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
    if (var_type->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid variable type in line %d\n", var_type->line);
        exit(1);
    }
    var_decl->type = var_type;
    var_decl->id = -1;
    var_decl->type_id = -1;

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
    Identifier* var_scope = malloc(sizeof(Identifier) * 16); // max scope depth is 16 
    
    while (get_token(tokens, *index)->type == TOKEN_IDENTIFIER) {
        Token* name = get_token(tokens, *index);
        Identifier ident;
        ident.id = -1;
        ident.name = name;
        var_scope[size] = ident;
        if (size == 16) {
            printf("Error: Invalid variable assignment, max scope depth is 16 in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        size++;
        (*index)++;
    }

    if (size == 0) {
        printf("Error: Invalid variable assignment in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }

    Identifier* var_scope_address = add_object(object_list, var_scope, sizeof(Identifier) * size);

    ScopedIdentifier scoped_identifier;
    scoped_identifier.const_replacement = NULL;
    scoped_identifier.scope = var_scope_address;
    scoped_identifier.size = size;
    var_assign->identifier = scoped_identifier;

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
    Identifier ident_param_name;
    Identifier ident_param_type;
    ident_param_name.id = -1;
    ident_param_type.id = -1;
    Token* param_name = NULL;
    Token* param_type = NULL;
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
            if (param_type->type != TOKEN_IDENTIFIER) {
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
            ident_param_name.name = param_name;
            ident_param_type.name = param_type;
            parameters[*parameter_count].name = ident_param_name;
            parameters[*parameter_count].type = ident_param_type;
            (*parameter_count)++;
        } else {
            printf("Error: Invalid parameter declaration in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        (*index)++;
    } 

    if (expected == 3) {
        ident_param_name.name = param_name;
        ident_param_type.name = param_type;
        parameters[*parameter_count].name = ident_param_name;
        parameters[*parameter_count].type = ident_param_type;
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
    func_decl->id = -1;

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
    if (get_token(tokens, *index)->type != TOKEN_IDENTIFIER) {
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
    struct_decl->id = -1;
}

void parse_function_call(TokenList* tokens, ObjectList* object_list, int* index, FunctionCall* func_call) {
    int size = 0;
    Identifier* scope = malloc(sizeof(Identifier) * 16); // max scope depth is 16 
    
    while (get_token(tokens, *index)->type == TOKEN_IDENTIFIER) {
        Token* name = get_token(tokens, *index);
        Identifier ident;
        ident.id = -1;
        ident.name = name;
        scope[size] = ident;
        if (size == 16) {
            printf("Error: Invalid function call, max scope depth is 16 in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        size++;
        (*index)++;
    }

    if (size == 0) {
        printf("Error: Invalid variable assignment in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }

    Identifier* scope_address = add_object(object_list, scope, sizeof(Identifier) * size);

    func_call->scope_size = size;
    func_call->scope = scope_address;

    if (scope != NULL) {
        free(scope);
    }
    
    if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_OPEN) {
        printf("Error: Invalid function call syntax in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;

    int argument_count = 0;
    Node* arguments = malloc(sizeof(Node) * 16); // Maximum of 16 arguments per function call

    while (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_CLOSE) {
        if (argument_count == 16) {
            printf("Error: Invalid function call, max arguments is 16 in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }
        arguments[argument_count] = parse_expression(tokens, object_list, index);
        argument_count++;

        if (get_token(tokens, *index)->type == TOKEN_COMMA) {
            (*index)++;
        } else {
            break;
        }
    }

    if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_CLOSE) {
        printf("Error: Invalid function call, expected `)`, found `%s` in line %d\n", get_token(tokens, *index)->value, get_token(tokens, *index)->line);
        exit(1);
    }
    
    (*index)++;

    func_call->arguments = add_object(object_list, arguments, sizeof(Node) * argument_count);
    func_call->argument_count = argument_count;
    
    if (arguments != NULL) {
        free(arguments);
    }
}

void parse_if(TokenList* tokens, ObjectList* object_list, int* index, IfStatement* if_stmt) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_IF) {
        printf("Error: Invalid if statement in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    (*index)++;

    if_stmt->line = get_token(tokens, *index)->line;
    if_stmt->condition = parse_expression(tokens, object_list, index);

    if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE) {
        printf("Error: Invalid if statement, expected end of line but found `%s` in line %d\n", get_token(tokens, *index)->value, get_token(tokens, *index)->line);
        exit(1);
    }

    (*index)++;

    NodeList* body = create_node_list();
    parser(tokens, object_list, body, index);
    if_stmt->body = body;

    if_stmt->elif_count = 0;
    if_stmt->elifs = NULL;
    if_stmt->else_body = NULL;

    if (get_token(tokens, *index - 1)->type == TOKEN_KEYWORD_END) {
        return;
    }

    if (get_token(tokens, *index)->type == TOKEN_KEYWORD_ELSE) {
        (*index)++;
        
        NodeList* else_body = create_node_list();
        parser(tokens, object_list, else_body, index);
        if_stmt->else_body = else_body;
        
        return;
    }

    int elif_count = 0;
    ElifIfStatement* elifs = malloc(sizeof(ElifIfStatement) * 64); // Maximum of 64 elifs per if statement

    while (get_token(tokens, *index)->type == TOKEN_KEYWORD_ELIF) {
        if (elif_count == 64) {
            printf("Error: Invalid if statement, max elifs is 64 in line %d\n", get_token(tokens, *index)->line);
            exit(1);
        }

        (*index)++;
        
        ElifIfStatement elif;

        elif.line = get_token(tokens, *index)->line;
        elif.condition = parse_expression(tokens, object_list, index);
        if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE) {
            printf("Error: Invalid elif statement, expected end of line but found `%s` in line %d\n", get_token(tokens, *index)->value, get_token(tokens, *index)->line);
            exit(1);
        }
        (*index)++;

        NodeList* elif_body = create_node_list();
        parser(tokens, object_list, elif_body, index);
        elif.body = elif_body;
        elifs[elif_count] = elif;
        elif_count++;
    }

    if (elif_count > 0) {
        if_stmt->elifs = add_object(object_list, elifs, sizeof(ElifIfStatement) * elif_count);
        if_stmt->elif_count = elif_count;

        if (elifs != NULL) {
            free(elifs);
        }
    }

    if (get_token(tokens, *index)->type == TOKEN_KEYWORD_ELSE) {
        (*index)++;
        
        NodeList* else_body = create_node_list();
        parser(tokens, object_list, else_body, index);
        if_stmt->else_body = else_body;
    }
}

void parse_while(TokenList* tokens, ObjectList* object_list, int* index, WhileLoop* while_loop) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_WHILE) {
        printf("Error: Invalid while statement in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    (*index)++;

    while_loop->line = get_token(tokens, *index)->line;
    while_loop->condition = parse_expression(tokens, object_list, index);

    if (get_token(tokens, *index)->type != TOKEN_END_OF_LINE) {
        printf("Error: Invalid while statement, expected end of line but found `%s` in line %d\n", get_token(tokens, *index)->value, get_token(tokens, *index)->line);
        exit(1);
    }

    (*index)++;

    NodeList* body = create_node_list();
    parser(tokens, object_list, body, index);
    while_loop->body = body;
}

void parse_const_decl(TokenList* tokens, ObjectList* object_list, int* index, ConstDeclaration* const_decl) {
    if (get_token(tokens, *index)->type != TOKEN_KEYWORD_CONST) {
        printf("Error: Invalid const declaration in line %d\n", get_token(tokens, *index)->line);
        exit(1);
    }
    (*index)++;

    Token* name = get_token(tokens, *index);
    if (name->type != TOKEN_IDENTIFIER) {
        printf("Error: Invalid const name in line %d\n", name->line);
        exit(1);
    }
    (*index)++;

    if (get_token(tokens, *index)->type != TOKEN_OPERATOR_ASSIGN) {
        printf("Error: Invalid const declaration, expected `=` but found `%s` in line %d\n", get_token(tokens, *index)->value, get_token(tokens, *index)->line);
        exit(1);
    }
    (*index)++;

    Node replacement = parse_expression(tokens, object_list, index);

    if (replacement.type != NODE_CONSTANT) {
        printf("Error: Invalid const declaration, expected constant but found `%s` in line %d\n", get_node_type_name(replacement.type), get_token(tokens, *index)->line);
        exit(1);
    }

    const_decl->token = name;
    const_decl->replacement = replacement;
}

bool line_contains_token(TokenList* tokens, int start, enum TokenType type) {
    int index = start;
    while (get_token(tokens, index)->type != TOKEN_END_OF_LINE) {
        if (get_token(tokens, index)->type == type) {
            return 1;
        }
        index++;
    }
    return 0;
}

void parse_line(TokenList* tokens, ObjectList* object_list, int* index, Node* node) {
    Token* first_token = get_token(tokens, *index);

    if (first_token->type == TOKEN_KEYWORD_LET) {
        VariableDeclaration var_decl;
        parse_var_decl(tokens, object_list, index, &var_decl);
        node->type = NODE_VARIABLE_DECLARATION;
        node->data = add_object(object_list, &var_decl, sizeof(VariableDeclaration));
    }
    else if (first_token->type == TOKEN_IDENTIFIER && line_contains_token(tokens, *index, TOKEN_OPERATOR_ASSIGN)) {
        VariableAssignment var_assign;
        parse_var_assign(tokens, object_list, index, &var_assign);
        node->type = NODE_VARIABLE_ASSIGNMENT;
        node->data = add_object(object_list, &var_assign, sizeof(VariableAssignment));
    }
    else if (first_token->type == TOKEN_IDENTIFIER && line_contains_token(tokens, *index, TOKEN_PARENTHESIS_OPEN)) {
        FunctionCall func_call;
        parse_function_call(tokens, object_list, index, &func_call);
        node->type = NODE_FUNCTION_CALL;
        node->data = add_object(object_list, &func_call, sizeof(VariableAssignment));
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
        return_stmt.line = first_token->line;
        node->type = NODE_RETURN_STATEMENT;
        node->data = add_object(object_list, &return_stmt, sizeof(ReturnStatement));
    }
    else if (first_token->type == TOKEN_KEYWORD_BREAK) {
        (*index)++;
        node->type = NODE_BREAK_STATEMENT;
        node->data = get_token(tokens, *index);
    }
    else if (first_token->type == TOKEN_KEYWORD_CONTINUE) {
        (*index)++;
        node->type = NODE_CONTINUE_STATEMENT;
        node->data = get_token(tokens, *index);
    }
    else if (first_token->type == TOKEN_KEYWORD_IF) {
        IfStatement if_stmt;
        parse_if(tokens, object_list, index, &if_stmt);
        node->type = NODE_IF_STATEMENT;
        node->data = add_object(object_list, &if_stmt, sizeof(IfStatement));
    }
    else if (first_token->type == TOKEN_KEYWORD_ELSE) {
        node->type = NODE_END; // it will be handled by the parse_if function
        // don't increment index
    }
    else if (first_token->type == TOKEN_KEYWORD_ELIF) {
        node->type = NODE_END; // it will be handled by the parse_if function
        // don't increment index
    }
    else if (first_token->type == TOKEN_KEYWORD_WHILE) {
        // TODO
        WhileLoop while_loop;
        parse_while(tokens, object_list, index, &while_loop);
        node->type = NODE_WHILE_LOOP;
        node->data = add_object(object_list, &while_loop, sizeof(WhileLoop));
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
    else if (first_token->type == TOKEN_KEYWORD_CONST) {
        ConstDeclaration const_decl;
        parse_const_decl(tokens, object_list, index, &const_decl);
        node->type = NODE_CONST_DECLARATION;
        node->data = add_object(object_list, &const_decl, sizeof(ConstDeclaration));
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
        int size = 0;
        int current_index = *index;
        Identifier* scope = malloc(sizeof(Identifier) * 16); // max scope depth is 16
        while (get_token(tokens, *index)->type == TOKEN_IDENTIFIER) {
            if (size == 16) {
                printf("Error: Invalid variable assignment, max scope depth is 16 in line %d\n", get_token(tokens, *index)->line);
                exit(1);
            }
            Identifier ident;
            ident.id = -1;
            ident.name = get_token(tokens, *index);
            scope[size] = ident;
            (*index)++;
            size++;
        }
        if (get_token(tokens, *index)->type == TOKEN_PARENTHESIS_OPEN) {
            // function call
            *index = current_index;
            FunctionCall func_call;
            parse_function_call(tokens, object_list, index, &func_call);
            node.type = NODE_FUNCTION_CALL;
            node.data = add_object(object_list, &func_call, sizeof(FunctionCall));
            if (scope != NULL) {
                free(scope);
            }
        } else {
            ScopedIdentifier ident;
            ident.const_replacement = NULL;
            ident.size = size;
            ident.scope = add_object(object_list, scope, sizeof(Identifier) * size);
            node.type = NODE_SCOPED_IDENTIFIER;
            node.data = add_object(object_list, &ident, sizeof(ScopedIdentifier));
            if (scope != NULL) {
                free(scope);
            }
        }
    } else if (token->type == TOKEN_PARENTHESIS_OPEN) {
        (*index)++;
        if (get_token(tokens, *index)->type == TOKEN_KEYWORD_NOT) {
            Token* op = get_token(tokens, *index);
            (*index)++;
            
            Node inner = parse_expression(tokens, object_list, index);
            if (get_token(tokens, *index)->type != TOKEN_PARENTHESIS_CLOSE) {
                printf("Error: Invalid token expression `%s` expected `)` for not operation in line %d\n", token->value, token->line);
                exit(1);
            }
            (*index)++;

            Operation operation;
            operation.is_not_operator = 1;
            operation.is_string_or_bool_eq_or_neq_operation = 0;
            operation.left = add_object(object_list, &inner, sizeof(Node));
            operation.right = NULL;
            operation.operator = op;

            node.type = NODE_OPERATION;
            node.data = add_object(object_list, &operation, sizeof(Operation));
            return node;
        }
        
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
            operation.is_string_or_bool_eq_or_neq_operation = 0;
            operation.is_not_operator = 0;
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
        printf("Error: Invalid token expression `%s` with type `%s` in line %d\n", token->value, get_token_type_name(token->type), token->line);
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
        case NODE_BREAK_STATEMENT: return "NODE_BREAK_STATEMENT";
        case NODE_CONTINUE_STATEMENT: return "NODE_CONTINUE_STATEMENT";
        case NODE_OPERATION: return "NODE_OPERATION";
        case NODE_SCOPED_IDENTIFIER: return "NODE_SCOPED_IDENTIFIER";
        case NODE_CONSTANT: return "NODE_CONSTANT";
        case NODE_CONST_DECLARATION: return "NODE_CONST_DECLARATION";
        case NODE_EMPTY: return "NODE_EMPTY";
        case NODE_END: return "NODE_END";
    }
    static char error_message[50];
    sprintf(error_message, "Error: Invalid node type: %d", type);
    return error_message;

}

void debug_parser_node(Node* node, FILE* out) {
    if (node->type == NODE_CONSTANT) {
        Token* token = (Token*)node->data;
        if (token->type == TOKEN_CONSTANT_STRING) {
            fprintf(out, "\"%s\" ", token->value);
        } else {
            fprintf(out, "%s ", token->value);
        }
    }
    else if (node->type == NODE_SCOPED_IDENTIFIER) {
        ScopedIdentifier* scope = (ScopedIdentifier*)node->data;
        for (int i = 0; i < scope->size; i++) {
            fprintf(out, "%s[%d] ", scope->scope[i].name->value, scope->scope[i].id);
        }
    }
    else if (node->type == NODE_VARIABLE_DECLARATION) {
        VariableDeclaration* var_decl = (VariableDeclaration*)node->data;
        if (var_decl->value.type == NODE_EMPTY) {
            fprintf(out, "let %s[%d] %s[%d]", var_decl->name->value, var_decl->id, var_decl->type->value, var_decl->type_id);
        } else {
            fprintf(out, "let %s[%d] %s[%d] = ", var_decl->name->value, var_decl->id, var_decl->type->value, var_decl->type_id);
            debug_parser_node(&var_decl->value, out);
        }
    }
    else if (node->type == NODE_VARIABLE_ASSIGNMENT) {
        VariableAssignment* var_assign = (VariableAssignment*)node->data;
        for (int i = 0; i < var_assign->identifier.size; i++) {
            fprintf(out, "%s[%d] ", var_assign->identifier.scope[i].name->value, var_assign->identifier.scope[i].id);
        }
        fprintf(out, "= ");
        debug_parser_node(&var_assign->value, out);
    }
    else if (node->type == NODE_FUNCTION_DECLARATION) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)node->data;
        fprintf(out, "fun");
        if (func_decl->struct_implementation) {
            fprintf(out, " %s", func_decl->struct_implementation->value);
        }
        fprintf(out, " %s[%d](", func_decl->name->value, func_decl->id);
        for (int i = 0; i < func_decl->parameter_count; i++) {
            if (i > 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "%s[%d] %s[%d]", func_decl->parameters[i].name.name->value, func_decl->parameters[i].name.id, func_decl->parameters[i].type.name->value, func_decl->parameters[i].type.id);
        }
        fprintf(out, ") %s\n", func_decl->return_type->value);

        for (int i = 0; i < func_decl->body->size; i++) {
            debug_parser_node(get_node(func_decl->body, i), out);
            fprintf(out, "\n");
        }
        fprintf(out, "end");
    } else if (node->type == NODE_RETURN_STATEMENT) {
        ReturnStatement* return_stmt = (ReturnStatement*)node->data;
        fprintf(out, "return ");
        if (return_stmt->value.type != NODE_EMPTY) {
            debug_parser_node(&return_stmt->value, out);
        }
    } else if(node->type == NODE_STRUCT_DECLARATION) {
        StructDeclaration* struct_decl = (StructDeclaration*)node->data;
        fprintf(out, "struct %s[%d]\n", struct_decl->name->value, struct_decl->id);
        for (int i = 0; i < struct_decl->field_count; i++) {
            fprintf(out, "    %s[%d] %s[%d]\n", struct_decl->fields[i].name.name->value, struct_decl->fields[i].name.id, struct_decl->fields[i].type.name->value, struct_decl->fields[i].type.id);
        }
        fprintf(out, "end");
    } else if(node->type == NODE_FUNCTION_CALL) {
        FunctionCall* func_call = (FunctionCall*)node->data;
        for (int i = 0; i < func_call->scope_size; i++) {
            fprintf(out, "%s[%d]", func_call->scope[i].name->value, func_call->scope[i].id);
            if (i < func_call->scope_size - 1) {
                fprintf(out, " ");
            }
        }
        fprintf(out, "[%d]( ", func_call->id);
        for (int i = 0; i < func_call->argument_count; i++) {
            if (i > 0) {
                fprintf(out, ", ");
            }
            debug_parser_node(&func_call->arguments[i], out);
        }
        fprintf(out, ") ");
    } else if (node->type == NODE_OPERATION) {
        Operation* operation = (Operation*)node->data;
        if (operation->is_not_operator) {
            fprintf(out, "( not ");
            debug_parser_node(operation->left, out);
            fprintf(out, ") ");
            return;
        }
        fprintf(out, "( ");
        debug_parser_node(operation->left, out);
        fprintf(out, "%s ", operation->operator->value);
        debug_parser_node(operation->right, out);
        fprintf(out, ") ");
    } else if (node->type == NODE_IF_STATEMENT) {
        IfStatement* if_stmt = (IfStatement*)node->data;
        fprintf(out, "if ");
        debug_parser_node(&if_stmt->condition, out);
        fprintf(out, "\n");
        for (int i = 0; i < if_stmt->body->size; i++) {
            debug_parser_node(get_node(if_stmt->body, i), out);
            fprintf(out, "\n");
        }
        if (if_stmt->elif_count > 0) {
            for (int i = 0; i < if_stmt->elif_count; i++) {
                ElifIfStatement* elif = &if_stmt->elifs[i];
                fprintf(out, "elif ");
                Node condition = elif->condition;
                debug_parser_node(&condition, out);
                fprintf(out, "\n");
                for (int j = 0; j < elif->body->size; j++) {
                    debug_parser_node(get_node(elif->body, j), out);
                    fprintf(out, "\n");
                }
            }
        }
        if (if_stmt->else_body != NULL && if_stmt->else_body->size > 0) {
            fprintf(out, "else\n");
            for (int i = 0; i < if_stmt->else_body->size; i++) {
                debug_parser_node(get_node(if_stmt->else_body, i), out);
                fprintf(out, "\n");
            }
        }
        fprintf(out, "end");
    } else if(node->type == NODE_WHILE_LOOP) {
        WhileLoop* while_loop = (WhileLoop*)node->data;
        fprintf(out, "while ");
        debug_parser_node(&while_loop->condition, out);
        fprintf(out, "\n");
        for (int i = 0; i < while_loop->body->size; i++) {
            debug_parser_node(get_node(while_loop->body, i), out);
            fprintf(out, "\n");
        }
        fprintf(out, "end");
    } else if(node->type == NODE_CONST_DECLARATION) {
        ConstDeclaration* const_decl = (ConstDeclaration*)node->data;
        fprintf(out, "const %s = ", const_decl->token->value);
        debug_parser_node(&const_decl->replacement, out);
    }
    else {
        fprintf(out, "[Error, TYPE: %s] ", get_node_type_name(node->type));
    }
}

void debug_parser(NodeList* ast, FILE* out) {
    fprintf(out, "AST Reshape: %d\n", ast->size);
    for (int i = 0; i < ast->size; i++) {
        Node* node = get_node(ast, i);
        debug_parser_node(node, out);
        fprintf(out, "\n");
    }
    fprintf(out, "End AST\n");
}

void emit_ast(NodeList* ast, char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        printf("Error: Could not open file %s for writing ast.\n", output_file);
        return;
    }

    debug_parser(ast, file);

    fclose(file);
}
