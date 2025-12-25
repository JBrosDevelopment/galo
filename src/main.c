#include "galo_headers.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    StringList* source_code_files = create_string_list();
    StringList* source_code_file_names = create_string_list();

    //printf("processing arguments...\n");
    BuildArguments build_arguments = process_args(argc, argv);
    debug_build_arguments(&build_arguments);
    
    if (build_arguments.build_option == OPTION_NEW) {
        build_option_new(build_arguments.project_name);
        return 0;
    }
    if (build_arguments.build_option == OPTION_VERSION) {
        build_option_version();
        return 0;
    }
    if (build_arguments.build_option == OPTION_HELP) {
        build_option_help();
        return 0;
    }
    
    printf("preprocessing...\n");
    preprocess("project/main.galo", source_code_file_names, source_code_files);

    TokenList* token_list = create_token_list(); 
    NodeList* ast = create_node_list();
    ObjectList* object_list = create_object_list();
    Validator_Object validator_object = create_validator_object();
    
    printf("lexing...\n");
    lexer_linker(source_code_file_names, source_code_files, token_list);
    //debug_lexer(token_list);
    //debug_lexer_reshape(token_list);
    
    printf("parsing...\n");
    int index = 0;
    parser(token_list, object_list, ast, &index);
    //debug_parser(ast);

    printf("validating...\n");
    validator(ast, &validator_object);
    //debug_validator(&validator_object);
    
    

    free_validator_object(&validator_object);
    free_node_list(ast);
    free_object_list(object_list);
    free_token_list(token_list);
    free_string_list(source_code_files);
    free_string_list(source_code_file_names);
    free_build_arguments(&build_arguments);

    return 0;
}