#ifndef Validator_H
#define Validator_H

#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MEMORY_ADDRESS_PADDING_LOWER -64000
#define MEMORY_ADDRESS_PADDING_UPPER 64000

#define TYPE_NOT_IN_BOUNDS(type) type < MEMORY_ADDRESS_PADDING_LOWER || type > MEMORY_ADDRESS_PADDING_UPPER

#define NO_EXPECTED_NODE -67
#define VOID_TYPE -1
#define INT_TYPE -2
#define STRING_TYPE -3
#define BOOLEAN_TYPE -4
#define FLOAT_TYPE -5

int validate_node(Node* node, Validator_Object* validator_object); // Forward declaration

void validator(NodeList* ast, Validator_Object* validator_object) {
    for (int i = 0; i < ast->size; i++) {
        Node* node = get_node(ast, i);
        validate_node(node, validator_object);
    }
}

int get_id_from_name(Token* name, Validator_Object* validator_object) {
    if (name->type == TOKEN_NATIVE_TYPE) {
        if (strcmp(name->value, "void") == 0) {
            return VOID_TYPE; 
        }
        if (strcmp(name->value, "int") == 0) {
            return INT_TYPE;
        }
        if (strcmp(name->value, "float") == 0) {
            return FLOAT_TYPE;
        }
        if (strcmp(name->value, "string") == 0) {
            return STRING_TYPE;
        }
        if (strcmp(name->value, "bool") == 0) {
            return BOOLEAN_TYPE;
        }
    }
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        if (strcmp(name->value, var_decl->name->value) == 0) {
            return var_decl->id;
        }
    }
    for (int i = 0; i < validator_object->functions->size; i++) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
        if (strcmp(name->value, func_decl->name->value) == 0) {
            return func_decl->id;
        }
    }
    for (int i = 0; i < validator_object->structs->size; i++) {
        StructDeclaration* struct_decl = (StructDeclaration*)validator_object->structs->objects[i];
        if (strcmp(name->value, struct_decl->name->value) == 0) {
            return struct_decl->id;
        }
    }

    return NO_EXPECTED_NODE;
}

const char* get_name_from_id(int id, Validator_Object* validator_object) {
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        if (var_decl->id == id) {
            return var_decl->name->value;
        }
    }
    for (int i = 0; i < validator_object->functions->size; i++) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
        if (func_decl->id == id) {
            return func_decl->name->value;
        }
    }
    for (int i = 0; i < validator_object->structs->size; i++) {
        StructDeclaration* struct_decl = (StructDeclaration*)validator_object->structs->objects[i];
        if (struct_decl->id == id) {
            return struct_decl->name->value;
        }
    }
    if (id == VOID_TYPE) {
        return "void";
    }
    if (id == INT_TYPE) {
        return "int";
    }
    if (id == STRING_TYPE) {
        return "string";
    }
    if (id == BOOLEAN_TYPE) {
        return "bool";
    }
    if (id == FLOAT_TYPE) {
        return "float";
    }
    return NULL;
}

VariableDeclaration* get_variable_from_id(int id, Validator_Object* validator_object) {
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        if (var_decl->id == id) {
            return var_decl;
        }
    }
    return NULL;
}
StructDeclaration* get_struct_from_id(int id, Validator_Object* validator_object) {
    for (int i = 0; i < validator_object->structs->size; i++) {
        StructDeclaration* struct_decl = (StructDeclaration*)validator_object->structs->objects[i];
        if (struct_decl->id == id) {
            return struct_decl;
        }
    }
    return NULL;
}
FunctionDeclaration* get_function_from_id(int id, Validator_Object* validator_object) {
    for (int i = 0; i < validator_object->functions->size; i++) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
        if (func_decl->id == id) {
            return func_decl;
        }
    }
    return NULL;
}

