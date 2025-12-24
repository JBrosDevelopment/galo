#define PROCESS_ARGS_C
#define PROCESS_ARGS_C

#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 
galo [OPTION] [ARGUMENTS] [FLAGS] -- [PASS-THROUGH FLAGS]

OPTIONS:

compile [INPUT_FILE OR -i ARG]
transpile [INPUT_FILE or -i ARG]
interpret [INPUT_FILE or -i ARG]
build [DIRECTORY OR NONE]
run [FILE OR NONE]
new [PROJECT_NAME]

ARGUMENTS: -ARGUMENT VALUE

-i [INPUT_FILE (can be used multiple times)]
-o [OUTPUT_FILE]
-I [INPUT_DIR]
-compiler [COMPILER_PATH_OR_NAME]      `OPTIONAL FOR COMPILE OPTION`
-to [TARGET_LANGUAGE]                  `REQUIRED FOR TRANSPILE OPTION`
-max-steps [MAXIMUM_INTERPRET_STEPS]   `OPTIONAL FOR INTERPRET OPTION`

FLAGS: --FLAG

--emit-tokens
--emit-ast
--emit-validator
--version
--help
--emit-c                               `OPTIONAL FOR COMPILE OPTION`
--emit-obj                             `OPTIONAL FOR COMPILE OPTION`
--debug-symbols                        `OPTIONAL FOR COMPILE OPTION`
--emit-header                          `OPTIONAL FOR TRANSPILE OPTION`
--single-file                          `OPTIONAL FOR TRANSPILE OPTION`
--interpret-debug                      `OPTIONAL FOR INTERPRET OPTION`

PASS-THROUGH FLAGS:

EVERYTHING AFTER `--` IS PASSED DIRECTLY TO THE COMPILED PROGRAM/INTERPRETER
The transpiler does not use pass-through flags and will warn if any are provided.
Interpreter pass-through flags are used in the main function as the argv array.
Compiler pass-through flags are used when invoking the compiler.

EXAMPLES:

galo compile project/main.galo -o project/bin/main.exe -compiler gcc -- -O2 -Wall

galo transpile project/main.galo -o project/build/main.py -to python --single-file

galo interpret project/main.galo --interpret-debug

galo build 
    requires current directory to have build.galo or main.galo with build options

galo run project/main.galo -- arg1 arg2
    requires project/main.galo to have build options or build.galo to be present in the same directory

galo new MyProject
    creates directory MyProject in current directory and adds a basic src/main.galo and build.galo file

*/

void parse_build_option(int argc, char** argv, int* index, char* file_build_options, BuildArguments* build_arguments); // forward declaration
void parse_parse_args_and_flags(int argc, char** argv, int* index, BuildArguments* build_arguments); // forward declaration
void parse_pass_through_flags(int argc, char** argv, int* index, BuildArguments* build_arguments); // forward declaration

BuildArguments process_args(int argc, char** argv, char* file_build_options) {
    BuildArguments build_arguments;
    int index = 0;
    
    parse_build_option(argc, argv, &index, file_build_options, &build_arguments);

    if (index >= argc) {
        return build_arguments;
    }

    parse_parse_args_and_flags(argc, argv, &index, &build_arguments);

    if (index >= argc) {
        return build_arguments;
    }

    parse_pass_through_flags(argc, argv, &index, &build_arguments);

    return build_arguments;
}

void parse_build_option(int argc, char** argv, int* index, char* file_build_options, BuildArguments* build_arguments) {
    // TODO
}
void parse_parse_args_and_flags(int argc, char** argv, int* index, BuildArguments* build_arguments) {
    // TODO
}
void parse_pass_through_flags(int argc, char** argv, int* index, BuildArguments* build_arguments) {
    // TODO
}

void free_build_arguments(BuildArguments* build_arguments) {
    if (build_arguments->input_file_count > 0 && build_arguments->input_files != NULL) {
        free(build_arguments->input_files);        
    }
    if (build_arguments->input_dirs_count > 0 && build_arguments->input_dirs != NULL) {
        free(build_arguments->input_dirs);        
    }
    if (build_arguments->passthrough_flag_count > 0 && build_arguments->passthrough_flags != NULL) {
        free(build_arguments->passthrough_flags);        
    }

    // don't free because not malloced
    //free(build_arguments);
}

char* build_option_to_string(enum BuildOptions build_option) {
    switch (build_option) {
        case OPTION_COMPILE:
            return "compile";
        case OPTION_TRANSPILE:
            return "transpile";
        case OPTION_INTERPRET:
            return "interpret";
        case OPTION_BUILD:
            return "build";
        case OPTION_RUN:
            return "run";
        case OPTION_NEW:
            return "new";
        default:
            return "ERROR: [unknown BUILD OPTION]";
    }
}

void print_option_string(char* option_name, char* option_value) {
    if (option_value != NULL) {
        printf(" %s: %s\n", option_name, option_value);
    } else {
        printf(" %s: NULL\n", option_name);
    }
}

void print_option_list(char* option_name, char** option_values, int count) {
    if (count == 0) {
        printf(" %s: NONE\n", option_name);
        return;
    }
    printf(" %s:\n", option_name);
    for (int i = 0; i < count; i++) {
        printf("  - %s\n", option_values[i]);
    }
}

void print_option_bool(char* option_name, bool option_value) {
    printf(" %s: %s\n", option_name, option_value ? "true" : "false");
}

void print_option_int(char* option_name, int option_value) {
    printf(" %s: %d\n", option_name, option_value);
}

void debug_build_arguments(BuildArguments* build_arguments) {
    printf("BUILD ARGUMENTS:\n");
    printf(" Build Option: %s\n", build_option_to_string(build_arguments->build_option));
    printf("Universal Arguments:\n");
    print_option_list(" Input File", build_arguments->input_files, build_arguments->input_file_count);
    print_option_string(" Output File", build_arguments->output_file);
    print_option_list(" Input Files", build_arguments->input_files, build_arguments->input_file_count);
    print_option_list(" Input Dirs", build_arguments->input_dirs, build_arguments->input_dirs_count);
    print_option_list(" Passthrough Flags", build_arguments->passthrough_flags, build_arguments->passthrough_flag_count);
    printf("Universal Flags:\n");
    print_option_bool(" Emit Tokens", build_arguments->emit_tokens);
    print_option_bool(" Emit AST", build_arguments->emit_ast);
    print_option_bool(" Emit Validator", build_arguments->emit_validator);
    print_option_bool(" Version", build_arguments->version);
    print_option_bool(" Help", build_arguments->help);
    printf("Compiler Arguments/Flags:\n");
    print_option_string(" Compiler", build_arguments->compiler);
    print_option_bool(" Emit C", build_arguments->emit_c);
    print_option_bool(" Emit Obj", build_arguments->emit_obj);
    print_option_bool(" Debug Symbols", build_arguments->debug_symbols);
    printf("Transpiler Arguments/Flags:\n");
    print_option_string(" Target Language", build_arguments->target_lang);
    print_option_bool(" Emit Headers", build_arguments->emit_headers);
    print_option_bool(" Single File", build_arguments->single_file);
    printf("Interpreter Arguments/Flags:\n");
    print_option_int(" Max Steps", build_arguments->max_steps);
    print_option_bool(" Interpret Debug", build_arguments->interpret_debug);
    printf("END OF BUILD ARGUMENTS\n");
}

#undef PROCESS_ARGS_C