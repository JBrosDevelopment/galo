#include "galo_headers.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    TokenList* token_list = create_token_list(); 
    NodeList* ast = create_node_list();
    ObjectList* object_list = create_object_list();
    Validator_Object validator_object = create_validator_object();
    FileList* source_code_files = create_file_list();
    FileList* source_code_file_names = create_file_list();
    char* file_build_options = NULL;
    
    printf("preprocessing...\n");
    preprocess("source.galo", source_code_file_names, source_code_files, file_build_options);
    
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
    free_file_list(source_code_files);

    return 0;
}