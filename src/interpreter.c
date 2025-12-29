#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// forward declarations
void ensure_scope_capacity(Interpreter_Object* interp); 
void push_scope(Interpreter_Object* interp);
void pop_scope(Interpreter_Object* interp);
void ensure_call_capacity(Interpreter_Object* interp);
void push_call_frame(Interpreter_Object* interp, CallFrame frame);
void pop_call_frame(Interpreter_Object* interp);
GaloObject interpret_node(Interpreter_Object* interp, Node* node);
void print_out_variable_values(Interpreter_Object* interp);
void print_galo_object(Interpreter_Object* interp, GaloObject object);

void interpret(Interpreter_Object* interp, int input_argc, char** input_argv) {
    // TODO real implementation with main function and arguments

    // testing implementation
    printf("starting interpreter\n");
    push_scope(interp);
    for (int i = 0; i < interp->ast->size; i++) {
        Node* node = get_node(interp->ast, i);
        interpret_node(interp, node);
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

GaloObject get_field_in_struct(Interpreter_Object* interp, GaloObject struct_object, int struct_id, Token* field_name) {
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

    void* position = (unsigned char*)struct_object.data + offset;

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

GaloObject interpret_node(Interpreter_Object* interp, Node* node) {
    if (node->type == NODE_STRUCT_DECLARATION) {
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
                value = get_field_in_struct(interp, value, value.type_id, scoped_identifier->scope[i].name);
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
            const char* value = token->value;
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
    } else {
        printf("TODO nodetype: %s\n", get_node_type_name(node->type));
        exit(1);
    }

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

    interp->call_capacity = 64;
    interp->call_stack = calloc(interp->call_capacity, sizeof(CallFrame));

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

void print_galo_object(Interpreter_Object* interp, GaloObject object) {
    if (object.data == NULL && object.type_id != VOID_TYPE) {
        printf("type: %d, size: %d, data: NULL\n", object.type_id, object.size);
    } else if (object.type_id == VOID_TYPE) {
        printf("type: void, size: %d\n", object.size);
    } else if (object.type_id == INT_TYPE) {
        printf("type: int, size: %d, value: %d\n", object.size, *(int*)object.data);
    } else if (object.type_id == FLOAT_TYPE) {
        printf("type: float, size: %d, value: %f\n", object.size, *(float*)object.data);
    } else if (object.type_id == BYTE_TYPE) {
        printf("type: byte, size: %d, value: %d\n", object.size, *(unsigned char*)object.data);
    } else if (object.type_id == STRING_TYPE) {
        printf("type: string, size: %d, value: %s\n", object.size, (char*)object.data);
    } else if (object.type_id == BOOLEAN_TYPE) {
        printf("type: bool, size: %d, value: %d\n", object.size, *(bool*)object.data);
    } else if (object.type_id == LIST_TYPE) {
        printf("type: list, size: %d\n", object.size);
    } else if (object.type_id == ANY_TYPE) {
        printf("type: any, size: %d\n", object.size);
    } else {
        printf("type: ");
        StructDeclaration* struct_decl = get_struct_from_id(object.type_id, interp->validator_object);
        if (struct_decl == NULL) {
            printf("%d, size: %d value: [struct not found]\n", object.type_id, object.size);
            return;
        } else {
            printf("%s, size: %d\n", struct_decl->name->value, object.size);
        }

        for (int i = 0; i < struct_decl->field_count; i++) {
            GaloObject field = get_field_in_struct(interp, object, object.type_id, struct_decl->fields[i].name.name);
            printf("  ");
            print_galo_object(interp, field);
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
                GaloObject field = get_field_in_struct(interp, object, object.type_id, struct_decl->fields[i].name.name);
                printf("  ");
                print_galo_object(interp, field);
            }
        }
    }
}