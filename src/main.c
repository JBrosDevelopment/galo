#include <stdio.h>

int main() {
    // for testing:
#if LanguageType == Interpreter
    printf("Interpreter\n");
#endif
#if LanguageType == Compiler
    printf("Compiler\n");
#endif
#if LanguageType == Transpiler
    printf("Transpiler\n");
#endif
    return 0;
}