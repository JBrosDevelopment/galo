#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define BUILTIN_FUNCTION(name) GaloObject builtin_##name(Interpreter_Object* interp, GaloObject* args, int arg_count)

// forward declarations
void ensure_scope_capacity(Interpreter_Object* interp); 
void push_scope(Interpreter_Object* interp);
void pop_scope(Interpreter_Object* interp);
void ensure_call_capacity(Interpreter_Object* interp);
void push_call_frame(Interpreter_Object* interp, CallFrame frame);
void pop_call_frame(Interpreter_Object* interp);
GaloObject interpret_node(Interpreter_Object* interp, Node* node);
void print_out_variable_values(Interpreter_Object* interp);
void print_galo_object(Interpreter_Object* interp, GaloObject* object);

void interpret(Interpreter_Object* interp, int input_argc, char** input_argv) {
    // TODO real implementation with main function and arguments

    // testing implementation
    printf("starting interpreter\n");
    push_scope(interp);
    for (int i = 0; i < interp->ast->size; i++) {
        Node* node = get_node(interp->ast, i);
        interpret_node(interp, node);
        
        if (interp->did_exit) {
            break;
        }
    }
    printf("finished interpreter\n");
    print_out_variable_values(interp);
    pop_scope(interp);
}

int get_type_size(Interpreter_Object* interp, int type_id) {
    if (type_id == INT_TYPE) {
        return sizeof(int);
    } else if (type_id == FLOAT_TYPE) {
        return sizeof(float);
    } else if (type_id == BYTE_TYPE) {
        return sizeof(unsigned char);
    } else if (type_id == STRING_TYPE) {
        return sizeof(char*);
    } else if (type_id == BOOLEAN_TYPE) {
        return sizeof(bool);
    } else if (type_id == VOID_TYPE) {
        return 0;
    } else if (type_id == LIST_TYPE) {
        return sizeof(ObjectList);
    } else if (type_id == ANY_TYPE || type_id == TYPE_AS_TYPE) {
        printf("TYPE NOT IMPLEMENTED: in `get_type_size`\n");
        exit(1);
    }

    StructDeclaration* struct_decl = get_struct_from_id(type_id, interp->validator_object);

    if (struct_decl == NULL) {
        printf("Runtime Error: Unknown struct: %s\n", struct_decl->name->value); // shouldn't happen
        exit(1);
    }

    int size = 0;
    for (int i = 0; i < struct_decl->field_count; i++) {
        int type_size = get_type_size(interp, struct_decl->fields[i].type.id);
        size += type_size;
    }

    return size;
}

GaloObject get_field_in_struct(Interpreter_Object* interp, GaloObject* struct_object, int struct_id, Token* field_name) {
    StructDeclaration* struct_decl = get_struct_from_id(struct_id, interp->validator_object);

    if (struct_decl == NULL) {
        printf("Runtime Error: Unknown struct: %s\n", struct_decl->name->value); // shouldn't happen
        exit(1);
    }

    int offset = 0;
    Parameter* field = NULL;
    for (int i = 0; i < struct_decl->field_count; i++) {
        if (strcmp(field_name->value, struct_decl->fields[i].name.name->value) == 0) {
            field = &struct_decl->fields[i];
            break;
        }
        offset += get_type_size(interp, struct_decl->fields[i].type.id);
    }

    if (field == NULL) {
        printf("Runtime Error: Unknown field: %s\n", field_name->value); // shouldn't happen
        exit(1);
    }

    void* position = (unsigned char*)struct_object->data + offset;

    GaloObject object;
    object.data = position;
    object.type_id = field->type.id;
    object.size = get_type_size(interp, field->type.id);

    return object;
}

GaloObject void_object_value() {
    GaloObject object;
    object.type_id = VOID_TYPE;
    object.size = 0;
    object.data = NULL;
    return object;
}

LValue resolve_lvalue(Interpreter_Object* interp, ScopedIdentifier* identifier) {
    LValue lvalue = {0};
    
    int base_id = identifier->scope[0].id;
    GaloObject base = interp->variables[base_id];
    
    if (identifier->size == 1) {
        lvalue.address = base.data;
        lvalue.size = base.size;
        lvalue.variable_id = base_id;
        lvalue.type_id = base.type_id;
    }

    void* current_address = base.data;
    int current_type_id = base.type_id;

    for (int i = 1; i < identifier->size; i++) {
        StructDeclaration* struct_decl = get_struct_from_id(current_type_id, interp->validator_object);

        if (struct_decl == NULL) {
            printf("Runtime Error: Unknown struct: %s\n", struct_decl->name->value); // shouldn't happen
            exit(1);
        }

        int offset = 0;
        Parameter* field = NULL;

        for (int j = 0; j < struct_decl->field_count; j++) {
            if (strcmp(identifier->scope[i].name->value, struct_decl->fields[j].name.name->value) == 0) {
                field = &struct_decl->fields[j];
                break;
            }
            offset += get_type_size(interp, struct_decl->fields[j].type.id);
        }

        if (field == NULL) {
            printf("Runtime Error: Unknown field: %s\n", identifier->scope[i].name->value); // shouldn't happen
            exit(1);
        }

        current_address = (unsigned char*)current_address + offset;
        current_type_id = field->type.id;
    }

    lvalue.address = current_address;
    lvalue.size = get_type_size(interp, current_type_id);
    lvalue.variable_id = -1; // not a variable
    lvalue.type_id = current_type_id;
    return lvalue;
}

void set_variable_value(Interpreter_Object* interp, int id, GaloObject value) {
    if (interp->variables[id].data != NULL) {
        free(interp->variables[id].data);
    }
    interp->variables[id] = value;
}

static void scope_add_variable(Interpreter_Object* interp, int var_id) {
    ScopeFrame* scope = &interp->scope_stack[interp->scope_depth - 1];

    if (scope->count >= scope->capacity) {
        scope->capacity *= 2;
        scope->variable_ids = realloc(scope->variable_ids, sizeof(int) * scope->capacity);
    }

    scope->variable_ids[scope->count++] = var_id;
}

