#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define MEMORY_ADDRESS_PADDING_LOWER -64000
#define MEMORY_ADDRESS_PADDING_UPPER 64000

#define TYPE_NOT_IN_BOUNDS(type) type < MEMORY_ADDRESS_PADDING_LOWER || type > MEMORY_ADDRESS_PADDING_UPPER

int validate_node(Node* node, Validator_Object* validator_object); // Forward declaration
bool function_is_predefined(Validator_Object* validator_object, Identifier* path, int path_count, PredefinedFunction** predefined_function); // Forward declaration

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
    } else if (lhs_type_id == BYTE_TYPE && (rhs_type_id == INT_TYPE || rhs_type_id == FLOAT_TYPE)) {
        return;
    }
    if (lhs_type_id != rhs_type_id) {
        const char* lhs_type_name = get_name_from_id(lhs_type_id, validator_object);
        const char* rhs_type_name = get_name_from_id(rhs_type_id, validator_object);
        if (lhs_type_name == NULL && rhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs id:`%d`, rhs id:`%d` in line %d\n", lhs_type_id, rhs_type_id, line);
        } else if (lhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs id:`%d`, rhs type:`%s` in line %d\n", lhs_type_id, rhs_type_name, line);
        } else if (rhs_type_name == NULL) {
            fprintf(stderr, "type mismatch: lhs type:`%s`, rhs id:`%d` in line %d\n", lhs_type_name, rhs_type_id, line);
        } else {
            fprintf(stderr, "type mismatch: lhs type:`%s`, rhs type:`%s` in line %d\n", lhs_type_name, rhs_type_name, line);
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
        var_decl->id = validator_object->last_variable_id++;
        var_decl->type_id = type_id;
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

        if (scoped_ident->size == 1) {
            for (int i = 0; i < validator_object->consts->size; i++) {
                ConstDeclaration* const_decl =
                    (ConstDeclaration*)get_object(validator_object->consts, i);
        
                if (strcmp(scoped_ident->scope[0].name->value,
                           const_decl->token->value) == 0) {
        
                    if (const_decl->replacement.type != NODE_CONSTANT) {
                        printf(
                            "Error: const must be a constant, got %s at line %d\n",
                            get_node_type_name(const_decl->replacement.type),
                            scoped_ident->scope[0].name->line
                        );
                        exit(1);
                    }
        
                    Token* src = (Token*)const_decl->replacement.data;
        
                    Token* tok = malloc(sizeof(Token));
                    tok->type = src->type;
                    tok->line = src->line;
                    tok->value = strdup(src->value);  // 🔥 REQUIRED
        
                    Node* new_node = malloc(sizeof(Node));
                    new_node->type = NODE_CONSTANT;
                    new_node->data = tok;

                    scoped_ident->const_replacement = new_node;
        
                    return validate_node(new_node, validator_object);
                }
            }
        }

        int first_name_id = get_id_from_name(scoped_ident->scope[0].name, validator_object);
        scoped_ident->scope[0].id = first_name_id;

        if (first_name_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(first_name_id)) {
            fprintf(stderr, "identifier doesn't exist: `%s` in line %d\n", scoped_ident->scope[0].name->value, scoped_ident->scope[0].name->line);
            exit(1);
        }
        VariableDeclaration* var = get_variable_from_id(first_name_id, validator_object);
        if (var == NULL) {
            fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", scoped_ident->scope[0].name->value, scoped_ident->scope[0].name->line);
            exit(1);
        }
        int var_type_id = get_id_from_name(var->type, validator_object);
        if (var_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_type_id)) {
            fprintf(stderr, "variable type doesn't exist: `%s` in line %d\n", scoped_ident->scope[0].name->value, scoped_ident->scope[0].name->line);
            exit(1);
        }

        if (contains_int(validator_object->active_variables, var->id) == 0) {
            fprintf(stderr, "variable is not initialized: `%s` in line %d\n", scoped_ident->scope[0].name->value, scoped_ident->scope[0].name->line);
            exit(1);
        }
        
        int last_type_id = var_type_id;
        
        for (int i = 1; i < scoped_ident->size; i++) {
            Token* field = scoped_ident->scope[i].name;
            StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
            if (struct_decl == NULL) {
                fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", field->value, field->line);
                exit(1);
            }
            Parameter* found_field = NULL;
            for (int j = 0; j < struct_decl->field_count; j++) {
                if (strcmp(struct_decl->fields[j].name.name->value, field->value) == 0) {
                    found_field = &struct_decl->fields[j];
                    break;
                }
            }
            if (found_field == NULL) {
                fprintf(stderr, "field doesn't exist: `%s` in line %d\n", field->value, field->line);
                exit(1);
            }
            last_type_id = get_id_from_name(found_field->type.name, validator_object);
            scoped_ident->scope[i].id = last_type_id;
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

        check_type_missmatch(lhs_type_id, rhs_type_id, var_assign->identifier.scope[0].name->line, validator_object);
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
            if ((lhs_type_id == FLOAT_TYPE || rhs_type_id == FLOAT_TYPE) && strcmp(op->operator->value, "%") == 0) {
                printf("Arithmetic modulo operator expects `int` or `byte`, but got: %s and %s, in line %d\n", get_name_from_id(lhs_type_id, validator_object), get_name_from_id(rhs_type_id, validator_object), op->operator->line);
                exit(1);
            }
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
        Identifier* scoped_func = (Identifier*)func_call->scope;
        int scoped_func_size = func_call->scope_size;
        
        int func_id = NO_EXPECTED_NODE;
        for (int i = 0; i < validator_object->functions->size; i++) {
            FunctionDeclaration* index_func_decl = (FunctionDeclaration*)validator_object->functions->objects[i];
            if (strcmp(scoped_func[scoped_func_size - 1].name->value, index_func_decl->name->value) == 0) {
                if (func_call->scope_size == 2) {
                    // static function call
                    int struct_id = get_id_from_name(func_call->scope[0].name, validator_object);
                    func_call->scope[0].id = struct_id;
                    if (struct_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(struct_id)) {
                        fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", func_call->scope[0].name->value, func_call->scope[0].name->line);
                        exit(1);
                    }
                    StructDeclaration* struct_decl = get_struct_from_id(struct_id, validator_object);
                    if (index_func_decl->struct_implementation == NULL && strcmp(index_func_decl->struct_implementation->value, struct_decl->name->value) != 0) {
                        continue;
                    }
                    func_id = index_func_decl->id;
                    scoped_func[scoped_func_size - 1].id = index_func_decl->id;
                    break;
                }
                else if (func_call->scope_size != 1) {
                    // dynamic function call
                    int var_id = get_id_from_name(func_call->scope[0].name, validator_object);
                    scoped_func[0].id = var_id;

                    if (var_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_id)) {
                        fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", func_call->scope[0].name->value, func_call->scope[0].name->line);
                        exit(1);
                    }

                    VariableDeclaration* var = get_variable_from_id(var_id, validator_object);
                    if (var == NULL) {
                        fprintf(stderr, "variable doesn't exist: `%s` in line %d\n", scoped_func[0].name->value, scoped_func[0].name->line);
                        exit(1);
                    }
                    int var_type_id = get_id_from_name(var->type, validator_object);
                    if (var_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(var_type_id)) {
                        fprintf(stderr, "variable type doesn't exist: `%s` in line %d\n", scoped_func[0].name->value, scoped_func[0].name->line);
                        exit(1);
                    }

                    if (contains_int(validator_object->active_variables, var->id) == 0) {
                        fprintf(stderr, "variable is not initialized: `%s` in line %d\n", scoped_func[0].name->value, scoped_func[0].name->line);
                        exit(1);
                    }
                    
                    int last_type_id = var_type_id;
                    
                    for (int i = 1; i < scoped_func_size; i++) {
                        if (i == scoped_func_size - 1) {
                            break;
                        }
                        Token* field = scoped_func[i].name;
                        StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
                        if (struct_decl == NULL) {
                            fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", field->value, field->line);
                            exit(1);
                        }
                        Parameter* found_field = NULL;
                        for (int k = 0; k < struct_decl->field_count; k++) {
                            if (strcmp(struct_decl->fields[k].name.name->value, field->value) == 0) {
                                found_field = &struct_decl->fields[k];
                                break;
                            }
                        }
                        if (found_field == NULL) {
                            fprintf(stderr, "field doesn't exist: `%s` in line %d\n", field->value, field->line);
                            exit(1);
                        }
                        last_type_id = found_field->type.id;
                        scoped_func[i].id = last_type_id;
                    }

                    if (last_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(last_type_id)) {
                        fprintf(stderr, "field type doesn't exist: `%s` in line %d\n", scoped_func[0].name->value, scoped_func[0].name->line);
                        exit(1);
                    }
                    
                    StructDeclaration* struct_decl = get_struct_from_id(last_type_id, validator_object);
                    if (index_func_decl->struct_implementation == NULL && strcmp(index_func_decl->struct_implementation->value, struct_decl->name->value) != 0) {
                        continue;
                    }

                    func_id = index_func_decl->id;
                    break;
                } else {
                    func_call->scope[0].id = get_id_from_name(func_call->scope[0].name, validator_object);
                    func_id = index_func_decl->id;
                    break;
                }
            }
        }
        PredefinedFunction* predefined_function = NULL;
        bool is_predefined = function_is_predefined(validator_object, scoped_func, scoped_func_size, &predefined_function);

        if ((func_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(func_id)) && !is_predefined) {
            fprintf(stderr, "function doesn't exist: `%s` in line %d\n", scoped_func[scoped_func_size - 1].name->value, scoped_func[0].name->line);
            exit(1);
        }

        if (is_predefined) {
            func_id = predefined_function->id;
            func_call->id = func_id;
            for (int i = 0; i < scoped_func_size - 1; i++) {
                func_call->scope[i].id = get_id_from_name(func_call->scope[i].name, validator_object);
            }
            func_call->scope[func_call->scope_size - 1].id = func_id;
            if (!predefined_function->infinite_parameters) {
                int i;
                for (i = 0; i < func_call->argument_count; i++) {
                    if (i >= predefined_function->parameter_count) {
                        fprintf(stderr, "too many arguments in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                        exit(1);
                    }
                    int parameter_type_id = predefined_function->parameter_ids[i];
                    if (parameter_type_id == TYPE_AS_TYPE) {
                        if (func_call->arguments[i].type != NODE_SCOPED_IDENTIFIER) {
                            fprintf(stderr, "Invalid type in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                            exit(1);
                        }
                        ScopedIdentifier* arg_data = (ScopedIdentifier*)func_call->arguments[i].data;
                        if (arg_data->size != 1) {
                            fprintf(stderr, "Invalid type in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                            exit(1);
                        }
                        Token* type_token = arg_data->scope[0].name;
                        int type_id = get_id_from_name(type_token, validator_object);
                        if (type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(type_id)) {
                            fprintf(stderr, "Invalid type in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                            exit(1);
                        }
                        arg_data->scope[0].id = TYPE_AS_TYPE;
                        continue;
                    }

                    Node arg = func_call->arguments[i];
                    int arg_type_id = validate_node(&arg, validator_object);
                    
                    check_type_missmatch(arg_type_id, parameter_type_id, func_call->scope[0].name->line, validator_object);
                }
                if (i < predefined_function->parameter_count) {
                    fprintf(stderr, "too few arguments in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                    exit(1);
                }
            } else {
                for (int i = 0; i < func_call->argument_count; i++) {
                    Node arg = func_call->arguments[i];
                    int arg_type_id = validate_node(&arg, validator_object);
                    if (arg_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(arg_type_id)) {
                        fprintf(stderr, "Invalid type in function call `%s` in line %d\n", func_call->scope[func_call->scope_size - 1].name->value, func_call->scope[0].name->line);
                        exit(1);
                    }
                }
            }

            return predefined_function->return_id;
        }
        else {
            func_call->id = func_id;
            FunctionDeclaration* func_decl = get_function_from_id(func_id, validator_object);
            if (func_decl == NULL) {
                fprintf(stderr, "function doesn't exist: `%s` in line %d\n", scoped_func[scoped_func_size - 1].name->value, scoped_func[0].name->line);
                exit(1);
            }
            
            if (func_call->argument_count != func_decl->parameter_count) {
                fprintf(stderr, "argument count mismatch: `%s` in line %d\n", func_call->scope[0].name->value, func_call->scope[0].name->line);
                exit(1);
            }
    
            for (int i = 0; i < func_call->argument_count; i++) {
                Node arg = func_call->arguments[i];
                int arg_type_id = validate_node(&arg, validator_object);
                int parameter_type_id = func_decl->parameters[i].type.id;
                
                check_type_missmatch(arg_type_id, parameter_type_id, func_call->scope[0].name->line, validator_object);
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
            int param_type_id = get_id_from_name(func_decl->parameters[i].type.name, validator_object);
            func_decl->parameters[i].type.id = param_type_id;
            if (param_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(param_type_id)) {
                fprintf(stderr, "Invalid parameter type: `%s` in line %d\n", func_decl->parameters[i].type.name->value, func_decl->parameters[i].type.name->line);
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

        func_decl->id = validator_object->last_function_id++;
        add_object(validator_object->functions, node->data, sizeof(FunctionDeclaration));

        IntList* saved = create_int_list();
        for (int i = 0; i < validator_object->active_variables->size; i++) {
            add_int(saved, *get_int(validator_object->active_variables, i));
        }

        for (int i = 0; i < func_decl->parameter_count; i++) {
            Parameter* param = &func_decl->parameters[i];
            VariableDeclaration var;
            var.name = param->name.name;
            var.type = param->type.name;
            Node empty;
            empty.type = NODE_EMPTY;
            var.value = empty;
            var.id = validator_object->last_variable_id++;
            var.type_id = param->type.id;
            param->name.id = var.id;
            param->type.id = var.type_id;
            add_object(validator_object->variables, &var, sizeof(VariableDeclaration));
            add_int(validator_object->active_variables, var.id);
        }

        bool is_inside_function = validator_object->is_inside_function;
        validator_object->is_inside_function = true;
        validator(func_decl->body, validator_object);
        validator_object->is_inside_function = is_inside_function;

        free_int_list(validator_object->active_variables);
        validator_object->active_variables = saved;

        return VOID_TYPE;
    }
    else if (node->type == NODE_RETURN_STATEMENT) {
        ReturnStatement* return_stmt = (ReturnStatement*)node->data;
        if (!validator_object->is_inside_function) {
            printf("ERROR: return statement outside of function in line %d\n", return_stmt->line);
            exit(1);
        }
        if (return_stmt->value.type == NODE_EMPTY) {
            return VOID_TYPE;
        }
        return validate_node(&return_stmt->value, validator_object);
    }
    else if (node->type == NODE_BREAK_STATEMENT) {
        if (!validator_object->is_inside_while_loop) {
            Token* token = (Token*)node->data;
            printf("ERROR: break statement outside of loop in line %d\n", token->line);
            exit(1);
        }
        return VOID_TYPE;
    }
    else if (node->type == NODE_CONTINUE_STATEMENT) {
        if (!validator_object->is_inside_while_loop) {
            Token* token = (Token*)node->data;
            printf("ERROR: break statement outside of loop in line %d\n", token->line);
            exit(1);
        }
        return VOID_TYPE;
    }
    else if (node->type == NODE_STRUCT_DECLARATION) {
        StructDeclaration* struct_decl = (StructDeclaration*)node->data;
        int type_id = get_id_from_name(struct_decl->name, validator_object);
        if (type_id != NO_EXPECTED_NODE) {
            fprintf(stderr, "struct already exists: `%s` in line %d\n", struct_decl->name->value, struct_decl->name->line);
            exit(1);
        }
        struct_decl->id = validator_object->last_struct_id++;

        for (int i = 0; i < struct_decl->field_count; i++) {
            int param_type_id = get_id_from_name(struct_decl->fields[i].type.name, validator_object);
            struct_decl->fields[i].type.id = param_type_id;
            if (param_type_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(param_type_id)) {
                fprintf(stderr, "Invalid parameter type: `%s` in line %d\n", struct_decl->fields[i].type.name->value, struct_decl->fields[i].type.name->line);
                exit(1);
            }
        }

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
        bool last_is_while_loop = validator_object->is_inside_while_loop;
        validator_object->is_inside_while_loop = true;
        validator(while_loop->body, validator_object);
        validator_object->is_inside_while_loop = last_is_while_loop;
        return VOID_TYPE;
    }
    else if (node->type == NODE_CONST_DECLARATION) {
        add_object(validator_object->consts, node->data, sizeof(ConstDeclaration));
    }
    else {
        printf("ERROR: TODO Node type: %s\n", get_node_type_name(node->type));
        exit(1);
    }
    return VOID_TYPE;
}

Validator_Object create_validator_object() {
    Validator_Object validator_object;
    validator_object.predefined_functions = create_object_list();
    validator_object.structs = create_object_list();
    validator_object.functions = create_object_list();
    validator_object.variables = create_object_list();
    validator_object.active_variables = create_int_list();
    validator_object.consts = create_object_list();
    validator_object.last_variable_id = 0;
    validator_object.last_struct_id = 0;
    validator_object.last_function_id = 0;
    validator_object.is_inside_function = false;
    validator_object.is_inside_while_loop = false;
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
        fprintf(out, "NAME: `%s` TYPE: `%s` TYPE_ID: `%d` ID: `%d` LINE: `%d`\n", var_decl->name->value, var_decl->type->value, var_decl->type_id, var_decl->id, var_decl->name->line);
    }
    fprintf(out, "Active Variables:\n");
    for (int i = 0; i < validator_object->active_variables->size; i++) {
        int* var_id = get_int(validator_object->active_variables, i);
        fprintf(out, "ID: `%d`\n", *var_id);
    }
    fprintf(out, "End of Validator Table\n");
}

bool function_is_predefined(Validator_Object* validator_object, Identifier* path, int path_count, PredefinedFunction** predefined_function) {
    for (int i = 0; i < validator_object->predefined_functions->size; i++) {
        PredefinedFunction* pf = (PredefinedFunction*)get_object(validator_object->predefined_functions, i);
        if (strcmp(path[path_count - 1].name->value, pf->name) == 0) {
            if (path_count == 2) {
                if (pf->parent_id == -1) {
                    continue;
                }

                int struct_id = get_id_from_name(path[0].name, validator_object);
                if (struct_id == NO_EXPECTED_NODE || TYPE_NOT_IN_BOUNDS(struct_id)) {
                    fprintf(stderr, "struct doesn't exist: `%s` in line %d\n", path[0].name->value, path[0].name->line);
                    exit(1);
                }

                if (struct_id != pf->parent_id) {
                    continue;
                }
            }

            if (path_count == 1 && pf->parent_id != -1) {
                continue;
            }

            *predefined_function = pf;
            return true;
        }
    }
    return false;
}

void add_function(Validator_Object* validator_object, char* name, int return_id, int parameter_count, int* parameter_ids, int parent_id) {
    PredefinedFunction pf;
    pf.id = validator_object->last_function_id++;
    pf.name = name;
    pf.return_id = return_id;
    pf.parameter_count = parameter_count;
    pf.parameter_ids = parameter_ids;
    pf.infinite_parameters = parameter_count == INFINTE_PARAMETERS;
    pf.parent_id = parent_id;
    add_object(validator_object->predefined_functions, &pf, sizeof(PredefinedFunction));
}

int* predefined_function_parameters(int parameter_count, ...) {
    int* args = malloc(parameter_count * sizeof(int));
    if (!args) {
        return NULL;
    }

    va_list ap;
    va_start(ap, parameter_count);

    for (int i = 0; i < parameter_count; i++) {
        args[i] = va_arg(ap, int);
    }

    va_end(ap);
    return args;
}

void add_predefined_functions(Validator_Object* validator_object) {
    add_function(validator_object, "print", VOID_TYPE, INFINTE_PARAMETERS, NULL, NO_PARENT); // id 0
    add_function(validator_object, "println", VOID_TYPE, INFINTE_PARAMETERS, NULL, NO_PARENT); // id 1
    add_function(validator_object, "exit", VOID_TYPE, 1, predefined_function_parameters(1, INT_TYPE), NO_PARENT); // id 2
    add_function(validator_object, "input", STRING_TYPE, 0, NULL, NO_PARENT); // id 3
    add_function(validator_object, "clear", VOID_TYPE, 0, NULL, NO_PARENT); // id 4
    add_function(validator_object, "cast", ANY_TYPE, 2, predefined_function_parameters(2, TYPE_AS_TYPE, ANY_TYPE), NO_PARENT); // id 5
    add_function(validator_object, "to_string", STRING_TYPE, 1, predefined_function_parameters(1, ANY_TYPE), NO_PARENT); // id 6
    add_function(validator_object, "format", STRING_TYPE, INFINTE_PARAMETERS, NULL, NO_PARENT); // id 7

    add_function(validator_object, "length", INT_TYPE, 1, predefined_function_parameters(1, STRING_TYPE), STRING_TYPE); // id 8
    add_function(validator_object, "index", BYTE_TYPE, 2, predefined_function_parameters(2, STRING_TYPE, INT_TYPE), STRING_TYPE); // id 9
    add_function(validator_object, "contains", BOOLEAN_TYPE, 2, predefined_function_parameters(2, STRING_TYPE, STRING_TYPE), STRING_TYPE); // id 10
    add_function(validator_object, "starts_with", BOOLEAN_TYPE, 2, predefined_function_parameters(2, STRING_TYPE, STRING_TYPE), STRING_TYPE); // id 11
    add_function(validator_object, "ends_with", BOOLEAN_TYPE, 2, predefined_function_parameters(2, STRING_TYPE, STRING_TYPE), STRING_TYPE); // id 12
    add_function(validator_object, "replace", STRING_TYPE, 3, predefined_function_parameters(3, STRING_TYPE, STRING_TYPE, STRING_TYPE), STRING_TYPE); // id 13
    add_function(validator_object, "sub", STRING_TYPE, 3, predefined_function_parameters(3, STRING_TYPE, INT_TYPE, INT_TYPE), STRING_TYPE); // id 14
    add_function(validator_object, "split", LIST_TYPE, 2, predefined_function_parameters(2, STRING_TYPE, STRING_TYPE), STRING_TYPE); // id 15
    add_function(validator_object, "concat", STRING_TYPE, INFINTE_PARAMETERS, NULL, STRING_TYPE); // id 16
    
    add_function(validator_object, "init", LIST_TYPE, 2, predefined_function_parameters(2, TYPE_AS_TYPE, INT_TYPE), LIST_TYPE); // id 17
    add_function(validator_object, "append", LIST_TYPE, 2, predefined_function_parameters(2, LIST_TYPE, ANY_TYPE), LIST_TYPE); // id 18
    add_function(validator_object, "remove", LIST_TYPE, 2, predefined_function_parameters(2, LIST_TYPE, INT_TYPE), LIST_TYPE); // id 19
    add_function(validator_object, "get", ANY_TYPE, 2, predefined_function_parameters(2, LIST_TYPE, INT_TYPE), LIST_TYPE); // id 20
    add_function(validator_object, "length", INT_TYPE, 1, predefined_function_parameters(1, LIST_TYPE), LIST_TYPE); // id 21
    add_function(validator_object, "contains", BOOLEAN_TYPE, 2, predefined_function_parameters(2, LIST_TYPE, ANY_TYPE), LIST_TYPE); // id 22
    add_function(validator_object, "index", INT_TYPE, 2, predefined_function_parameters(2, LIST_TYPE, ANY_TYPE), LIST_TYPE); // id 23
    add_function(validator_object, "set", LIST_TYPE, 3, predefined_function_parameters(3, LIST_TYPE, INT_TYPE, ANY_TYPE), LIST_TYPE); // id 24
    add_function(validator_object, "insert", LIST_TYPE, 3, predefined_function_parameters(3, LIST_TYPE, INT_TYPE, ANY_TYPE), LIST_TYPE); // id 25
    add_function(validator_object, "clear", LIST_TYPE, 1, predefined_function_parameters(1, LIST_TYPE), LIST_TYPE); // id 26
    add_function(validator_object, "list", LIST_TYPE, INFINTE_PARAMETERS, NULL, NO_PARENT); // id 27

    add_function(validator_object, "is_type", BOOLEAN_TYPE, 2, predefined_function_parameters(2, TYPE_AS_TYPE, ANY_TYPE), NO_PARENT); // id 28
    add_function(validator_object, "print_all_variables", VOID_TYPE, 0, NULL, NO_PARENT); // id 29 
    // can't do sizeof or malloc or any other C function because those would only be valid in the context of a compiler.
    // It could work with interpreter but wouldn't be valid with transpiling to python or javascript
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
