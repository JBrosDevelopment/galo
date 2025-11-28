#include <stdio.h>
#include "galo_headers.h"

int main() {
    const char* source_code = "let x int = 10\0";
    TokenList* tokens = create_token_list(); 

    lexer(source_code, tokens);
    parser();
    validator();
    run();

    free_token_list(tokens);
    return 0;
}