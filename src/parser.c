#ifndef Parser_H
#define Parser_H

#include "galo_headers.h"
#include <stdio.h>

void parser(TokenList* tokens, NodeList* ast) {
    printf("Parsing...\n");

    Node root_node;
    root_node.type = NODE_BODY;
    Body root_body;
    root_body.statements = NULL;
    root_body.statement_count = 69; // Placeholder value
    root_node.data = &root_body;
    add_node(ast, root_node);

    printf("%d\n", ((Body*)(ast->nodes[0].data))->statement_count); // prints 69
}

#endif // Parser_H