#include "galo_headers.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    BuildArguments build_arguments = process_args(argc, argv);
    
    if (build_arguments.build_option == OPTION_NEW) {
        build_option_new(build_arguments.project_name);
        return 0;
    } else if (build_arguments.build_option == OPTION_VERSION) {
        build_option_version();
        return 0;
    } else if (build_arguments.build_option == OPTION_HELP) {
        build_option_help();
        return 0;
    } else if (build_arguments.build_option == OPTION_BUILD) {
        printf("Build option is not yet implemented.\n");
        return 0;
    } else if (build_arguments.build_option == OPTION_RUN) {
        char** file_argv = NULL;
        int file_argc = 0;
        string_list_to_owned_array(build_arguments.file_build_options, &file_argv, &file_argc);

        BuildArguments file_build_arguments = process_args(file_argc, file_argv);

        printf("build options from `%s` are valid and will be run accordingly.\n", build_arguments.main_file_path);
        printf("NOTICE: Any WARNINGS about build options being ignored are fine and should be ignored.\n");
        
        free_owned_string_array(file_argv, file_argc);
        free_build_arguments(&build_arguments);
        build_arguments = file_build_arguments;
    } 
    
    StringList* source_code_files = create_string_list();
    StringList* source_code_file_names = create_string_list();

    for (int i = 0; i < build_arguments.input_files->size; i++) {
        char* file_name = get_string(build_arguments.input_files, i);
        preprocess(file_name, source_code_file_names, source_code_files);
    }

    TokenList* token_list = create_token_list(); 
    NodeList* ast = create_node_list();
    ObjectList* object_list = create_object_list();
    Validator_Object validator_object = create_validator_object();
    
    lexer_linker(source_code_file_names, source_code_files, token_list);

    if (build_arguments.emit_tokens) {
        printf("Emitting tokens to emit_tokens.txt\n");
        emit_tokens(token_list, "emit_tokens.txt");
        printf("Finished emitting tokens.\n");
    }
    
    int index = 0;
    parser(token_list, object_list, ast, &index);
    
    validator(ast, &validator_object);

    if (build_arguments.emit_ast) {
        printf("Emitting AST to emit_ast.txt\n");
        emit_ast(ast, "emit_ast.txt");
        printf("Finished emitting AST.\n");
    }

    if (build_arguments.emit_validator) {
        printf("Emitting Validator to emit_validator.txt\n");
        emit_validator(&validator_object, "emit_validator.txt");
        printf("Finished emitting Validator.\n");
    }
    
    if (build_arguments.build_option == OPTION_COMPILE) {
        printf("Compile option is not yet implemented.\n");
    } else if (build_arguments.build_option == OPTION_TRANSPILE) {
        printf("Transpile option is not yet implemented.\n");
    } else if (build_arguments.build_option == OPTION_INTERPRET) {
        Interpreter_Object* interpreter_object = create_interpreter_object(ast, &validator_object);

        int input_argc = 0;
        char** input_argv = NULL;
        string_list_to_owned_array(build_arguments.passthrough_flags, &input_argv, &input_argc);
        
        interpret(interpreter_object, input_argc, input_argv);
        
        free_owned_string_array(input_argv, input_argc);
        free_interpreter_object(interpreter_object);
    } else {
        printf("Error: Unknown build option.\n"); // Should be unreachable
        return 1;
    }

    free_validator_object(&validator_object);
    free_node_list(ast);
    free_object_list(object_list);
    free_token_list(token_list);
    free_string_owned_list(source_code_files);
    free_string_list(source_code_file_names);

    free_build_arguments(&build_arguments);

    return 0;
}