int validate_node(Node* node, Validator_Object* validator_object) {
    if (node == NULL) {
        return VOID_TYPE;
    }
    if (node->type == NODE_VARIABLE_DECLARATION) {
        VariableDeclaration* var_decl = (VariableDeclaration*)node->data;
        int type_id = get_id_from_name(var_decl->type, validator_object);
        if (type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(type_id)) {
            fprintf(stderr, "Invalid type: `%s` in line %d\n", var_decl->type->value, var_decl->type->line);
            exit(1);
        }
        int value_type_id = validate_node(&var_decl->value, validator_object);
        if (value_type_id != type_id && value_type_id != VOID_TYPE) { // because uninitialized variables are void
            const char* type_name = get_name_from_id(value_type_id, validator_object);
            if (type_name == NULL) {
                fprintf(stderr, "Expected type: `%s` (id: %d), got: `%d` in line %d\n", var_decl->type->value, type_id, value_type_id, var_decl->type->line);
            } else {
                fprintf(stderr, "Expected type: `%s`, got: `%s` in line %d\n", var_decl->type->value, type_name, var_decl->type->line);
            }
            exit(1);
        }
        var_decl->id = validator_object->last_id++;
        add_int(validator_object->active_variables, var_decl->id);
        add_object(validator_object->variables, node->data, sizeof(VariableDeclaration));
        return VOID_TYPE;
    }
    else if (node->type == NODE_CONSTANT) {
        Token* constant = (Token*)node->data;
        
        if (constant->type == TOKEN_CONSTANT_FLOAT) {
            return FLOAT_TYPE;
        } else if (constant->type == TOKEN_CONSTANT_INTEGER) {
            return INT_TYPE;
        } else if (constant->type == TOKEN_CONSTANT_STRING) {
            return STRING_TYPE;
        } else if (constant->type == TOKEN_CONSTANT_BOOLEAN) {
            return BOOLEAN_TYPE;
        }
    }
    else if (node->type == NODE_SCOPED_IDENTIFIER) {
        ScopedIdentifier* scoped_ident = (ScopedIdentifier*)node->data;
        int first_name_id = get_id_from_name(scoped_ident->scope[0], validator_object);
        if (first_name_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(first_name_id)) {
            fprintf(stderr, "identifier doesn't exist: `%s` in line %d\n", scoped_ident->scope[0]->value, scoped_ident->scope[0]->line);
            exit(1);
        }
        VariableDeclaration* var = get_variable_from_id(first_name_id, validator_object);
        if (var == NULL) {
            fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", scoped_ident->scope[0]->value, scoped_ident->scope[0]->line);
            exit(1);
        }
        int var_type_id = get_id_from_name(var->type, validator_object);
        if (var_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_type_id)) {
            fprintf(stderr, "variable type doesn't exist: `%s` in line %d\n", scoped_ident->scope[0]->value, scoped_ident->scope[0]->line);
            exit(1);
        }

        if (contains_int(validator_object->active_variables, var->id) == 0) {
            fprintf(stderr, "variable is not initialized: `%s` in line %d\n", scoped_ident->scope[0]->value, scoped_ident->scope[0]->line);
            exit(1);
        }
        
        int last_type_id = var_type_id;
        
        for (int i = 1; i < scoped_ident->size; i++) {
            Token* field = scoped_ident->scope[i];
            StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
            if (struct_decl == NULL) {
                fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", field->value, field->line);
                exit(1);
            }
            Parameter* found_field = NULL;
            for (int j = 0; j < struct_decl->field_count; j++) {
                if (strcmp(struct_decl->fields[j].name->value, field->value) == 0) {
                    found_field = &struct_decl->fields[j];
                    break;
                }
            }
            if (found_field == NULL) {
                fprintf(stderr, "field doesn't exist: `%s` in line %d\n", field->value, field->line);
                exit(1);
            }
            last_type_id = get_id_from_name(found_field->type, validator_object);
        }

        return last_type_id;
    }
    else if (node->type == NODE_VARIABLE_ASSIGNMENT) {
        VariableAssignment* var_assign = (VariableAssignment*)node->data;
        ScopedIdentifier scope = var_assign->identifier;
        Node temp;
        temp.data = (void*)&scope;
        temp.type = NODE_SCOPED_IDENTIFIER;
        int lhs_type_id = validate_node(&temp, validator_object);
        int rhs_type_id = validate_node(&var_assign->value, validator_object);
        if (lhs_type_id != rhs_type_id) {
            const char* lhs_type_name = get_name_from_id(lhs_type_id, validator_object);
            const char* rhs_type_name = get_name_from_id(rhs_type_id, validator_object);
            if (lhs_type_name == NULL && rhs_type_name == NULL) {
                fprintf(stderr, "type mismatch: lhs id:`%d`, rhs id:`%d` in line %d\n", lhs_type_id, rhs_type_id, var_assign->identifier.scope[0]->line);
            } else if (lhs_type_name == NULL) {
                fprintf(stderr, "type mismatch: lhs id:`%d`, rhs name:`%s` in line %d\n", lhs_type_id, rhs_type_name, var_assign->identifier.scope[0]->line);
            } else if (rhs_type_name == NULL) {
                fprintf(stderr, "type mismatch: lhs name:`%s`, rhs id:`%d` in line %d\n", lhs_type_name, rhs_type_id, var_assign->identifier.scope[0]->line);
            } else {
                fprintf(stderr, "type mismatch: lhs name:`%s`, rhs name:`%s` in line %d\n", lhs_type_name, rhs_type_name, var_assign->identifier.scope[0]->line);
            }
            exit(1);
        }
        return VOID_TYPE;
    }
    else if (node->type == NODE_EMPTY) {
        return VOID_TYPE;
    }
    else {
        printf("ERROR: TODO Node type: %s\n", get_node_type_name(node->type));
        exit(1);
    }
    return VOID_TYPE;
}

Validator_Object create_validator_object() {
    Validator_Object validator_object;
    validator_object.structs = create_object_list();
    validator_object.functions = create_object_list();
    validator_object.variables = create_object_list();
    validator_object.active_variables = create_int_list();
    validator_object.last_id = 1;
    return validator_object;
}
void free_validator_object(Validator_Object* validator_object) {
    free_object_list(validator_object->structs);
    free_object_list(validator_object->functions);
    free_object_list(validator_object->variables);
    free_int_list(validator_object->active_variables);
}

void debug_validator(Validator_Object* validator_object) {
    printf("Debugging Validator Table:\n");
    printf("Structs:\n");
    for (int i = 0; i < validator_object->structs->size; i++) {
        StructDeclaration* struct_decl = (StructDeclaration*)validator_object->structs->objects[i];
        printf("NAME: `%s` ID: `%d` LINE: `%d`\n", struct_decl->name->value, struct_decl->id, struct_decl->name->line);
    }
    printf("Functions:\n");
    for (int i = 0; i < validator_object->functions->size; i++) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
        printf("NAME: `%s` ID: `%d` LINE: `%d`\n", func_decl->name->value, func_decl->id, func_decl->name->line);
    }
    printf("Variables:\n");
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        printf("NAME: `%s` ID: `%d` LINE: `%d`\n", var_decl->name->value, var_decl->id, var_decl->name->line);
    }
    printf("Active Variables:\n");
    for (int i = 0; i < validator_object->active_variables->size; i++) {
        int* var_id = get_int(validator_object->active_variables, i);
        printf("ID: `%d`\n", *var_id);
    }
    printf("End of Validator Table\n");
}

#endif // Validator_H