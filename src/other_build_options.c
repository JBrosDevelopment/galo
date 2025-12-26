#ifndef OTHER_BUILD_OPTIONS_C
#define OTHER_BUILD_OPTIONS_C

#include "galo_headers.h"
#include <stdio.h>
#include <stdlib.h>

void build_option_new(char* project_name) {
    printf("Creating new Galo project: %s\n", project_name);

    printf("TODO: implement project creation\n");
    exit(1);
}

void build_option_version() {
    printf("Galo Version: %s\n", GALO_VERSION);
}

void build_option_help() {
    printf("Galo Help:\n");
    char* help_message = "Galo - Simple Programming Language Toolchain \n\
\n\
USAGE:\n\
  galo [OPTION] [ARGUMENTS] [FLAGS] -- [PASS-THROUGH FLAGS]\n\
\n\
OPTIONS:\n\
  compile     [INPUT_FILE | -i ARG]     Compile Galo source to a native binary\n\
  transpile   [INPUT_FILE | -i ARG]     Transpile Galo source to another language\n\
  interpret   [INPUT_FILE | -i ARG]     Run Galo source using the interpreter\n\
  build       [DIRECTORY | none]        Build project using build.galo file.\n\
                                        If no directory is provided, uses current\n\
                                        directory and finds build.galo\n\
  run         [FILE | none]             Runs galo file with build options.\n\
                                        If no file is provided, uses `main.galo`.\n\
                                        Requires file to have build options\n\
  new         [PROJECT_NAME]            Create a new Galo project in a subdirectory\n\
  version                               Print the current Galo version\n\
  help                                  Show this help message\n\
\n\
ARGUMENTS:\n\
\n\
  -i <file>            Input file (can be used multiple times)\n\
  -o <file>            Output file\n\
  -I <dir>             Input directory\n\
  -t <language>        Target language              (required for transpile only)\n\
                                                    (alias --target-lang <language>)\n\
  --compiler <path>    Compiler path or name        (optional for compile only: default gcc)\n\
  --max-steps <count>  Max interpreter steps        (optional for interpret only)\n\
\n\
FLAGS:\n\
  --emit-tokens        Output lexer tokens\n\
  --emit-ast           Output abstract syntax tree\n\
  --emit-validator     Output validator information\n\
\n\
  --emit-c             Emit C source                 (compile only)\n\
  --emit-obj           Emit object file              (compile only)\n\
  --debug-symbols      Generate debug symbols        (compile only)\n\
\n\
  --emit-header        Emit header file              (transpile only)\n\
  --single-file        Output a single file          (transpile only)\n\
\n\
  --interpret-debug    Enable interpreter debug mode (interpret only)\n\
\n\
PASS-THROUGH FLAGS:\n\
  Everything after `--` is passed directly to the program or compiler.\n\
\n\
  * Transpiler: pass-through flags are ignored (a warning is shown)\n\
  * Interpreter: passed to the program as argv\n\
  * Compiler: passed to the underlying compiler\n\
  * Build or Run: Not supported and will error\n\
\n\
EXAMPLES:\n\
  galo compile project/main.galo -o project/bin/main.exe --compiler gcc -- -O2 -Wall\n\
      Compile main.galo using gcc with optimization and warnings enabled\n\
\n\
  galo transpile project/main.galo -o project/build/main.py -t python --single-file\n\
      Transpile main.galo to Python as a single file\n\
\n\
  galo interpret project/main.galo --interpret-debug\n\
      Run the interpreter with debug output enabled\n\
\n\
  galo build\n\
      Build using build.galo file in the current directory\n\
\n\
  galo run project/main.galo\n\
      Runs project/main.galo with build options from the file itself\n\
\n\
  galo new MyProject\n\
      Create a new project with src/main.galo and build.galo\n\
\n\
  galo version\n\
      Print the current Galo version\n\
\n\
  galo help\n\
      Display this help message";

    printf("%s\n", help_message);
}

#endif // OTHER_BUILD_OPTIONS_C