#include <stdio.h>
#include <stdlib.h>
#include "galo_headers.h"

int main() {
    const char* source_code = read_file("source.galo");
    if (!source_code) {
        fprintf(stderr, "Failed to read file\n");
        return 1;
    }
    TokenList* token_list = create_token_list(); 
    
    printf("lexing...\n");
    lexer(source_code, token_list);
    //debug_lexer(token_list);
    //debug_lexer_reshape(token_list);
    
    NodeList* ast = create_node_list();
    ObjectList* object_list = create_object_list();
    int index = 0;
    
    printf("parsing...\n");
    parser(token_list, object_list, ast, &index);
    //debug_parser(ast);

    Validator_Object validator_object = create_validator_object();
    printf("validating...\n");
    validator(ast, &validator_object);
    debug_validator(&validator_object);
    
    printf("runnning...\n");
    run();

    free_validator_object(&validator_object);
    free_node_list(ast);
    free_object_list(object_list);
    free_token_list(token_list);
    free((void*)source_code);
    return 0;
}

const char* read_file(const char* filename) {
    FILE* f = fopen(filename, "rb");

    if (!f) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        printf("Failed to seek to end of file\n");
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        printf("Failed to get file size\n");
        return NULL;
    }
    rewind(f);

    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        printf("Failed to allocate buffer\n");
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);

    if (read != (size_t)size) {
        free(buffer);
        printf("Failed to read file contents\n");
        return NULL;
    }

    printf("File '%s' read successfully, size: %ld bytes\n", filename, size);

    buffer[size] = '\0';
    return (const char*)buffer; // Caller must free: free((void*)ptr)
}