#include "galo_headers.h"
#include "builtin_functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// forward declarations
void ensure_scope_capacity(Interpreter_Object* interp); 
void push_scope(Interpreter_Object* interp);
void pop_scope(Interpreter_Object* interp);
GaloObject interpret_node(Interpreter_Object* interp, Node* node);

int interpret(Interpreter_Object* interp, int input_argc, char** input_argv) {
    FunctionDeclaration* main = NULL;
    for (int i = 0; i < interp->validator_object->functions->size; i++) {
        FunctionDeclaration* index = (FunctionDeclaration*)get_object(interp->validator_object->functions, i);
        if (strcmp(index->name->value, "main") == 0) {
            main = index;
            break;
        }
    }

    if (main == NULL) {
        printf("Error no entry point found. Add a `main` function: `fun main() int`\n");
        exit(1);
    }

    if (main->parameter_count > 1 || main->parameter_count < 0) {
        printf("Error: `main` function must have 0 or 1 parameters: `fun main(args list) int` or `fun main() int`\n");
        exit(1);
    }

    if (strcmp(main->return_type->value, "int") != 0 && strcmp(main->return_type->value, "void") != 0) {
        printf("Error: `main` function must return void or int: `fun main() int` or `fun main() void`\n");
        exit(1);
    }

    ObjectList* args = create_object_list();
    if (main->parameter_count == 1) {
        if (main->parameters[0].type.id != LIST_TYPE) {
            printf("Error: `main` function parameter must be a list: `fun main(args list) int`\n");
            exit(1);
        }
        for (int i = 0; i < input_argc; i++) {
            GaloObject arg = string_object_value(input_argv[i]);
            add_object(args, &arg, sizeof(GaloObject));
        }
    }

    GaloObject arg_object = list_object_value(args);

    GaloObject return_value = function_call(interp, main, main->parameter_count, &arg_object);

    if (interp->did_exit) {
        free(return_value.data);
        return interp->exit_code;
    } else if (strcmp(main->return_type->value, "int") != 0) {
        int exit_code = *(int*)return_value.data;
        free(return_value.data);
        return exit_code;
    } else {
        free(return_value.data);
        return 0;
    }
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
    if (interp->variable_count <= id) {
        printf("INTERNAL Runtime Error: Variable id out of range: %d\n", id);
        interp->did_exit = true;
        return;
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

        return void_object_value();
    } else if (node->type == NODE_SCOPED_IDENTIFIER) {
        ScopedIdentifier* scoped_identifier = (ScopedIdentifier*)node->data;

        printf("scoped identifier const replcement %p\n", scoped_identifier->const_replacement);

        if (scoped_identifier->const_replacement != NULL) {
            return interpret_node(interp, scoped_identifier->const_replacement);
        }
        
        if (scoped_identifier->scope[0].id == -1) {
            printf("ERROR: Invalid scoped identifier\n");
            exit(1);
        }
        
        GaloObject value = interp->variables[scoped_identifier->scope[0].id];
        
        if (scoped_identifier->size > 1) {
            printf("INTERPRET SCOPE\n");
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
            GaloObject* var = &interp->variables[lv.variable_id];
            
            // free old value
            free(var->data);

            // allocate and set new one
            var->type_id = rhs.type_id;
            var->size = rhs.size;
            var->data = malloc(rhs.size);

            address = var->data;
        } 

        if (lv.type_id == BYTE_TYPE && rhs.type_id == INT_TYPE) {
            int int_value = *(int*)rhs.data;
            rhs.type_id = BYTE_TYPE;
            rhs.size = sizeof(unsigned char);
            free(rhs.data);
            rhs.data = malloc(sizeof(unsigned char));
            *(unsigned char*)rhs.data = (unsigned char)int_value;
        }
        if (lv.type_id == FLOAT_TYPE && rhs.type_id == INT_TYPE) {
            int int_value = *(int*)rhs.data;
            rhs.type_id = FLOAT_TYPE;
            rhs.size = sizeof(float);
            free(rhs.data);
            rhs.data = malloc(sizeof(float));
            *(float*)rhs.data = (float)int_value;
        }
        
        memcpy(address, rhs.data, lv.size);
        
        free(rhs.data);
        return void_object_value();

    } else if (node->type == NODE_CONSTANT) {
        Token* token = (Token*)node->data;
        if (token->type == TOKEN_CONSTANT_INTEGER) {
            int value = atoi(token->value);
            return int_object_value(value);
        } else if (token->type == TOKEN_CONSTANT_FLOAT) {
            float value = atof(token->value);
            return float_object_value(value);
        } else if (token->type == TOKEN_CONSTANT_STRING) {
            char* value = strdup((char*)token->value);
            resolve_escape_characters(value);
            return string_object_value(value);
        } else if (token->type == TOKEN_CONSTANT_BOOLEAN) {
            bool value;
            if (strcmp(token->value, "true") == 0) {
                value = true;
            } else {
                value = false;
            }
            return bool_object_value(value);
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
                printf("Type not supported for arithmetic operation, lhs type id: %d, rhs type id: %d\n", lhs.type_id, rhs.type_id);
                exit(1);
            }
            if (rhs.type_id == FLOAT_TYPE) {
                right_value = *(float*)rhs.data;
            } else if (rhs.type_id == INT_TYPE) {
                right_value = *(int*)rhs.data;
            } else if (rhs.type_id == BYTE_TYPE) {
                right_value = *(unsigned char*)rhs.data;
            } else {
                printf("Type not supported for arithmetic operation, lhs type id: %d, rhs type id: %d\n", lhs.type_id, rhs.type_id);
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
            FunctionDeclaration* func = (FunctionDeclaration*)get_object(interp->validator_object->functions, func_call->id - interp->validator_object->predefined_functions->size);
            return_value = function_call(interp, func, argument_count, arguments);
        }

        for (int i = 0; i < argument_count && is_predefined_function; i++) {
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
    push_scope(interp);

    for (int i = 0; i < argument_count; i++) {
        Parameter param = function->parameters[i];
        
        GaloObject value = arguments[i];

        if (strcmp(param.type.name->value, "byte") == 0 && value.type_id == INT_TYPE) {
            int int_value = *(int*)value.data;
            value.type_id = BYTE_TYPE;
            value.size = sizeof(unsigned char);
            free(value.data);
            value.data = malloc(sizeof(unsigned char));
            *(unsigned char*)value.data = (unsigned char)int_value;
        }
        if (strcmp(param.type.name->value, "float") == 0 && value.type_id == INT_TYPE) {
            int int_value = *(int*)value.data;
            value.type_id = FLOAT_TYPE;
            value.size = sizeof(float);
            free(value.data);
            value.data = malloc(sizeof(float));
            *(float*)value.data = (float)int_value;
        }

        set_variable_value(interp, param.name.id, value);
        
        scope_add_variable(interp, param.name.id);
    }

    GaloObject return_value = void_object_value();

    for (int i = 0; i < function->body->size; i++) {
        Node* node = get_node(function->body, i);
        return_value = interpret_node(interp, node);

        if (interp->did_exit || interp->did_return || i == function->body->size - 1) {
            break;
        }
    }
    interp->did_return = false;

    GaloObject return_value_copy;
    return_value_copy.type_id = return_value.type_id;
    return_value_copy.size = return_value.size;
    return_value_copy.data = malloc(return_value.size);
    memcpy(return_value_copy.data, return_value.data, return_value.size);

    if (return_value.type_id != VOID_TYPE) {
        free(return_value.data);
    }

    pop_scope(interp);
    return return_value_copy;
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

GaloObject builtin_print_all_variables(Interpreter_Object* interp, GaloObject* args, int arg_count) {
    print_out_variable_values(interp);
    return void_object_value();
}

Interpreter_Object* create_interpreter_object(NodeList* ast, Validator_Object* validator_object) {
    Interpreter_Object* interp = calloc(1, sizeof(Interpreter_Object));

    interp->validator_object = validator_object;
    interp->ast = ast;

    interp->variable_count = validator_object->variables->size;
    interp->variables = calloc(interp->variable_count, sizeof(GaloObject));

    interp->scope_capacity = 64;
    interp->scope_stack = calloc(interp->scope_capacity, sizeof(ScopeFrame));

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

    add_builtin_function(interp, 17, builtin_list_init);
    add_builtin_function(interp, 18, builtin_list_append);
    add_builtin_function(interp, 19, builtin_list_remove);
    add_builtin_function(interp, 20, builtin_list_get);
    add_builtin_function(interp, 21, builtin_list_length);
    add_builtin_function(interp, 22, builtin_list_contains);
    add_builtin_function(interp, 23, builtin_list_index);
    add_builtin_function(interp, 24, builtin_list_set);
    add_builtin_function(interp, 25, builtin_list_insert);
    add_builtin_function(interp, 26, builtin_list_clear);
    add_builtin_function(interp, 27, builtin_list);
    
    add_builtin_function(interp, 28, builtin_is_type);
    add_builtin_function(interp, 29, builtin_print_all_variables);

    return interp;
}

void free_interpreter_object(Interpreter_Object* interp) {
    free(interp->variables);
    free(interp->scope_stack);
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

bool type_is_malloced(GaloObject* object) {
    return object->type_id != INT_TYPE && object->type_id != FLOAT_TYPE && object->type_id != BOOLEAN_TYPE && object->type_id != BYTE_TYPE;
}

void pop_scope(Interpreter_Object* interp) {
    ScopeFrame* scope = &interp->scope_stack[--interp->scope_depth];

    for (int i = 0; i < scope->count; i++) {
        int id = scope->variable_ids[i];
        if (type_is_malloced(&interp->variables[id])) {
            free(interp->variables[id].data);
        }
        interp->variables[id].data = NULL;
    }

    free(scope->variable_ids);
}

void print_galo_object(Interpreter_Object* interp, GaloObject* object) {
    if (object->data == NULL && object->type_id != VOID_TYPE) {
        printf("data-address: %p, type: %d, size: %d, data: NULL\n", object->data, object->type_id, object->size);
    } else if (object->type_id == VOID_TYPE) {
        printf("data-address: %p, type: void, size: %d\n", object->data, object->size);
    } else if (object->type_id == INT_TYPE) {
        printf("data-address: %p, type: int, size: %d, value: %d\n", object->data, object->size, *(int*)object->data);
    } else if (object->type_id == FLOAT_TYPE) {
        printf("data-address: %p, type: float, size: %d, value: %f\n", object->data, object->size, *(float*)object->data);
    } else if (object->type_id == BYTE_TYPE) {
        printf("data-address: %p, type: byte, size: %d, value: %d\n", object->data, object->size, *(unsigned char*)object->data);
    } else if (object->type_id == STRING_TYPE) {
        printf("data-address: %p, type: string, size: %d, value: %s\n", object->data, object->size, (char*)object->data);
    } else if (object->type_id == BOOLEAN_TYPE) {
        printf("data-address: %p, type: bool, size: %d, value: %d\n", object->data, object->size, *(bool*)object->data);
    } else if (object->type_id == LIST_TYPE) {
        printf("data-address: %p, type: list, size: %d\n", object->data, object->size);
        ObjectList* list = (ObjectList*)object->data;
        for (int i = 0; i < list->size; i++) {
            printf("  ");
            print_galo_object(interp, (GaloObject*)list->objects[i]);
        }
    } else if (object->type_id == ANY_TYPE) {
        printf("data-address: %p, type: any, size: %d\n", object->data, object->size);
    } else {
        printf("data-address: %p, type: ", object->data);
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
            printf("id: %d, data-address: %p, type: %d, size: %d, data: NULL\n", i, object.data, object.type_id, object.size);
        } else if (object.type_id == VOID_TYPE) {
            printf("id: %d, data-address: %p, type: void, size: %d\n", i, object.data, object.size);
        } else if (object.type_id == INT_TYPE) {
            printf("id: %d, data-address: %p, type: int, size: %d, value: %d\n", i, object.data, object.size, *(int*)object.data);
        } else if (object.type_id == FLOAT_TYPE) {
            printf("id: %d, data-address: %p, type: float, size: %d, value: %f\n", i, object.data, object.size, *(float*)object.data);
        } else if (object.type_id == BYTE_TYPE) {
            printf("id: %d, data-address: %p, type: byte, size: %d, value: %d\n", i, object.data, object.size, *(unsigned char*)object.data);
        } else if (object.type_id == STRING_TYPE) {
            printf("id: %d, data-address: %p, type: string, size: %d, value: %s\n", i, object.data, object.size, (char*)object.data);
        } else if (object.type_id == BOOLEAN_TYPE) {
            printf("id: %d, data-address: %p, type: bool, size: %d, value: %d\n", i, object.data, object.size, *(bool*)object.data);
        } else if (object.type_id == LIST_TYPE) {
            printf("id: %d, data-address: %p, type: list, size: %d\n", i, object.data, object.size);
            ObjectList* list = (ObjectList*)object.data;
            for (int i = 0; i < list->size; i++) {
                printf("  ");
                print_galo_object(interp, (GaloObject*)list->objects[i]);
            }
        } else if (object.type_id == ANY_TYPE) {
            printf("id: %d, data-address: %p, type: any, size: %d\n", i, object.data, object.size);
        } else {
            printf("id: %d, data-address: %p, type: ", i, object.data);
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