void resolve_escape_characters(char *str) {
    char *src = str;
    char *dst = str;

    while (*src) {
        if (*src == '\\') {
            src++;
            switch (*src) {
                case 'n':  *dst++ = '\n'; break;
                case 't':  *dst++ = '\t'; break;
                case 'r':  *dst++ = '\r'; break;
                case '0':  *dst++ = '\0'; break;
                case '\\': *dst++ = '\\'; break;
                case '"':  *dst++ = '"';  break;
                case '\'': *dst++ = '\''; break;
                default:
                    *dst++ = '\\';
                    *dst++ = *src;
                    break;
            }

            if (*src) {
                src++;
            }
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

GaloObject interpret_node_as_type(Interpreter_Object* interp, Node* node) {
    GaloObject type_object;
    type_object.type_id = TYPE_AS_TYPE;
    type_object.size = sizeof(int);
    type_object.data = malloc(sizeof(int));
    
    ScopedIdentifier* identifier = (ScopedIdentifier*)node->data;
    if (identifier->size != 1) {
        printf("Runtime Error: Invalid type identifier\n"); // shouldn't happen
        exit(1);
    }

    char* type_name = (char*)identifier->scope[0].name->value;
    int type_id = NO_EXPECTED_NODE;

    if (strcmp(type_name, "int") == 0) {
        type_id = INT_TYPE;
    } else if (strcmp(type_name, "float") == 0) {
        type_id = FLOAT_TYPE;
    } else if (strcmp(type_name, "byte") == 0) {
        type_id = BYTE_TYPE;
    } else if (strcmp(type_name, "string") == 0) {
        type_id = STRING_TYPE;
    } else if (strcmp(type_name, "bool") == 0) {
        type_id = BOOLEAN_TYPE;
    } else if (strcmp(type_name, "void") == 0) {
        type_id = VOID_TYPE;
    } else if (strcmp(type_name, "list") == 0) {
        type_id = LIST_TYPE;
    } else if (strcmp(type_name, "any") == 0) {
        type_id = ANY_TYPE;
    } else if (strcmp(type_name, "type") == 0) {
        type_id = TYPE_AS_TYPE;
    } else {
        for (int i = 0; i < interp->validator_object->structs->size; i++) {
            StructDeclaration* struct_decl = (StructDeclaration*)get_object(interp->validator_object->structs, i);
            if (strcmp(struct_decl->name->value, type_name) == 0) {
                type_id = struct_decl->id;
                break;
            }
        }
    }

    if (type_id == NO_EXPECTED_NODE) {
        printf("Error: type name %s not found\n", type_name);
        exit(1);
    }

    *(int*)type_object.data = type_id;

    return type_object;
}

GaloObject interpret_node(Interpreter_Object* interp, Node* node) {
    if (interp->did_exit) { // scope back out if exit was called
        return void_object_value();
    }
    if (node->type == NODE_STRUCT_DECLARATION || node->type == NODE_FUNCTION_DECLARATION) {
        return void_object_value(); // handled by validator
    }
    else if (node->type == NODE_VARIABLE_DECLARATION) {
        VariableDeclaration* var_decl = (VariableDeclaration*)node->data;

        GaloObject value = void_object_value();
        if (var_decl->value.type != NODE_EMPTY) {
            value = interpret_node(interp, &var_decl->value);
        } else {
            // reserve space for value
            value.type_id = var_decl->type_id;
            value.size = get_type_size(interp, var_decl->type_id);
            value.data = malloc(value.size);
        }

        if (strcmp(var_decl->type->value, "byte") == 0 && value.type_id == INT_TYPE) {
            int int_value = *(int*)value.data;
            value.type_id = BYTE_TYPE;
            value.size = sizeof(unsigned char);
            free(value.data);
            value.data = malloc(sizeof(unsigned char));
            *(unsigned char*)value.data = (unsigned char)int_value;
        }
        if (strcmp(var_decl->type->value, "float") == 0 && value.type_id == INT_TYPE) {
            int int_value = *(int*)value.data;
            value.type_id = FLOAT_TYPE;
            value.size = sizeof(float);
            free(value.data);
            value.data = malloc(sizeof(float));
            *(float*)value.data = (float)int_value;
        }
        
        set_variable_value(interp, var_decl->id, value);
        
        scope_add_variable(interp, var_decl->id);
        
        return value;
    } else if (node->type == NODE_SCOPED_IDENTIFIER) {
        ScopedIdentifier* scoped_identifier = (ScopedIdentifier*)node->data;
        
        GaloObject value = interp->variables[scoped_identifier->scope[0].id];
        
        if (scoped_identifier->size > 1) {
            value = interp->variables[scoped_identifier->scope[0].id];
            for (int i = 1; i < scoped_identifier->size; i++) {
                value = get_field_in_struct(interp, &value, value.type_id, scoped_identifier->scope[i].name);
            }
        }
        
        GaloObject value_copy;
        value_copy.type_id = value.type_id;
        value_copy.size = value.size;
        value_copy.data = malloc(value.size);
        memcpy(value_copy.data, value.data, value.size);

        return value_copy;

    } else if (node->type == NODE_VARIABLE_ASSIGNMENT) {
        VariableAssignment* var_assign = (VariableAssignment*)node->data;

        GaloObject rhs = interpret_node(interp, &var_assign->value);
        LValue lv = resolve_lvalue(interp, &var_assign->identifier);

        void* address = lv.address; // defaults if it is in a struct field

        if (lv.variable_id != -1) { // is a variable (not a struct field)
            // free old value
            free(interp->variables[lv.variable_id].data);

            // allocate and set new one
            interp->variables[lv.variable_id].size = rhs.size;
            interp->variables[lv.variable_id].type_id = rhs.type_id;
            interp->variables[lv.variable_id].data = malloc(rhs.size);
            address = interp->variables[lv.variable_id].data;
        } 

        int lhs_type_id = lv.type_id;
        int rhs_type_id = rhs.type_id;
        if (lhs_type_id == BYTE_TYPE && rhs_type_id == INT_TYPE) {
            int int_value = *(int*)rhs.data;
            rhs.type_id = BYTE_TYPE;
            rhs.size = sizeof(unsigned char);
            free(rhs.data);
            rhs.data = malloc(sizeof(unsigned char));
            *(unsigned char*)rhs.data = (unsigned char)int_value;
        }
        if (lhs_type_id == FLOAT_TYPE && rhs_type_id == INT_TYPE) {
            int int_value = *(int*)rhs.data;
            rhs.type_id = FLOAT_TYPE;
            rhs.size = sizeof(float);
            free(rhs.data);
            rhs.data = malloc(sizeof(float));
            *(float*)rhs.data = (float)int_value;
        }
        
        memcpy(address, rhs.data, lv.size);
        
        return void_object_value();

    } else if (node->type == NODE_CONSTANT) {
        Token* token = (Token*)node->data;
        GaloObject object;
        if (token->type == TOKEN_CONSTANT_INTEGER) {
            int value = atoi(token->value);
            object.type_id = INT_TYPE;
            object.size = sizeof(int);
            object.data = malloc(sizeof(int));
            *(int*)object.data = value;
            return object;
        } else if (token->type == TOKEN_CONSTANT_FLOAT) {
            float value = atof(token->value);
            object.type_id = FLOAT_TYPE;
            object.size = sizeof(float);
            object.data = malloc(sizeof(float));
            *(float*)object.data = value;
            return object;
        } else if (token->type == TOKEN_CONSTANT_STRING) {
            char* value = (char*)token->value;
            resolve_escape_characters(value);
            object.type_id = STRING_TYPE;
            object.size = strlen(value) + 1;
            object.data = malloc(object.size);
            strcpy((char*)object.data, value);
            return object;
        } else if (token->type == TOKEN_CONSTANT_BOOLEAN) {
            bool value;
            if (strcmp(token->value, "true") == 0) {
                value = true;
            } else {
                value = false;
            }
            object.type_id = BOOLEAN_TYPE;
            object.size = sizeof(bool);
            object.data = malloc(sizeof(bool));
            *(bool*)object.data = value;
            return object;
        } 
        else {
            printf("UNKNOWN CONSTANT TYPE: %s\n", get_token_type_name(token->type));
        }
    } else if (node->type == NODE_OPERATION) {
        Operation* op = (Operation*)node->data;
        
        GaloObject lhs = interpret_node(interp, op->left);
        
        if (op->is_not_operator) {
            bool value = !*(bool*)lhs.data;
            GaloObject value_copy;
            value_copy.type_id = BOOLEAN_TYPE;
            value_copy.size = sizeof(bool);
            value_copy.data = malloc(sizeof(bool));
            *(bool*)value_copy.data = value;
            free(lhs.data);
            return value_copy;
        }

        const char* operator = op->operator->value;
        
        if (op->operator->type == TOKEN_OPERATOR_ARITHMETIC || op->operator->type == TOKEN_OPERATOR_COMPARISON) {
            GaloObject rhs = interpret_node(interp, op->right);

            double left_value = 0;
            double right_value = 0;
            int size = sizeof(int);
            if (lhs.type_id == FLOAT_TYPE || rhs.type_id == FLOAT_TYPE) {
                size = sizeof(float);
            } else if (lhs.type_id == BYTE_TYPE || rhs.type_id == BYTE_TYPE) {
                size = sizeof(unsigned char);
            }

            if (lhs.type_id == FLOAT_TYPE) {
                left_value = *(float*)lhs.data;
            } else if (lhs.type_id == INT_TYPE) {
                left_value = *(int*)lhs.data;
            } else if (lhs.type_id == BYTE_TYPE) {
                left_value = *(unsigned char*)lhs.data;
            } else {
                printf("Type not supported for arithmetic operation, type id: %d\n", lhs.type_id);
                exit(1);
            }
            if (rhs.type_id == FLOAT_TYPE) {
                right_value = *(float*)rhs.data;
            } else if (rhs.type_id == INT_TYPE) {
                right_value = *(int*)rhs.data;
            } else if (rhs.type_id == BYTE_TYPE) {
                right_value = *(unsigned char*)rhs.data;
            } else {
                printf("Type not supported for arithmetic operation, type id: %d\n", rhs.type_id);
                exit(1);
            }

            if (op->operator->type == TOKEN_OPERATOR_ARITHMETIC) {
                double result = 0;
                if (strcmp(operator, "+") == 0) {
                    result = left_value + right_value;
                } else if (strcmp(operator, "-") == 0) {
                    result = left_value - right_value;
                } else if (strcmp(operator, "*") == 0) {
                    result = left_value * right_value;
                } else if (strcmp(operator, "/") == 0) {
                    if (right_value == 0) {
                        printf("Runtime error: Division by zero\n");
                        exit(1);
                    }
                    result = left_value / right_value;
                } else if (strcmp(operator, "%") == 0) {
                    result = (int)left_value % (int)right_value;
                }

                GaloObject value_copy;
                if (size == sizeof(int)) {
                    value_copy.type_id = INT_TYPE;
                } else if (size == sizeof(float)) {
                    value_copy.type_id = FLOAT_TYPE;
                } else if (size == sizeof(unsigned char)) {
                    value_copy.type_id = BYTE_TYPE;
                }
                value_copy.size = size;
                value_copy.data = malloc(size);
                if (size == sizeof(int)) {
                    *(int*)value_copy.data = (int)result;
                } else if (size == sizeof(float)) {
                    *(float*)value_copy.data = (float)result;
                } else if (size == sizeof(unsigned char)) {
                    *(unsigned char*)value_copy.data = (unsigned char)result;
                }
                free(lhs.data);
                free(rhs.data);
                return value_copy;
            } else { // TOKEN_OPERATOR_COMPARISON
                bool result = false;
                if (strcmp(operator, "==") == 0) {
                    result = left_value == right_value;
                } else if (strcmp(operator, "!=") == 0) {
                    result = left_value != right_value;
                } else if (strcmp(operator, ">") == 0) {
                    result = left_value > right_value;
                } else if (strcmp(operator, "<") == 0) {
                    result = left_value < right_value;
                } else if (strcmp(operator, ">=") == 0) {
                    result = left_value >= right_value;
                } else if (strcmp(operator, "<=") == 0) {
                    result = left_value <= right_value;
                }
                GaloObject value_copy;
                value_copy.type_id = BOOLEAN_TYPE;
                value_copy.size = sizeof(bool);
                value_copy.data = malloc(sizeof(bool));
                *(bool*)value_copy.data = result;
                free(lhs.data);
                free(rhs.data);
                return value_copy;
            }
        } else if (op->operator->type == TOKEN_OPERATOR_LOGICAL) {
            bool left_value = *(bool*)lhs.data;
            bool result = false;
            
            if (strcmp(operator, "and") == 0) {
                if (left_value == false) { // short circuit
                    result = false;
                } else {
                    GaloObject rhs = interpret_node(interp, op->right);
                    bool right_value = *(bool*)rhs.data;
                    result = left_value && right_value;
                    free(rhs.data);
                }
            } else if (strcmp(operator, "or") == 0) {
                if (left_value == true) { // short circuit
                    result = true;
                } else {
                    GaloObject rhs = interpret_node(interp, op->right);
                    bool right_value = *(bool*)rhs.data;
                    result = left_value || right_value;
                    free(rhs.data);
                }
            } 

            GaloObject value_copy;
            value_copy.type_id = BOOLEAN_TYPE;
            value_copy.size = sizeof(bool);
            value_copy.data = malloc(sizeof(bool));
            *(bool*)value_copy.data = result;
            free(lhs.data);
            return value_copy;
        }
    } else if (node->type == NODE_IF_STATEMENT) {
        IfStatement* if_stmt = (IfStatement*)node->data;

        GaloObject condition = interpret_node(interp, &if_stmt->condition);
        bool condition_value = *(bool*)condition.data;
        free(condition.data);

        NodeList* body_to_run = NULL;

        if (condition_value) {
            body_to_run = if_stmt->body;
        } else if (if_stmt->elifs != NULL) {
            for (int i = 0; i < if_stmt->elif_count; i++) {
                ElifIfStatement elif_stmt = if_stmt->elifs[i];
                GaloObject elif_condition = interpret_node(interp, &elif_stmt.condition);
                bool elif_condition_value = *(bool*)elif_condition.data;
                free(elif_condition.data);

                if (elif_condition_value) {
                    body_to_run = elif_stmt.body;
                    break;
                }
            }
        }
        
        if (body_to_run == NULL && if_stmt->else_body != NULL) {
            body_to_run = if_stmt->else_body;
        }

        
        if (body_to_run != NULL) {
            push_scope(interp);
            
            for (int i = 0; i < body_to_run->size; i++) {
                Node* node = get_node(body_to_run, i);
                interpret_node(interp, node);

                if (interp->did_exit) {
                    break;
                }
            }
            pop_scope(interp);
        }


        return void_object_value();
    } else if (node->type == NODE_WHILE_LOOP) {
        WhileLoop* while_loop = (WhileLoop*)node->data;
    
        push_scope(interp);

        bool pre_did_break = interp->did_break;
        bool pre_did_continue = interp->did_continue;
    
        while (1) {
            GaloObject condition = interpret_node(interp, &while_loop->condition);

            if (interp->did_continue) {
                interp->did_continue = false;
                continue;
            }
    
            bool condition_value = *(bool*)condition.data;
            free(condition.data);
    
            if (!condition_value) {
                break;
            }
    
            for (int i = 0; i < while_loop->body->size; i++) {
                Node* body_node = get_node(while_loop->body, i);
                interpret_node(interp, body_node);
                if (interp->did_break || interp->did_continue || interp->did_exit) {
                    break;
                }
            }
            if (interp->did_break || interp->did_exit) {
                interp->did_break = false;
                break;
            }
        }

        interp->did_break = pre_did_break;
        interp->did_continue = pre_did_continue;
    
        pop_scope(interp);
        return void_object_value();
    } else if (node->type == NODE_BREAK_STATEMENT) {
        interp->did_break = true;
        return void_object_value();
    } else if (node->type == NODE_CONTINUE_STATEMENT) {
        interp->did_continue = true;
        return void_object_value();
    } else if (node->type == NODE_RETURN_STATEMENT) {
        interp->did_return = true;
        ReturnStatement* return_stmt = (ReturnStatement*)node->data;
        if (return_stmt->value.type != NODE_EMPTY) {
            GaloObject value = interpret_node(interp, &return_stmt->value);
            return value;
        }
        return void_object_value();
    } else if (node->type == NODE_FUNCTION_CALL) {
        FunctionCall* func_call = (FunctionCall*)node->data;

        int argument_count = func_call->argument_count;
        GaloObject* arguments = calloc(argument_count, sizeof(GaloObject));

        bool is_predefined_function = func_call->id < interp->validator_object->predefined_functions->size;
        PredefinedFunction* predefined_function = NULL;
        if (is_predefined_function) {
            predefined_function = (PredefinedFunction*)get_object(interp->validator_object->predefined_functions, func_call->id);
        }

        for (int i = 0; i < argument_count; i++) {
            Node arg = func_call->arguments[i];
            
            bool argument_is_type = false;

            if (is_predefined_function && predefined_function->parameter_count != 0) {
                if (predefined_function->parameter_count >= i && predefined_function->parameter_ids[i] == TYPE_AS_TYPE) {
                    argument_is_type = true;
                }
            }

            if (argument_is_type) {
                // argument is a type
                arguments[i] = interpret_node_as_type(interp, &arg);
            } else {
                // argument is a value
                arguments[i] = interpret_node(interp, &arg);
            }
        }

        GaloObject return_value = void_object_value();

        if (is_predefined_function) {
            // predefined function call
            return_value = predefined_function_call(interp, predefined_function, argument_count, arguments);
        } else {
            // user defined function call
            FunctionDeclaration* func = (FunctionDeclaration*)get_object(interp->validator_object->functions, func_call->id);
            return_value = function_call(interp, func, argument_count, arguments);
        }

        for (int i = 0; i < argument_count; i++) {
            GaloObject arg = arguments[i];
            free(arg.data);
        }
        free(arguments);

        return return_value;
    } else {
        printf("ERROR: Unknown nodetype: %s\n", get_node_type_name(node->type));
        exit(1);
    }

    return void_object_value();
}

GaloObject predefined_function_call(Interpreter_Object* interp, PredefinedFunction* predefined_function, int argument_count, GaloObject* arguments) {
    int id = predefined_function->id;
    GaloObject (*function)(Interpreter_Object* interp, GaloObject* args, int arg_count) = interp->builtins[id];
    return function(interp, arguments, argument_count);
}
GaloObject function_call(Interpreter_Object* interp, FunctionDeclaration* function, int argument_count, GaloObject* arguments) {
    printf("TODO: function_call\n");
    exit(1);
}

int get_struct_id(Interpreter_Object* interp, const char* name) {
    int id = -1;

    if (name != NULL) {
        if (strcmp(name, "any") == 0) {
            id = ANY_TYPE;
        } else if (strcmp(name, "list") == 0) {
            id = LIST_TYPE;
        } else if (strcmp(name, "int") == 0) {
            id = INT_TYPE;
        } else if (strcmp(name, "float") == 0) {
            id = FLOAT_TYPE;
        } else if (strcmp(name, "byte") == 0) {
            id = BYTE_TYPE;
        } else if (strcmp(name, "string") == 0) {
            id = STRING_TYPE;
        } else if (strcmp(name, "bool") == 0) {
            id = BOOLEAN_TYPE;
        } else if (strcmp(name, "void") == 0) {
            id = VOID_TYPE;
        } else {
            for (int i = 0; i < interp->validator_object->structs->size; i++) {
                StructDeclaration* index = (StructDeclaration*)get_object(interp->validator_object->structs, i);
                if (strcmp(index->name->value, name) == 0) {
                    id = index->id;
                    break;
                }
            }
        }
    }

    return id;
}

int get_builtin_function_id(Interpreter_Object* interp, const char* name, int parent_id) {
    PredefinedFunction* pf = NULL;
    for (int i = 0; i < interp->validator_object->predefined_functions->size; i++) {
        PredefinedFunction* index = (PredefinedFunction*)get_object(interp->validator_object->predefined_functions, i);
        if (strcmp(index->name, name) == 0 && index->parent_id == parent_id) {
            pf = index;
            break;
        }
    }

    return pf->id;
}

void add_builtin_function(Interpreter_Object* interp, int id, GaloObject (*function)(Interpreter_Object* interp, GaloObject* args, int arg_count)) {
    PredefinedFunction* pf = NULL;
    for (int i = 0; i < interp->validator_object->predefined_functions->size; i++) {
        PredefinedFunction* index = (PredefinedFunction*)get_object(interp->validator_object->predefined_functions, i);
        if (index->id == id) {
            pf = index;
            break;
        }
    }

    if (pf == NULL) {
        printf("DEVELOPER ERROR: Function id `%d` not found in predefined functions from the validator object\n", id);
        exit(1);
    }

    interp->builtins[pf->id] = function;
}

static void append_to_string(char** buffer, size_t* capacity, size_t* length, const char* text) {
    size_t add = strlen(text);

    if (*length + add + 1 > *capacity) {
        *capacity = (*capacity + add + 1) * 2;
        char* new_buf = realloc(*buffer, *capacity);
        if (!new_buf) {
            printf("Out of memory\n");
            exit(1);
        }
        *buffer = new_buf;
    }

    memcpy(*buffer + *length, text, add);
    *length += add;
    (*buffer)[*length] = '\0';
}

GaloObject string_object_value(char* string) {
    GaloObject object;
    object.type_id = STRING_TYPE;
    object.size = strlen(string) + 1;
    object.data = string;
    return object;
}

GaloObject int_object_value(int value) {
    GaloObject object;
    object.type_id = INT_TYPE;
    object.size = sizeof(int);
    object.data = malloc(sizeof(int));
    *(int*)object.data = value;
    return object;
}

GaloObject float_object_value(float value) {
    GaloObject object;
    object.type_id = FLOAT_TYPE;
    object.size = sizeof(float);
    object.data = malloc(sizeof(float));
    *(float*)object.data = value;
    return object;
}

GaloObject bool_object_value(bool value) {
    GaloObject object;
    object.type_id = BOOLEAN_TYPE;
    object.size = sizeof(bool);
    object.data = malloc(sizeof(bool));
    *(bool*)object.data = value;
    return object;
}

GaloObject byte_object_value(unsigned char value) {
    GaloObject object;
    object.type_id = BYTE_TYPE;
    object.size = sizeof(unsigned char);
    object.data = malloc(sizeof(unsigned char));
    *(unsigned char*)object.data = value;
    return object;
}

GaloObject list_object_value(ObjectList* list) {
    GaloObject object;
    object.type_id = LIST_TYPE;
    object.size = sizeof(ObjectList);
    object.data = list;
    return object;
}

void runtime_error(Interpreter_Object* interp, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    interp->did_exit = true;
}

BUILTIN_FUNCTION(to_string) {
    GaloObject object = args[0];
    if (object.type_id == STRING_TYPE) {
        char* string = strdup((char*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == INT_TYPE) {
        char* string = calloc(32, sizeof(char));
        sprintf(string, "%d", *(int*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == FLOAT_TYPE) {
        char* string = calloc(32, sizeof(char));
        sprintf(string, "%f", *(float*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == BOOLEAN_TYPE) {
        char* string = calloc(6, sizeof(char));
        if (*(bool*)object.data) {
            sprintf(string, "true");
        } else {
            sprintf(string, "false");
        }
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == BYTE_TYPE) {
        char* string = calloc(8, sizeof(char));
        sprintf(string, "%d", *(unsigned char*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == LIST_TYPE) {
        size_t cap = 64;
        size_t len = 0;
        char* string = malloc(cap);
        string[0] = '\0';
    
        append_to_string(&string, &cap, &len, "[");
    
        ObjectList* list = (ObjectList*)object.data;
    
        for (int i = 0; i < list->size; i++) {
            GaloObject* item = get_object(list, i);
            GaloObject item_str = builtin_to_string(interp, item, 1);
    
            append_to_string(&string, &cap, &len, (char*)item_str.data);
    
            if (i != list->size - 1) {
                append_to_string(&string, &cap, &len, ", ");
            }
    
            free(item_str.data);
        }
    
        append_to_string(&string, &cap, &len, "]");
    
        return string_object_value(string);
    } else {
        size_t cap = 64;
        size_t len = 0;
        char* string = malloc(cap);
        string[0] = '\0';
    
        append_to_string(&string, &cap, &len, "{");
    
        StructDeclaration* decl = get_struct_from_id(object.type_id, interp->validator_object);
    
        for (int i = 0; i < decl->field_count; i++) {
            GaloObject field = get_field_in_struct(interp, &object, object.type_id, decl->fields[i].name.name);
    
            GaloObject field_str = builtin_to_string(interp, &field, 1);
    
            append_to_string(&string, &cap, &len, decl->fields[i].name.name->value);
            append_to_string(&string, &cap, &len, " = ");
            append_to_string(&string, &cap, &len, (char*)field_str.data);
    
            if (i != decl->field_count - 1) {
                append_to_string(&string, &cap, &len, ", ");
            }
    
            free(field_str.data);
        }
    
        append_to_string(&string, &cap, &len, "}");
    
        return string_object_value(string);
    }    
}

BUILTIN_FUNCTION(print) {
    for (int i = 0; i < arg_count; i++) {
        GaloObject arg = args[i];
        GaloObject arg_to_string = builtin_to_string(interp, &arg, 1);
        char* string = (char*)arg_to_string.data;
        printf("%s", string);
        free(arg_to_string.data);
    }

    return void_object_value();
}

BUILTIN_FUNCTION(println) {
    builtin_print(interp, args, arg_count);
    printf("\n");

    return void_object_value();
}

BUILTIN_FUNCTION(exit) {
    int exit_code = *(int*)args[0].data;
    interp->did_exit = true;
    interp->exit_code = exit_code;
    return void_object_value();
}

BUILTIN_FUNCTION(input) {
    size_t capacity = 64;
    size_t length = 0;
    char* buffer = malloc(capacity);

    if (!buffer) {
        perror("malloc");
        exit(1);
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
            if (!buffer) {
                perror("realloc");
                exit(1);
            }
        }
        buffer[length++] = (char)c;
    }

    buffer[length] = '\0';

    GaloObject input_object = string_object_value(buffer);
    return input_object;
}

BUILTIN_FUNCTION(clear) {
    system("clear");
    return void_object_value();
}

BUILTIN_FUNCTION(cast) {   
    int target_type = *(int*)args[0].data;
    GaloObject src = args[1];

    if (target_type == src.type_id) {
        GaloObject copy;
        copy.type_id = src.type_id;
        copy.size = src.size;
        copy.data = malloc(src.size);
        memcpy(copy.data, src.data, src.size);
        return copy;
    }

    GaloObject result;
    result.type_id = target_type;

    switch (target_type) {

    case INT_TYPE: {
        int value;
        char* end;

        switch (src.type_id) {
        case FLOAT_TYPE:
            value = (int)(*(float*)src.data);
            break;
        case BYTE_TYPE:
            value = (int)(*(unsigned char*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1 : 0;
            break;
        case STRING_TYPE:
            value = (int)strtol((char*)src.data, &end, 10);
            if (*end != '\0') {
                runtime_error(interp, "Invalid int cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            break;
        default:
            runtime_error(interp, "Cannot cast type %d to int", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(int);
        result.data = malloc(sizeof(int));
        *(int*)result.data = value;
        return result;
    }

    case FLOAT_TYPE: {
        float value;
        char* end;

        switch (src.type_id) {
        case INT_TYPE:
            value = (float)(*(int*)src.data);
            break;
        case BYTE_TYPE:
            value = (float)(*(unsigned char*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1.0f : 0.0f;
            break;
        case STRING_TYPE:
            value = strtof((char*)src.data, &end);
            if (*end != '\0') {
                runtime_error(interp, "Invalid float cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            break;
        default:
            runtime_error(interp, "Cannot cast type %d to float", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(float);
        result.data = malloc(sizeof(float));
        *(float*)result.data = value;
        return result;
    }

    case BYTE_TYPE: {
        unsigned char value;

        switch (src.type_id) {
        case INT_TYPE:
            value = (unsigned char)(*(int*)src.data);
            break;
        case FLOAT_TYPE:
            value = (unsigned char)(*(float*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1 : 0;
            break;
        case STRING_TYPE: {
            char* end;
            long tmp = strtol((char*)src.data, &end, 10);
            if (*end != '\0' || tmp < 0 || tmp > 255) {
                runtime_error(interp, "Invalid byte cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            value = (unsigned char)tmp;
            break;
        }
        default:
            runtime_error(interp, "Cannot cast type %d to byte", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(unsigned char);
        result.data = malloc(sizeof(unsigned char));
        *(unsigned char*)result.data = value;
        return result;
    }

    case BOOLEAN_TYPE: {
        bool value;

        switch (src.type_id) {
        case INT_TYPE:
            value = (*(int*)src.data) != 0;
            break;
        case FLOAT_TYPE:
            value = (*(float*)src.data) != 0.0f;
            break;
        case BYTE_TYPE:
            value = (*(unsigned char*)src.data) != 0;
            break;
        case STRING_TYPE: {
            char* s = (char*)src.data;
            value = !(s[0] == '\0' ||
                      strcmp(s, "0") == 0 ||
                      strcmp(s, "false") == 0);
            break;
        }
        default:
            runtime_error(interp, "Cannot cast type %d to bool", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(bool);
        result.data = malloc(sizeof(bool));
        *(bool*)result.data = value;
        return result;
    }

    case STRING_TYPE:
        return builtin_to_string(interp, &args[1], 1);

    default:
        runtime_error(interp, "Invalid cast target type id: %d", target_type);
        return void_object_value();
    }
}

BUILTIN_FUNCTION(format) {
    if (arg_count == 0) {
        return string_object_value(strdup(""));
    }

    size_t capacity = 64;
    size_t length = 0;
    char* buffer = malloc(capacity);

    buffer[0] = '\0';

    for (int i = 0; i < arg_count; i++) {
        GaloObject arg = args[i];
        GaloObject str_obj = builtin_to_string(interp, &arg, 1);
        char* str = (char*)str_obj.data;
        size_t str_len = strlen(str);

        while (length + str_len + 1 > capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
            if (!buffer) {
                printf("ERROR: Out of memory in format()\n");
                exit(1);
            }
        }

        memcpy(buffer + length, str, str_len);
        length += str_len;
        buffer[length] = '\0';

        free(str_obj.data);
    }

    return string_object_value(buffer);
}

BUILTIN_FUNCTION(string_length) {
    GaloObject string_object = args[0];

    char* string = (char*)string_object.data;
    int length = strlen(string);
    
    return int_object_value(length);
}

BUILTIN_FUNCTION(string_index) {
    char* string = (char*)args[0].data;
    int index = *(int*)args[1].data;

    unsigned char byte = string[index];

    return byte_object_value(byte);
}

BUILTIN_FUNCTION(string_contains) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strstr(string, substring) != NULL;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_starts_with) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strncmp(string, substring, strlen(substring)) == 0;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_ends_with) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strncmp(string + strlen(string) - strlen(substring), substring, strlen(substring)) == 0;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_replace) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;
    char* replacement = (char*)args[2].data;

    size_t string_length = strlen(string);
    size_t substring_length = strlen(substring);
    size_t replacement_length = strlen(replacement);

    if (substring_length == 0) {
        return string_object_value(strdup(string));
    }

    size_t count = 0;
    char* cursor = string;
    while ((cursor = strstr(cursor, substring)) != NULL) {
        count++;
        cursor += substring_length;
    }

    if (count == 0) {
        return string_object_value(strdup(string));
    }

    size_t new_length = string_length + count * (replacement_length - substring_length);

    char* new_string = malloc(new_length + 1);
    if (!new_string) {
        printf("ERROR: Out of memory in string replace()\n");
        exit(1);
    }

    char* src = string;
    char* dst = new_string;

    while ((cursor = strstr(src, substring)) != NULL) {
        size_t bytes = cursor - src;
        memcpy(dst, src, bytes);
        dst += bytes;

        memcpy(dst, replacement, replacement_length);
        dst += replacement_length;

        src = cursor + substring_length;
    }

    strcpy(dst, src);

    return string_object_value(new_string);
}

BUILTIN_FUNCTION(string_sub) {
    char* string = (char*)args[0].data;
    int start = *(int*)args[1].data;
    int end = *(int*)args[2].data;

    char* substring = malloc(end - start + 1);
    if (!substring) {
        printf("ERROR: Out of memory in string sub()\n");
        exit(1);
    }

    memcpy(substring, string + start, end - start);
    substring[end - start] = '\0';

    return string_object_value(substring);
}

BUILTIN_FUNCTION(string_split) {
    char* string = (char*)args[0].data;
    char separator = ((char*)args[1].data)[0];

    ObjectList* result = create_object_list();

    char* start = string;
    char* cursor;

    while ((cursor = strchr(start, separator)) != NULL) {
        size_t len = cursor - start;

        char* part = malloc(len + 1);
        if (!part) {
            printf("ERROR: Out of memory in string_split()\n");
            exit(1);
        }

        memcpy(part, start, len);
        part[len] = '\0';

        GaloObject* object = malloc(sizeof(GaloObject));
        *object = string_object_value(part);

        add_object(result, object, sizeof(GaloObject));

        start = cursor + 1;
    }

    size_t len = strlen(start);
    char* part = malloc(len + 1);
    if (!part) {
        printf("ERROR: Out of memory in string_split()\n");
        exit(1);
    }

    memcpy(part, start, len);
    part[len] = '\0';

    GaloObject* object = malloc(sizeof(GaloObject));
    *object = string_object_value(part);

    add_object(result, object, sizeof(GaloObject));
    
    
    return list_object_value(result);
}

BUILTIN_FUNCTION(string_concat) {
    if (arg_count == 0) {
        return string_object_value(strdup(""));
    }

    size_t total_length = 0;
    for (int i = 0; i < arg_count; i++) {
        total_length += strlen((char*)args[i].data);
    }

    char* result = malloc(total_length + 1);
    if (!result) {
        printf("ERROR: Out of memory in string_concat()\n");
        exit(1);
    }

    char* cursor = result;
    for (int i = 0; i < arg_count; i++) {
        char* s = (char*)args[i].data;
        size_t len = strlen(s);
        memcpy(cursor, s, len);
        cursor += len;
    }

    result[total_length] = '\0';

    return string_object_value(result);
}

Interpreter_Object* create_interpreter_object(NodeList* ast, Validator_Object* validator_object) {
    Interpreter_Object* interp = calloc(1, sizeof(Interpreter_Object));

    interp->validator_object = validator_object;
    interp->ast = ast;

    interp->variable_count = validator_object->variables->size;
    interp->variables = calloc(interp->variable_count, sizeof(GaloObject));

    interp->scope_capacity = 64;
    interp->scope_stack = calloc(interp->scope_capacity, sizeof(ScopeFrame));

    interp->call_capacity = 64;
    interp->call_stack = calloc(interp->call_capacity, sizeof(CallFrame));

    interp->builtin_count = validator_object->predefined_functions->size;
    interp->builtins = calloc(interp->builtin_count, sizeof(GaloObject (*)(Interpreter_Object* interp, GaloObject* args, int arg_count)));

    // Builtin functions, ids are defined in the validator `add_predefined_functions` function
    add_builtin_function(interp, 0, builtin_print);
    add_builtin_function(interp, 1, builtin_println);
    add_builtin_function(interp, 2, builtin_exit);
    add_builtin_function(interp, 3, builtin_input);
    add_builtin_function(interp, 4, builtin_clear);
    add_builtin_function(interp, 5, builtin_cast);
    add_builtin_function(interp, 6, builtin_to_string);
    add_builtin_function(interp, 7, builtin_format);

    add_builtin_function(interp, 8, builtin_string_length);
    add_builtin_function(interp, 9, builtin_string_index);
    add_builtin_function(interp, 10, builtin_string_contains);
    add_builtin_function(interp, 11, builtin_string_starts_with);
    add_builtin_function(interp, 12, builtin_string_ends_with);
    add_builtin_function(interp, 13, builtin_string_replace);
    add_builtin_function(interp, 14, builtin_string_sub);
    add_builtin_function(interp, 15, builtin_string_split);
    add_builtin_function(interp, 16, builtin_string_concat);

    return interp;
}

void free_interpreter_object(Interpreter_Object* interp) {
    free(interp->variables);
    free(interp->scope_stack);
    free(interp->call_stack);
    free(interp);
}

void ensure_scope_capacity(Interpreter_Object* interp) {
    if (interp->scope_depth >= interp->scope_capacity) {
        interp->scope_capacity *= 2;
        interp->scope_stack = realloc(
            interp->scope_stack,
            sizeof(ScopeFrame) * interp->scope_capacity
        );
    }
}

void push_scope(Interpreter_Object* interp) {
    ensure_scope_capacity(interp);

    ScopeFrame* scope = &interp->scope_stack[interp->scope_depth++];
    scope->count = 0;
    scope->capacity = 8;
    scope->variable_ids = malloc(sizeof(int) * scope->capacity);
}

void pop_scope(Interpreter_Object* interp) {
    ScopeFrame* scope = &interp->scope_stack[--interp->scope_depth];

    for (int i = 0; i < scope->count; i++) {
        int id = scope->variable_ids[i];
        free(interp->variables[id].data);
        interp->variables[id].data = NULL;
    }

    free(scope->variable_ids);
}

void ensure_call_capacity(Interpreter_Object* interp) {
    if (interp->call_depth >= interp->call_capacity) {
        interp->call_capacity *= 2;
        interp->call_stack = realloc(
            interp->call_stack,
            sizeof(CallFrame) * interp->call_capacity
        );
    }
}

void push_call_frame(Interpreter_Object* interp, CallFrame frame) {
    ensure_call_capacity(interp);
    interp->call_stack[interp->call_depth++] = frame;
}

void pop_call_frame(Interpreter_Object* interp) {
    interp->call_depth--;

    CallFrame frame = interp->call_stack[interp->call_depth];
    free(frame.return_node);
}

void print_galo_object(Interpreter_Object* interp, GaloObject* object) {
    if (object->data == NULL && object->type_id != VOID_TYPE) {
        printf("type: %d, size: %d, data: NULL\n", object->type_id, object->size);
    } else if (object->type_id == VOID_TYPE) {
        printf("type: void, size: %d\n", object->size);
    } else if (object->type_id == INT_TYPE) {
        printf("type: int, size: %d, value: %d\n", object->size, *(int*)object->data);
    } else if (object->type_id == FLOAT_TYPE) {
        printf("type: float, size: %d, value: %f\n", object->size, *(float*)object->data);
    } else if (object->type_id == BYTE_TYPE) {
        printf("type: byte, size: %d, value: %d\n", object->size, *(unsigned char*)object->data);
    } else if (object->type_id == STRING_TYPE) {
        printf("type: string, size: %d, value: %s\n", object->size, (char*)object->data);
    } else if (object->type_id == BOOLEAN_TYPE) {
        printf("type: bool, size: %d, value: %d\n", object->size, *(bool*)object->data);
    } else if (object->type_id == LIST_TYPE) {
        printf("type: list, size: %d\n", object->size);
        ObjectList* list = (ObjectList*)object->data;
        for (int i = 0; i < list->size; i++) {
            printf("  ");
            print_galo_object(interp, (GaloObject*)list->objects[i]);
        }
    } else if (object->type_id == ANY_TYPE) {
        printf("type: any, size: %d\n", object->size);
    } else {
        printf("type: ");
        StructDeclaration* struct_decl = get_struct_from_id(object->type_id, interp->validator_object);
        if (struct_decl == NULL) {
            printf("%d, size: %d value: [struct not found]\n", object->type_id, object->size);
            return;
        } else {
            printf("%s, size: %d\n", struct_decl->name->value, object->size);
        }

        for (int i = 0; i < struct_decl->field_count; i++) {
            GaloObject field = get_field_in_struct(interp, object, object->type_id, struct_decl->fields[i].name.name);
            printf("  ");
            print_galo_object(interp, &field);
        }
    }
}

void print_out_variable_values(Interpreter_Object* interp) {
    for (int i = 0; i < interp->variable_count; i++) {
        GaloObject object = interp->variables[i];
        if (object.data == NULL && object.type_id != VOID_TYPE) {
            printf("id: %d, type: %d, size: %d, data: NULL\n", i, object.type_id, object.size);
        } else if (object.type_id == VOID_TYPE) {
            printf("id: %d, type: void, size: %d\n", i, object.size);
        } else if (object.type_id == INT_TYPE) {
            printf("id: %d, type: int, size: %d, value: %d\n", i, object.size, *(int*)object.data);
        } else if (object.type_id == FLOAT_TYPE) {
            printf("id: %d, type: float, size: %d, value: %f\n", i, object.size, *(float*)object.data);
        } else if (object.type_id == BYTE_TYPE) {
            printf("id: %d, type: byte, size: %d, value: %d\n", i, object.size, *(unsigned char*)object.data);
        } else if (object.type_id == STRING_TYPE) {
            printf("id: %d, type: string, size: %d, value: %s\n", i, object.size, (char*)object.data);
        } else if (object.type_id == BOOLEAN_TYPE) {
            printf("id: %d, type: bool, size: %d, value: %d\n", i, object.size, *(bool*)object.data);
        } else if (object.type_id == LIST_TYPE) {
            printf("id: %d, type: list, size: %d\n", i, object.size);
        } else if (object.type_id == ANY_TYPE) {
            printf("id: %d, type: any, size: %d\n", i, object.size);
        } else {
            printf("id: %d, type: ", i);
            StructDeclaration* struct_decl = get_struct_from_id(object.type_id, interp->validator_object);
            if (struct_decl == NULL) {
                printf("%d, size: %d value: [struct not found]\n", object.type_id, object.size);
                continue;
            } else {
                printf("%s, size: %d\n", struct_decl->name->value, object.size);
            }

            for (int i = 0; i < struct_decl->field_count; i++) {
                GaloObject field = get_field_in_struct(interp, &object, object.type_id, struct_decl->fields[i].name.name);
                printf("  ");
                print_galo_object(interp, &field);
            }
        }
    }
}