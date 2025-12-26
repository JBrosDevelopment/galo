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
#define BYTE_TYPE -6
#define LIST_TYPE -7
#define MAP_TYPE -8
#define TYPE_AS_TYPE -9 /*used in `list init(type)`*/
#define ANY_TYPE -10 /*used in `list get(list, index)` as return type*/

int validate_node(Node* node, Validator_Object* validator_object); // Forward declaration
bool function_is_predefined(Validator_Object* validator_object, ScopedIdentifier* path, PredefinedFunction** predefined_function); // Forward declaration

void validator(NodeList* ast, Validator_Object* validator_object) {
    for (int i = 0; i < ast->size; i++) {
        Node* node = get_node(ast, i);
        validate_node(node, validator_object);
    }
}

int get_id_from_name(Token* name, Validator_Object* validator_object) {
    if (name->type == TOKEN_IDENTIFIER) {
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
        if (strcmp(name->value, "byte") == 0) {
            return BYTE_TYPE;
        }
        if (strcmp(name->value, "list") == 0) {
            return LIST_TYPE;
        }
    }
    int found_non_initialized_ids = 0;
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        if (strcmp(name->value, var_decl->name->value) == 0) {
            if (contains_int(validator_object->active_variables, var_decl->id)) {
                return var_decl->id;
            } else {
                found_non_initialized_ids++;
            }
        }
    }
    if (found_non_initialized_ids > 1) {
        fprintf(stderr, "variable is not initialized: `%s` in line %d\n", name->value, name->line);
        exit(1);
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
    if (id == BYTE_TYPE) {
        return "byte";
    }
    if (id == LIST_TYPE) {
        return "list";
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

void check_type_missmatch(int lhs_type_id, int rhs_type_id, int line, Validator_Object* validator_object) {
    if (lhs_type_id == ANY_TYPE || rhs_type_id == ANY_TYPE) {
        return;
    }
    if (lhs_type_id == FLOAT_TYPE && (rhs_type_id == INT_TYPE || rhs_type_id == BYTE_TYPE)) {
        return;
    } else if (lhs_type_id == INT_TYPE && (rhs_type_id == FLOAT_TYPE || rhs_type_id == BYTE_TYPE)) {
        return;
    } else if (lhs_type_id == BYTE_TYPE && rhs_type_id == INT_TYPE) {
        return;
    }
    if (lhs_type_id != rhs_type_id) {
        const char* lhs_type_name = get_name_from_id(lhs_type_id, validator_object);
        const char* rhs_type_name = get_name_from_id(rhs_type_id, validator_object);
        if (lhs_type_name == NULL && rhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs id:`%d`, rhs id:`%d` in line %d\n", lhs_type_id, rhs_type_id, line);
        } else if (lhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs id:`%d`, rhs name:`%s` in line %d\n", lhs_type_id, rhs_type_name, line);
        } else if (rhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs name:`%s`, rhs id:`%d` in line %d\n", lhs_type_name, rhs_type_id, line);
        } else {
            fprintf(stderr, "type mismatch: lhs name:`%s`, rhs name:`%s` in line %d\n", lhs_type_name, rhs_type_name, line);
        }
        exit(1);
    }
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
        if (value_type_id != ANY_TYPE && !(value_type_id == type_id || value_type_id == VOID_TYPE || (type_id == FLOAT_TYPE && (value_type_id == INT_TYPE || value_type_id == BYTE_TYPE)) || (type_id == INT_TYPE && value_type_id == BYTE_TYPE) || (type_id == BYTE_TYPE && value_type_id == INT_TYPE))) {
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

        check_type_missmatch(lhs_type_id, rhs_type_id, var_assign->identifier.scope[0]->line, validator_object);
        return VOID_TYPE;
    }
    else if (node->type == NODE_OPERATION) {
        Operation* op = (Operation*)node->data;
        if (op->is_not_operator) {
            int type_id = validate_node(op->left, validator_object);
            if (type_id != BOOLEAN_TYPE) {
                fprintf(stderr, "type mismatch, expected: `boolean`, got: `%s` in `not` operation in line %d\n", get_name_from_id(type_id, validator_object), op->operator->line);
                exit(1);
            }
            return BOOLEAN_TYPE;
        }
        int lhs_type_id = validate_node(op->left, validator_object);
        int rhs_type_id = validate_node(op->right, validator_object);
        check_type_missmatch(lhs_type_id, rhs_type_id, op->operator->line, validator_object);
        if (lhs_type_id == BOOLEAN_TYPE && rhs_type_id == BOOLEAN_TYPE && op->operator->type == TOKEN_OPERATOR_COMPARISON && (strcmp(op->operator->value, "==") || strcmp(op->operator->value, "!="))) {
            return BOOLEAN_TYPE;
        }
        switch (op->operator->type)
        {
        case TOKEN_OPERATOR_ARITHMETIC:
            if ((lhs_type_id == FLOAT_TYPE && (rhs_type_id == INT_TYPE || rhs_type_id == BYTE_TYPE)) || ((lhs_type_id == INT_TYPE || lhs_type_id == BYTE_TYPE) && rhs_type_id == FLOAT_TYPE) || (lhs_type_id == FLOAT_TYPE && rhs_type_id == FLOAT_TYPE)) {
                return FLOAT_TYPE;
            } else if ((lhs_type_id == INT_TYPE && rhs_type_id == BYTE_TYPE) || (lhs_type_id == BYTE_TYPE && rhs_type_id == INT_TYPE) || (lhs_type_id == INT_TYPE && rhs_type_id == INT_TYPE)) {
                return INT_TYPE;
            } else if (lhs_type_id == BYTE_TYPE && rhs_type_id == BYTE_TYPE) {
                return BYTE_TYPE;
            } else {
                printf("Arithmetic Operator expects `int`, `byte`, or `float`, but got: %s and %s, in line %d\n", get_name_from_id(lhs_type_id, validator_object), get_name_from_id(rhs_type_id, validator_object), op->operator->line);
                exit(1);
            }
            break;
        case TOKEN_OPERATOR_COMPARISON:
            if ((lhs_type_id == FLOAT_TYPE || lhs_type_id == INT_TYPE || lhs_type_id == BYTE_TYPE) && (rhs_type_id == FLOAT_TYPE || rhs_type_id == INT_TYPE || rhs_type_id == BYTE_TYPE)) {
                return BOOLEAN_TYPE;
            } else {
                printf("Comparison Operator expects `int`, `byte`, or `float`, but got: %s and %s, in line %d\n", get_name_from_id(lhs_type_id, validator_object), get_name_from_id(rhs_type_id, validator_object), op->operator->line);
                exit(1);
            }
            break;
        case TOKEN_OPERATOR_LOGICAL:
            if (lhs_type_id == BOOLEAN_TYPE && rhs_type_id == BOOLEAN_TYPE) {
                return BOOLEAN_TYPE;
            } else {
                printf("Logical Operator expects `bool` and `bool`, but got: %s and %s, in line %d\n", get_name_from_id(lhs_type_id, validator_object), get_name_from_id(rhs_type_id, validator_object), op->operator->line);
                exit(1);
            }
            break;
        default:
            printf("Unexpected operator type: %s\n", get_token_type_name(op->operator->type));
            exit(1);
            break;
        }
    }
    else if (node->type == NODE_FUNCTION_CALL) {
        FunctionCall* func_call = (FunctionCall*)node->data;
        ScopedIdentifier* scoped_func = (ScopedIdentifier*)node->data;
        
        int func_type_id = NO_EXPECTED_NODE;
        for (int i = 0; i < validator_object->functions->size; i++) {
            FunctionDeclaration* index_func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
            if (strcmp(scoped_func->scope[scoped_func->size - 1]->value, index_func_decl->name->value) == 0) {
                if (func_call->scope_size == 2) {
                    // static function call
                    int struct_id = get_id_from_name(func_call->scope[0], validator_object);
                    if (struct_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(struct_id)) {
                        fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", func_call->scope[0]->value, func_call->scope[0]->line);
                        exit(1);
                    }
                    StructDeclaration* struct_decl = get_struct_from_id(struct_id, validator_object);
                    if (index_func_decl->struct_implementation == NULL && strcmp(index_func_decl->struct_implementation->value, struct_decl->name->value) != 0) {
                        continue;
                    }
                    func_type_id = index_func_decl->id;
                    break;
                }
                else if (func_call->scope_size != 1) {
                    // dynamic function call
                    int var_id = get_id_from_name(func_call->scope[0], validator_object);
                    if (var_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_id)) {
                        fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", func_call->scope[0]->value, func_call->scope[0]->line);
                        exit(1);
                    }

                    VariableDeclaration* var = get_variable_from_id(var_id, validator_object);
                    if (var == NULL) {
                        fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", scoped_func->scope[0]->value, scoped_func->scope[0]->line);
                        exit(1);
                    }
                    int var_type_id = get_id_from_name(var->type, validator_object);
                    if (var_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_type_id)) {
                        fprintf(stderr, "variable type doesn't exist: `%s` in line %d\n", scoped_func->scope[0]->value, scoped_func->scope[0]->line);
                        exit(1);
                    }

                    if (contains_int(validator_object->active_variables, var->id) == 0) {
                        fprintf(stderr, "variable is not initialized: `%s` in line %d\n", scoped_func->scope[0]->value, scoped_func->scope[0]->line);
                        exit(1);
                    }
                    
                    int last_type_id = var_type_id;
                    
                    for (int i = 1; i < scoped_func->size; i++) {
                        if (i == scoped_func->size - 1) {
                            break;
                        }
                        Token* field = scoped_func->scope[i];
                        StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
                        if (struct_decl == NULL) {
                            fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", field->value, field->line);
                            exit(1);
                        }
                        Parameter* found_field = NULL;
                        for (int k = 0; k < struct_decl->field_count; k++) {
                            if (strcmp(struct_decl->fields[k].name->value, field->value) == 0) {
                                found_field = &struct_decl->fields[k];
                                break;
                            }
                        }
                        if (found_field == NULL) {
                            fprintf(stderr, "field doesn't exist: `%s` in line %d\n", field->value, field->line);
                            exit(1);
                        }
                        last_type_id = get_id_from_name(found_field->type, validator_object);
                    }

                    if (last_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(last_type_id)) {
                        fprintf(stderr, "field type doesn't exist: `%s` in line %d\n", scoped_func->scope[0]->value, scoped_func->scope[0]->line);
                        exit(1);
                    }
                    
                    StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
                    if (index_func_decl->struct_implementation == NULL && strcmp(index_func_decl->struct_implementation->value, struct_decl->name->value) != 0) {
                        continue;
                    }

                    func_type_id = index_func_decl->id;
                    break;
                } else {
                    func_type_id = index_func_decl->id;
                    break;
                }
            }
        }
        PredefinedFunction* predefined_function = NULL;
        bool is_predefined = function_is_predefined(validator_object, scoped_func, &predefined_function);

        if ((func_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(func_type_id)) && !is_predefined) {
            fprintf(stderr, "function doesn't exist: `%s` in line %d\n", scoped_func->scope[scoped_func->size - 1]->value, scoped_func->scope[0]->line);
            exit(1);
        }


        if (is_predefined) {
            func_type_id = predefined_function->id;
            if (!predefined_function->infinite_parameters) {
                int i;
                for (i = 0; i < func_call->argument_count; i++) {
                    if (i >= predefined_function->parameter_count) {
                        fprintf(stderr, "too many arguments in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1]->value, func_call->scope[0]->line);
                        exit(1);
                    }
                    int parameter_type_id = predefined_function->parameter_ids[i];
                    if (parameter_type_id == TYPE_AS_TYPE) {
                        int type_id = get_id_from_name((Token*)func_call->arguments[i].data, validator_object);
                        if (type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(type_id)) {
                            fprintf(stderr, "Invalid type in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1]->value, func_call->scope[0]->line);
                            exit(1);
                        }
                    }

                    Node arg = func_call->arguments[i];
                    int arg_type_id = validate_node(&arg, validator_object);
                    
                    check_type_missmatch(arg_type_id, parameter_type_id, func_call->scope[0]->line, validator_object);
                }
                if (i < predefined_function->parameter_count) {
                    fprintf(stderr, "too few arguments in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1]->value, func_call->scope[0]->line);
                    exit(1);
                }
            }

            return predefined_function->return_id;
        }
        else {
            FunctionDeclaration* func_decl = get_function_from_id(func_type_id, validator_object);
            if (func_decl == NULL) {
                fprintf(stderr, "function doesn't exist: `%s` in line %d\n", scoped_func->scope[scoped_func->size - 1]->value, scoped_func->scope[0]->line);
                exit(1);
            }
            
            if (func_call->argument_count != func_decl->parameter_count) {
                fprintf(stderr, "argument count mismatch: `%s` in line %d\n", func_call->scope[0]->value, func_call->scope[0]->line);
                exit(1);
            }
    
            for (int i = 1; i < func_call->argument_count; i++) {
                Node arg = func_call->arguments[i];
                int arg_type_id = validate_node(&arg, validator_object);
                int parameter_type_id = get_id_from_name(func_decl->parameters[i].type, validator_object);
                
                check_type_missmatch(arg_type_id, parameter_type_id, func_call->scope[0]->line, validator_object);
            }

            int return_type_id = get_id_from_name(func_decl->return_type, validator_object);
            return return_type_id;
        }
    }
    else if (node->type == NODE_FUNCTION_DECLARATION) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)node->data;
        int type_id = get_id_from_name(func_decl->return_type, validator_object);
        if (type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(type_id)) {
            fprintf(stderr, "Invalid type: `%s` in line %d\n", func_decl->return_type->value, func_decl->return_type->line);
            exit(1);
        }
        for (int i = 0; i < func_decl->parameter_count; i++) {
            int param_type_id = get_id_from_name(func_decl->parameters[i].type, validator_object);
            if (param_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(param_type_id)) {
                fprintf(stderr, "Invalid parameter type: `%s` in line %d\n", func_decl->parameters[i].type->value, func_decl->parameters[i].type->line);
                exit(1);
            }
        }
        if (func_decl->struct_implementation != NULL) {
            StructDeclaration* struct_decl = get_struct_from_id(get_id_from_name(func_decl->struct_implementation, validator_object), validator_object);
            if (struct_decl == NULL) {
                fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", func_decl->struct_implementation->value, func_decl->struct_implementation->line);
                exit(1);
            }
        }

        func_decl->id = validator_object->last_id++;
        add_object(validator_object->functions, node->data, sizeof(FunctionDeclaration));

        IntList* saved = create_int_list();
        for (int i = 0; i < validator_object->active_variables->size; i++) {
            add_int(saved, *get_int(validator_object->active_variables, i));
        }

        for (int i = 0; i < func_decl->parameter_count; i++) {
            Parameter* param = &func_decl->parameters[i];
            VariableDeclaration var;
            var.name = param->name;
            var.type = param->type;
            Node empty;
            empty.type = NODE_EMPTY;
            var.value = empty;
            var.id = validator_object->last_id++;
            add_object(validator_object->variables, &var, sizeof(VariableDeclaration));
            add_int(validator_object->active_variables, var.id);
        }

        validator(func_decl->body, validator_object);

        free_int_list(validator_object->active_variables);
        validator_object->active_variables = saved;

        return VOID_TYPE;
    }
    else if (node->type == NODE_RETURN_STATEMENT) {
        ReturnStatement* return_stmt = (ReturnStatement*)node->data;
        return validate_node(&return_stmt->value, validator_object);
    }
    else if (node->type == NODE_STRUCT_DECLARATION) {
        StructDeclaration* struct_decl = (StructDeclaration*)node->data;
        int type_id = get_id_from_name(struct_decl->name, validator_object);
        if (type_id != NO_EXPECTED_NODE) {
            fprintf(stderr, "struct already exists: `%s` in line %d\n", struct_decl->name->value, struct_decl->name->line);
            exit(1);
        }
        struct_decl->id = validator_object->last_id++;
        add_object(validator_object->structs, node->data, sizeof(StructDeclaration));
        return VOID_TYPE;
    }
    else if (node->type == NODE_EMPTY) {
        return VOID_TYPE;
    }
    else if (node->type == NODE_IF_STATEMENT) {
        IfStatement* if_stmt = (IfStatement*)node->data;
        int condition_type_id = validate_node(&if_stmt->condition, validator_object);
        if (condition_type_id != BOOLEAN_TYPE) {
            fprintf(stderr, "Expected condition type: `boolean` in line %d\n", if_stmt->line);
            exit(1);
        }

        validator(if_stmt->body, validator_object);
        for (int i = 0; i < if_stmt->elif_count; i++) {
            int elif_condition_type_id = validate_node(&if_stmt->elifs[i].condition, validator_object);
            if (elif_condition_type_id != BOOLEAN_TYPE) {
                fprintf(stderr, "Expected condition type: `boolean` in line %d\n", if_stmt->line);
                exit(1);
            }
            validator(if_stmt->elifs[i].body, validator_object);
        }
        if (if_stmt->else_body != NULL) {
            validator(if_stmt->else_body, validator_object);
        }
        return VOID_TYPE;
    }
    else if (node->type == NODE_WHILE_LOOP) {
        WhileLoop* while_loop = (WhileLoop*)node->data;
        int condition_type_id = validate_node(&while_loop->condition, validator_object);
        if (condition_type_id != BOOLEAN_TYPE) {
            fprintf(stderr, "Expected condition type: `boolean` in line %d\n", while_loop->line);
            exit(1);
        }
        validator(while_loop->body, validator_object);
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
    add_predefined_functions(&validator_object);
    return validator_object;
}
void free_validator_object(Validator_Object* validator_object) {
    free_object_list(validator_object->structs);
    free_object_list(validator_object->functions);
    free_object_list(validator_object->variables);
    free_int_list(validator_object->active_variables);

    for (int i = 0; i < validator_object->predefined_functions->size; i++) {
        PredefinedFunction* pf = (PredefinedFunction*)get_object(validator_object->predefined_functions, i);
        if (pf->parameter_count > 0 && pf->parameter_ids != NULL) {
            free(pf->parameter_ids);
        }
    }
    free_object_list(validator_object->predefined_functions);
}

void debug_validator(Validator_Object* validator_object, FILE* out) {
    fprintf(out, "Debugging Validator Table:\n");
    fprintf(out, "Structs:\n");
    for (int i = 0; i < validator_object->structs->size; i++) {
        StructDeclaration* struct_decl = (StructDeclaration*)validator_object->structs->objects[i];
        fprintf(out, "NAME: `%s` ID: `%d` LINE: `%d`\n", struct_decl->name->value, struct_decl->id, struct_decl->name->line);
    }
    fprintf(out, "Functions:\n");
    for (int i = 0; i < validator_object->functions->size; i++) {
        FunctionDeclaration* func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
        if (func_decl->struct_implementation == NULL) {
            fprintf(out, "NAME: `%s` RETURN: `%s` PARAMS: `%d` ID: `%d` LINE: `%d`\n", func_decl->name->value, func_decl->return_type->value, func_decl->parameter_count, func_decl->id, func_decl->name->line);
        } else {
            int implements_id = get_id_from_name(func_decl->struct_implementation, validator_object);
            if (implements_id == NO_EXPECTED_NODE) {
                fprintf(out, "NAME: `%s` RETURN: `%s` PARAMS: `%d` IMPLEMENTS: `%s` ID: `%d` LINE: `%d`\n", func_decl->name->value, func_decl->return_type->value, func_decl->parameter_count, func_decl->struct_implementation->value, func_decl->id, func_decl->name->line);
            } else {
                fprintf(out, "NAME: `%s` RETURN: `%s` PARAMS: `%d` IMPLEMENTS_ID: `%d` ID: `%d` LINE: `%d`\n", func_decl->name->value, func_decl->return_type->value, func_decl->parameter_count, implements_id, func_decl->id, func_decl->name->line);
            }
        }
    }
    fprintf(out, "Variables:\n");
    for (int i = 0; i < validator_object->variables->size; i++) {
        VariableDeclaration* var_decl = (VariableDeclaration*)validator_object->variables->objects[i];
        fprintf(out, "NAME: `%s` TYPE: `%s` ID: `%d` LINE: `%d`\n", var_decl->name->value, var_decl->type->value, var_decl->id, var_decl->name->line);
    }
    fprintf(out, "Active Variables:\n");
    for (int i = 0; i < validator_object->active_variables->size; i++) {
        int* var_id = get_int(validator_object->active_variables, i);
        fprintf(out, "ID: `%d`\n", *var_id);
    }
    fprintf(out, "End of Validator Table\n");
}

bool function_is_predefined(Validator_Object* validator_object, ScopedIdentifier* path, PredefinedFunction** predefined_function) {
    for (int i = 0; i < validator_object->predefined_functions->size; i++) {
        PredefinedFunction* pf = (PredefinedFunction*)get_object(validator_object->predefined_functions, i);
        if (strcmp(path->scope[path->size - 1]->value, pf->name) == 0) {
            if (path->size == 2) {
                if (pf->parent_id == -1) {
                    continue;
                }

                int struct_id = get_id_from_name(path->scope[0], validator_object);
                if (struct_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(struct_id)) {
                    fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", path->scope[0]->value, path->scope[0]->line);
                    exit(1);
                }

                if (struct_id != pf->parent_id) {
                    continue;
                }
            }

            if (path->size == 1 && pf
                ->parent_id != -1) {
                continue;
            }

            *predefined_function = pf;
            return true;
        }
    }
    return false;
}

void add_predefined_functions(Validator_Object* validator_object) {
    validator_object->predefined_functions = create_object_list();

    PredefinedFunction print_func;
    print_func.name = "print";
    print_func.id = validator_object->last_id++;
    print_func.parameter_count = 0;
    print_func.parameter_ids = NULL;
    print_func.infinite_parameters = true;
    print_func.return_id = VOID_TYPE;
    print_func.parent_id = -1;
    add_object(validator_object->predefined_functions, &print_func, sizeof(PredefinedFunction));

    PredefinedFunction exit_func;
    exit_func.name = "exit";
    exit_func.id = validator_object->last_id++;
    exit_func.parameter_count = 0;
    exit_func.parameter_ids = NULL;
    exit_func.infinite_parameters = false;
    exit_func.return_id = VOID_TYPE;
    exit_func.parent_id = -1;
    add_object(validator_object->predefined_functions, &exit_func, sizeof(PredefinedFunction));

    PredefinedFunction input_func;
    input_func.name = "input";
    input_func.id = validator_object->last_id++;
    input_func.parameter_count = 0;
    input_func.parameter_ids = NULL;
    input_func.infinite_parameters = false;
    input_func.return_id = STRING_TYPE;
    input_func.parent_id = -1;
    add_object(validator_object->predefined_functions, &input_func, sizeof(PredefinedFunction));

    PredefinedFunction clear_func;
    clear_func.name = "clear";
    clear_func.id = validator_object->last_id++;
    clear_func.parameter_count = 0;
    clear_func.parameter_ids = NULL;
    clear_func.infinite_parameters = false;
    clear_func.return_id = VOID_TYPE;
    clear_func.parent_id = -1;
    add_object(validator_object->predefined_functions, &clear_func, sizeof(PredefinedFunction));

    PredefinedFunction format_func;
    format_func.name = "format";
    format_func.id = validator_object->last_id++;
    format_func.parameter_count = 0;
    format_func.parameter_ids = NULL;
    format_func.infinite_parameters = true;
    format_func.return_id = STRING_TYPE;
    format_func.parent_id = -1;
    add_object(validator_object->predefined_functions, &format_func, sizeof(PredefinedFunction));

    PredefinedFunction string_length_func;
    string_length_func.name = "length";
    string_length_func.id = validator_object->last_id++;
    string_length_func.parameter_count = 1;
    string_length_func.parameter_ids = malloc(1 * sizeof(int));
    string_length_func.parameter_ids[0] = STRING_TYPE;
    string_length_func.infinite_parameters = false;
    string_length_func.return_id = INT_TYPE;
    string_length_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_length_func, sizeof(PredefinedFunction));

    PredefinedFunction string_index_func;
    string_index_func.name = "index";
    string_index_func.id = validator_object->last_id++;
    string_index_func.parameter_count = 2;
    string_index_func.parameter_ids = malloc(2 * sizeof(int));
    string_index_func.parameter_ids[0] = STRING_TYPE;
    string_index_func.parameter_ids[1] = INT_TYPE;
    string_index_func.infinite_parameters = false;
    string_index_func.return_id = BYTE_TYPE;
    string_index_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_index_func, sizeof(PredefinedFunction));

    PredefinedFunction string_contains_func;
    string_contains_func.name = "contains";
    string_contains_func.id = validator_object->last_id++;
    string_contains_func.parameter_count = 2;
    string_contains_func.parameter_ids = malloc(2 * sizeof(int));
    string_contains_func.parameter_ids[0] = STRING_TYPE;
    string_contains_func.parameter_ids[1] = STRING_TYPE;
    string_contains_func.infinite_parameters = false;
    string_contains_func.return_id = BOOLEAN_TYPE;
    string_contains_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_contains_func, sizeof(PredefinedFunction));

    PredefinedFunction string_startswith_func;
    string_startswith_func.name = "starts_with";
    string_startswith_func.id = validator_object->last_id++;
    string_startswith_func.parameter_count = 2;
    string_startswith_func.parameter_ids = malloc(2 * sizeof(int));
    string_startswith_func.parameter_ids[0] = STRING_TYPE;
    string_startswith_func.parameter_ids[1] = STRING_TYPE;
    string_startswith_func.infinite_parameters = false;
    string_startswith_func.return_id = BOOLEAN_TYPE;
    string_startswith_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_startswith_func, sizeof(PredefinedFunction));

    PredefinedFunction string_endswith_func;
    string_endswith_func.name = "ends_with";
    string_endswith_func.id = validator_object->last_id++;
    string_endswith_func.parameter_count = 2;
    string_endswith_func.parameter_ids = malloc(2 * sizeof(int));
    string_endswith_func.parameter_ids[0] = STRING_TYPE;
    string_endswith_func.parameter_ids[1] = STRING_TYPE;
    string_endswith_func.infinite_parameters = false;
    string_endswith_func.return_id = BOOLEAN_TYPE;
    string_endswith_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_endswith_func, sizeof(PredefinedFunction));

    PredefinedFunction string_replace_func;
    string_replace_func.name = "replace";
    string_replace_func.id = validator_object->last_id++;
    string_replace_func.parameter_count = 3;
    string_replace_func.parameter_ids = malloc(3 * sizeof(int));
    string_replace_func.parameter_ids[0] = STRING_TYPE;
    string_replace_func.parameter_ids[1] = STRING_TYPE;
    string_replace_func.parameter_ids[2] = STRING_TYPE;
    string_replace_func.infinite_parameters = false;
    string_replace_func.return_id = STRING_TYPE;
    string_replace_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_replace_func, sizeof(PredefinedFunction));

    PredefinedFunction string_sub_func;
    string_sub_func.name = "sub";
    string_sub_func.id = validator_object->last_id++;
    string_sub_func.parameter_count = 3;
    string_sub_func.parameter_ids = malloc(3 * sizeof(int));
    string_sub_func.parameter_ids[0] = STRING_TYPE;
    string_sub_func.parameter_ids[1] = INT_TYPE;
    string_sub_func.parameter_ids[2] = INT_TYPE;
    string_sub_func.infinite_parameters = false;
    string_sub_func.return_id = STRING_TYPE;
    string_sub_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_sub_func, sizeof(PredefinedFunction));

    PredefinedFunction string_concat_func;
    string_concat_func.name = "concat";
    string_concat_func.id = validator_object->last_id++;
    string_concat_func.parameter_count = 2;
    string_concat_func.parameter_ids = malloc(2 * sizeof(int));
    string_concat_func.parameter_ids[0] = STRING_TYPE;
    string_concat_func.parameter_ids[1] = STRING_TYPE;
    string_concat_func.infinite_parameters = false;
    string_concat_func.return_id = STRING_TYPE;
    string_concat_func.parent_id = STRING_TYPE;
    add_object(validator_object->predefined_functions, &string_concat_func, sizeof(PredefinedFunction));

    PredefinedFunction int_convert_func;
    int_convert_func.name = "convert";
    int_convert_func.id = validator_object->last_id++;
    int_convert_func.parameter_count = 1;
    int_convert_func.parameter_ids = malloc(1 * sizeof(int));
    int_convert_func.parameter_ids[0] = STRING_TYPE;
    int_convert_func.infinite_parameters = false;
    int_convert_func.return_id = INT_TYPE;
    int_convert_func.parent_id = INT_TYPE;
    add_object(validator_object->predefined_functions, &int_convert_func, sizeof(PredefinedFunction));

    PredefinedFunction float_convert_func;
    float_convert_func.name = "convert";
    float_convert_func.id = validator_object->last_id++;
    float_convert_func.parameter_count = 1;
    float_convert_func.parameter_ids = malloc(1 * sizeof(int));
    float_convert_func.parameter_ids[0] = STRING_TYPE;
    float_convert_func.infinite_parameters = false;
    float_convert_func.return_id = FLOAT_TYPE;
    float_convert_func.parent_id = FLOAT_TYPE;
    add_object(validator_object->predefined_functions, &float_convert_func, sizeof(PredefinedFunction));

    PredefinedFunction bool_convert_func;
    bool_convert_func.name = "convert";
    bool_convert_func.id = validator_object->last_id++;
    bool_convert_func.parameter_count = 1;
    bool_convert_func.parameter_ids = malloc(1 * sizeof(int));
    bool_convert_func.parameter_ids[0] = STRING_TYPE;
    bool_convert_func.infinite_parameters = false;
    bool_convert_func.return_id = BOOLEAN_TYPE;
    bool_convert_func.parent_id = BOOLEAN_TYPE;
    add_object(validator_object->predefined_functions, &bool_convert_func, sizeof(PredefinedFunction));

    PredefinedFunction byte_convert_func;
    byte_convert_func.name = "convert";
    byte_convert_func.id = validator_object->last_id++;
    byte_convert_func.parameter_count = 1;
    byte_convert_func.parameter_ids = malloc(1 * sizeof(int));
    byte_convert_func.parameter_ids[0] = STRING_TYPE;
    byte_convert_func.infinite_parameters = false;
    byte_convert_func.return_id = BYTE_TYPE;
    byte_convert_func.parent_id = BYTE_TYPE;
    add_object(validator_object->predefined_functions, &byte_convert_func, sizeof(PredefinedFunction));

    PredefinedFunction list_init_func;
    list_init_func.name = "init";
    list_init_func.id = validator_object->last_id++;
    list_init_func.parameter_count = 0;
    list_init_func.parameter_ids = NULL;
    list_init_func.infinite_parameters = true;
    list_init_func.return_id = LIST_TYPE;
    list_init_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_init_func, sizeof(PredefinedFunction));

    PredefinedFunction list_add_func;
    list_add_func.name = "add";
    list_add_func.id = validator_object->last_id++;
    list_add_func.parameter_count = 2;
    list_add_func.parameter_ids = malloc(2 * sizeof(int));
    list_add_func.parameter_ids[0] = LIST_TYPE;
    list_add_func.parameter_ids[1] = ANY_TYPE;
    list_add_func.infinite_parameters = false;
    list_add_func.return_id = VOID_TYPE;
    list_add_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_add_func, sizeof(PredefinedFunction));

    PredefinedFunction list_get_func;
    list_get_func.name = "get";
    list_get_func.id = validator_object->last_id++;
    list_get_func.parameter_count = 2;
    list_get_func.parameter_ids = malloc(2 * sizeof(int));
    list_get_func.parameter_ids[0] = LIST_TYPE;
    list_get_func.parameter_ids[1] = INT_TYPE;
    list_get_func.infinite_parameters = false;
    list_get_func.return_id = ANY_TYPE;
    list_get_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_get_func, sizeof(PredefinedFunction));

    PredefinedFunction list_length_func;
    list_length_func.name = "length";
    list_length_func.id = validator_object->last_id++;
    list_length_func.parameter_count = 1;
    list_length_func.parameter_ids = malloc(1 * sizeof(int));
    list_length_func.parameter_ids[0] = LIST_TYPE;
    list_length_func.infinite_parameters = false;
    list_length_func.return_id = INT_TYPE;
    list_length_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_length_func, sizeof(PredefinedFunction));

    PredefinedFunction list_remove_func;
    list_remove_func.name = "remove";
    list_remove_func.id = validator_object->last_id++;
    list_remove_func.parameter_count = 2;
    list_remove_func.parameter_ids = malloc(2 * sizeof(int));
    list_remove_func.parameter_ids[0] = LIST_TYPE;
    list_remove_func.parameter_ids[1] = INT_TYPE;
    list_remove_func.infinite_parameters = false;
    list_remove_func.return_id = VOID_TYPE;
    list_remove_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_remove_func, sizeof(PredefinedFunction));

    PredefinedFunction list_clear_func;
    list_clear_func.name = "clear";
    list_clear_func.id = validator_object->last_id++;
    list_clear_func.parameter_count = 1;
    list_clear_func.parameter_ids = malloc(1 * sizeof(int));
    list_clear_func.parameter_ids[0] = LIST_TYPE;
    list_clear_func.infinite_parameters = false;
    list_clear_func.return_id = VOID_TYPE;
    list_clear_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_clear_func, sizeof(PredefinedFunction));

    PredefinedFunction list_contains_func;
    list_contains_func.name = "contains";
    list_contains_func.id = validator_object->last_id++;
    list_contains_func.parameter_count = 2;
    list_contains_func.parameter_ids = malloc(2 * sizeof(int));
    list_contains_func.parameter_ids[0] = LIST_TYPE;
    list_contains_func.parameter_ids[1] = ANY_TYPE;
    list_contains_func.infinite_parameters = false;
    list_contains_func.return_id = BOOLEAN_TYPE;
    list_contains_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_contains_func, sizeof(PredefinedFunction));

    PredefinedFunction list_set_func;
    list_set_func.name = "set";
    list_set_func.id = validator_object->last_id++;
    list_set_func.parameter_count = 3;
    list_set_func.parameter_ids = malloc(3 * sizeof(int));
    list_set_func.parameter_ids[0] = LIST_TYPE;
    list_set_func.parameter_ids[1] = INT_TYPE;
    list_set_func.parameter_ids[2] = ANY_TYPE;
    list_set_func.infinite_parameters = false;
    list_set_func.return_id = VOID_TYPE;
    list_set_func.parent_id = LIST_TYPE;
    add_object(validator_object->predefined_functions, &list_set_func, sizeof(PredefinedFunction));

    // REMEMBER: update validator_object->predefined_function_count if adding more   
}

void emit_validator(Validator_Object* validator_object, char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        printf("Error: Could not open file %s for writing validator.\n", output_file);
        return;
    }

    debug_validator(validator_object, file);

    fclose(file);
}
