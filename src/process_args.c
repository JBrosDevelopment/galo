#include "galo_headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <direct.h>   // _getcwd
    #define getcwd _getcwd
#else
    #include <unistd.h>   // getcwd
#endif

/* 
galo [OPTION] [ARGUMENTS] [FLAGS] -- [PASS-THROUGH FLAGS]

OPTIONS:

compile [INPUT_FILE OR -i ARG]
transpile [INPUT_FILE or -i ARG]
interpret [INPUT_FILE or -i ARG]
build [DIRECTORY OR NONE]
run [FILE OR NONE]
new [PROJECT_NAME]
version
help

ARGUMENTS: -ARGUMENT VALUE

-i [INPUT_FILE (can be used multiple times)]
-o [OUTPUT_FILE]
-I [INPUT_DIR]
-t [TARGET_LANGUAGE]                    `REQUIRED FOR TRANSPILE OPTION` (alias `--target-lang`)
--compiler [COMPILER_PATH_OR_NAME]      `OPTIONAL FOR COMPILE OPTION`
--max-steps [MAXIMUM_INTERPRET_STEPS]   `OPTIONAL FOR INTERPRET OPTION`

FLAGS: --FLAG

--emit-tokens
--emit-ast
--emit-validator
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

galo compile project/main.galo -o project/bin/main.exe --compiler gcc -- -O2 -Wall
    compiles main.galo to an executable using gcc and outputs to project/bin/main.exe with optimization and all warnings enabled

galo transpile project/main.galo -o project/build/main.py -t python --single-file
    transpiles main.galo to Python and outputs to project/build/main.py as a single file

galo interpret project/main.galo --interpret-debug
    starts the interpreter with the main.galo file and enables debug mode

galo build 
    requires current directory to have build.galo with build options

galo run project/main.galo
    requires project/main.galo to have build options

galo new MyProject
    creates directory MyProject in current directory and adds a basic src/main.galo and build.galo file

galo version
    prints the current Galo version

galo help
    prints out a help message with usage information

*/

bool parse_build_option(int argc, char** argv, int* index, BuildArguments* build_arguments); // forward declaration
void parse_parse_args_and_flags(int argc, char** argv, int* index, BuildArguments* build_arguments); // forward declaration
void parse_pass_through_flags(int argc, char** argv, int* index, BuildArguments* build_arguments); // forward declaration
void check_required_arguments(BuildArguments* build_arguments); // forward declaration
void check_unnecessary_arguments(BuildArguments* build_arguments); // forward declaration
void check_build_option_compatibility(BuildArguments* build_arguments); // forward declaration
void check_run_option_compatibility(BuildArguments* build_arguments); // forward declaration
void print_option_list(char* option_name, char** option_values, int count); // forward declaration
void print_option_string(char* option_name, char* option_value); // forward declaration
void print_option_bool(char* option_name, bool option_value); // forward declaration
void print_option_int(char* option_name, int option_value); // forward declaration
int get_current_directory(char *buffer, size_t size); // forward declaration
static int dir_exists(const char* path); // forward declaration
static int file_exists(const char* path); // forward declaration

BuildArguments process_args(int argc, char** argv) {
    // for debugging
    // printf("\n");
    // printf("DEBUG INPUT ARGUMENTS:\n");
    // print_option_list("ARGS", argv, argc);
    // printf("\n");

    BuildArguments build_arguments;
    build_arguments.input_dirs = create_string_list();
    build_arguments.input_files = create_string_list();
    build_arguments.passthrough_flags = create_string_list();
    build_arguments.file_build_options = create_string_list();
    build_arguments.output_file = NULL;
    build_arguments.compiler = NULL;
    build_arguments.target_lang = NULL;
    build_arguments.project_name = NULL;
    build_arguments.build_file_path = NULL;
    build_arguments.main_file_path = NULL;
    build_arguments.emit_tokens = false;
    build_arguments.emit_ast = false;
    build_arguments.emit_validator = false;
    build_arguments.emit_c = false;
    build_arguments.emit_obj = false;
    build_arguments.debug_symbols = false;
    build_arguments.interpret_debug = false;
    build_arguments.emit_headers = false;
    build_arguments.single_file = false;
    build_arguments.max_steps = -1;

    int index = 1; // start at 1 to skip program name
    
    if (index >= argc) {
        printf("Error: No build option provided. Use `galo help` for more information.\n");
        exit(1);
    }

    bool done_parsing = parse_build_option(argc, argv, &index, &build_arguments);

    if (done_parsing) {
        goto done_parsing;
    }

    if (index >= argc) {
        goto done_parsing;
    }

    parse_parse_args_and_flags(argc, argv, &index, &build_arguments);

    if (index >= argc) {
        goto done_parsing;
    }

    parse_pass_through_flags(argc, argv, &index, &build_arguments);

done_parsing:
    check_required_arguments(&build_arguments);
    check_unnecessary_arguments(&build_arguments);

    if (build_arguments.build_option == OPTION_BUILD) {
        check_build_option_compatibility(&build_arguments);
    }

    if (build_arguments.build_option == OPTION_RUN) {
        check_run_option_compatibility(&build_arguments);
    }

    return build_arguments;
}

bool parse_build_option(int argc, char** argv, int* index, BuildArguments* build_arguments) { // returns if done parsing after this
    char* option = argv[*index];
    bool next_could_be_file = false;
    bool next_could_be_directory = false;
    bool next_is_project_name = false;
    bool ignore_next = false;
    if (strcmp(option, "compile") == 0) {
        build_arguments->build_option = OPTION_COMPILE;
        next_could_be_file = true;
    } else if (strcmp(option, "transpile") == 0) {
        build_arguments->build_option = OPTION_TRANSPILE;
        next_could_be_file = true;
    } else if (strcmp(option, "interpret") == 0) {
        build_arguments->build_option = OPTION_INTERPRET;
        next_could_be_file = true;
    } else if (strcmp(option, "build") == 0) {
        build_arguments->build_option = OPTION_BUILD;
        next_could_be_directory = true;
    } else if (strcmp(option, "run") == 0) {
        build_arguments->build_option = OPTION_RUN;
        next_could_be_file = true;
        next_could_be_directory = true;
    } else if (strcmp(option, "new") == 0) {
        build_arguments->build_option = OPTION_NEW;
        next_is_project_name = true;
    } else if (strcmp(option, "version") == 0) {
        build_arguments->build_option = OPTION_VERSION;
        ignore_next = true;
    } else if (strcmp(option, "help") == 0) {
        build_arguments->build_option = OPTION_HELP;
        ignore_next = true;
    } else {
        printf("Error: Unknown build option `%s`. Use `galo help` for more information.\n", option);
        exit(1);
    }
    (*index)++;

    if (ignore_next) {
        return true;
    }
    if (next_is_project_name) {
        build_arguments->project_name = argv[*index];
        (*index)++;
        return true;
    }
    if (next_could_be_file) {
        if (*index < argc) {
            char* possible_file = strdup(argv[*index]);
            if (possible_file[0] != '-') {
                // not a flag, assume it's a file
                add_string(build_arguments->input_files, possible_file);
                (*index)++;
            }
        }
    }
    if (next_could_be_directory) {
        if (*index < argc) {
            char* possible_dir = strdup(argv[*index]);
            if (possible_dir[0] != '-') {
                // not a flag, assume it's a directory
                add_string(build_arguments->input_dirs, possible_dir);
                (*index)++;
            }
        }
    }

    return false;
}
void parse_parse_args_and_flags(int argc, char** argv, int* index, BuildArguments* build_arguments) {
    while (*index < argc) {
        char* arg = argv[*index];

        if (arg[0] != '-') {
            printf("Error: Unexpected argument or flag `%s` in position %d. Use `galo help` for more information.\n", arg, *index);
            exit(1);
        }

        if (arg[1] == '-') {
            if (strlen(arg) == 2) {
                // Pass-through flags start here
                break;
            }

            // long flag or argument
            char* flag_name = &arg[2];

            char* argument_value = NULL;
            if (strcmp(flag_name, "compiler") == 0 || strcmp(flag_name, "target-lang") == 0 || strcmp(flag_name, "max-steps") == 0) {
                (*index)++;
                if (*index >= argc) {
                    printf("Error: Expected value for argument `%s` in position %d. Use `galo help` for more information.\n", flag_name, *index - 1);
                    exit(1);
                } else {
                    argument_value = strdup(argv[*index]);
                }
            }
            
            if (strcmp(flag_name, "emit-tokens") == 0) {
                build_arguments->emit_tokens = true;
            } else if (strcmp(flag_name, "emit-ast") == 0) {
                build_arguments->emit_ast = true;
            } else if (strcmp(flag_name, "emit-validator") == 0) {
                build_arguments->emit_validator = true;
            } else if (strcmp(flag_name, "emit-c") == 0) {
                build_arguments->emit_c = true;
            } else if (strcmp(flag_name, "emit-obj") == 0) {
                build_arguments->emit_obj = true;
            } else if (strcmp(flag_name, "debug-symbols") == 0) {
                build_arguments->debug_symbols = true;
            } else if (strcmp(flag_name, "emit-header") == 0) {
                build_arguments->emit_headers = true;
            } else if (strcmp(flag_name, "single-file") == 0) {
                build_arguments->single_file = true;
            } else if (strcmp(flag_name, "interpret-debug") == 0) {
                build_arguments->interpret_debug = true;
            } else if (strcmp(flag_name, "compiler") == 0) {
                build_arguments->compiler = argument_value;
            } else if (strcmp(flag_name, "target-lang") == 0) {
                build_arguments->target_lang = argument_value;
            } else if (strcmp(flag_name, "max-steps") == 0) {
                int max_steps = atoi(argument_value);
                if (max_steps <= 0) {
                    printf("Error: Max steps must be a positive integer. Use `galo help` for more information.\n");
                    exit(1);
                }
                build_arguments->max_steps = max_steps;
            } else {
                printf("Error: Unknown flag `%s` in position %d. Use `galo help` for more information.\n", flag_name, *index);
                exit(1);
            }
        } else {
            // short flag or argument
            char* argument_name = &arg[1];
            
            (*index)++;
            char* argument_value = NULL;
            if (*index >= argc) {
                printf("Error: Expected value for argument `%s` in position %d. Use `galo help` for more information.\n", argument_name, *index - 1);
                exit(1);
            } else {
                argument_value = strdup(argv[*index]);
            }

            if (strcmp(argument_name, "i") == 0) {
                add_string(build_arguments->input_files, argument_value);
            } else if (strcmp(argument_name, "o") == 0) {
                build_arguments->output_file = argument_value;
            } else if (strcmp(argument_name, "I") == 0) {
                add_string(build_arguments->input_dirs, argument_value);
            } else if (strcmp(argument_name, "t") == 0) {
                build_arguments->target_lang = argument_value;
            } else {
                printf("Error: Unknown argument `%s` in position %d. Use `galo help` for more information.\n", argument_name, *index);
                exit(1);
            }
        }

        (*index)++;
    }
}
void parse_pass_through_flags(int argc, char** argv, int* index, BuildArguments* build_arguments) {
    if (*index < argc && strcmp(argv[*index], "--") == 0) {
        (*index)++;
    } else {
        return;
    }

    while (*index < argc && strcmp(argv[*index], "\n") != 0 && strcmp(argv[*index], "\r") != 0 && strcmp(argv[*index], "\0") != 0) {
        char* pass_through_flag = strdup(argv[*index]);
        add_string(build_arguments->passthrough_flags, pass_through_flag);
        (*index)++;
    }
}
void check_required_arguments(BuildArguments* build_arguments) {
    if (build_arguments->build_option == OPTION_COMPILE) {
        if (build_arguments->input_files->size == 0 || build_arguments->input_files == NULL) {
            printf("Error: No input file provided for `compile` option. Use `galo help` for more information.\n");
            exit(1);
        }
    } else if (build_arguments->build_option == OPTION_TRANSPILE) {
        if (build_arguments->input_files->size == 0 || build_arguments->input_files == NULL) {
            printf("Error: No input file provided for `transpile` option. Use `galo help` for more information.\n");
            exit(1);
        }
        if (build_arguments->target_lang == NULL) {
            printf("Error: No target language provided for `transpile` option. Use `galo help` for more information.\n");
            exit(1);
        }
    } else if (build_arguments->build_option == OPTION_INTERPRET) {
        if (build_arguments->input_files->size == 0 || build_arguments->input_files == NULL) {
            printf("Error: No input file provided for `interpret` option. Use `galo help` for more information.\n");
            exit(1);
        }
    } else if (build_arguments->build_option == OPTION_BUILD) {
        if (build_arguments->input_dirs->size == 0 || build_arguments->input_dirs == NULL) {
            // instead of throwing an error, assume current directory
            char current_dir[1024];
            if (get_current_directory(current_dir, sizeof(current_dir))) {
                char* current_dir_copy = strdup(current_dir);
                trim_trailing_whitespace(current_dir_copy);
                add_string(build_arguments->input_dirs, current_dir_copy);
            } else {
                printf("Error: No input directory provided for `build` option and failed to get current directory. Use `galo help` for more information.\n");
                exit(1);
            }
        }
    } else if (build_arguments->build_option == OPTION_RUN) {
        if (build_arguments->input_files->size == 0 || build_arguments->input_files == NULL) {
            char current_dir[1024];
            if (get_current_directory(current_dir, sizeof(current_dir)) == 0) {
                printf("Error: No input file provided for `run` option and failed to get current directory. Use `galo help` for more information.\n");
                exit(1);
            }

            char* target_file_name = "main.galo";
            char* main_file_path = malloc(strlen(current_dir) + strlen(target_file_name) + 2);
            #ifdef _WIN32
                sprintf(main_file_path, "%s\\%s", current_dir, target_file_name);
            #else
                sprintf(main_file_path, "%s/%s", current_dir, target_file_name);
            #endif
            add_string(build_arguments->input_files, main_file_path);
        }
    } else if (build_arguments->build_option == OPTION_NEW) {
        if (build_arguments->project_name == NULL) {
            printf("Error: No project name provided for `new` option. Use `galo help` for more information.\n");
            exit(1);
        }
    }
}

void check_unnecessary_arguments(BuildArguments* build_arguments) {
    if (build_arguments->target_lang != NULL && build_arguments->build_option != OPTION_TRANSPILE) {
        printf("Warning: Target language argument is unnecessary for `compile` option and will be ignored.\n");
    }
    if (build_arguments->compiler != NULL && build_arguments->build_option != OPTION_COMPILE) {
        printf("Warning: Compiler argument is unnecessary for non `compile` options and will be ignored.\n");
    }
    if (build_arguments->max_steps != -1 && build_arguments->build_option != OPTION_INTERPRET) {
        printf("Warning: Max steps argument is unnecessary for non `interpret` options and will be ignored.\n");
    }
    if (build_arguments->interpret_debug != false && build_arguments->build_option != OPTION_INTERPRET) {
        printf("Warning: Interpret debug argument is unnecessary for non `interpret` options and will be ignored.\n");
    }
    if (build_arguments->emit_c != false && build_arguments->build_option != OPTION_COMPILE) {
        printf("Warning: Emit C flag is unnecessary for non `compile` options and will be ignored.\n");
    }
    if (build_arguments->emit_obj != false && build_arguments->build_option != OPTION_COMPILE) {
        printf("Warning: Emit Obj flag is unnecessary for non `compile` options and will be ignored.\n");
    }
    if (build_arguments->debug_symbols != false && build_arguments->build_option != OPTION_COMPILE) {
        printf("Warning: Debug symbols flag is unnecessary for non `compile` options and will be ignored.\n");
    }
    if (build_arguments->emit_headers != false && build_arguments->build_option != OPTION_TRANSPILE) {
        printf("Warning: Emit headers flag is unnecessary for non `transpile` options and will be ignored.\n");
    }
    if (build_arguments->single_file != false && build_arguments->build_option != OPTION_TRANSPILE) {
        printf("Warning: Single file flag is unnecessary for non `transpile` options and will be ignored.\n");
    }
}

void check_build_option_compatibility(BuildArguments* build_arguments) {
    if (build_arguments->emit_c || build_arguments->emit_obj || build_arguments->debug_symbols) {
        printf("Error: `build` option cannot be used with `compile`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->target_lang != NULL || build_arguments->emit_headers || build_arguments->single_file) {
        printf("Error: `build` option cannot be used with `transpile`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->max_steps != -1 || build_arguments->interpret_debug) {
        printf("Error: `build` option cannot be used with `interpret`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->passthrough_flags->size > 0) {
        printf("Error: `build` option cannot be used with pass-through flags. Use `galo help` for more information.\n");
        exit(1);
    }

    if (build_arguments->input_dirs->size == 0 || build_arguments->input_dirs == NULL) {
        printf("Error: No input directory provided for `build` option. Use `galo help` for more information.\n");
        exit(1);
    }

    char* build_file = NULL;
    for (int i = 0; i < build_arguments->input_dirs->size; i++) {
        char* dir = get_string(build_arguments->input_dirs, i);

        if (!dir_exists(dir)) {
            printf("Error: Input directory `%s` does not exist. Use `galo help` for more information.\n", dir);
            exit(1);
        }

        size_t path_length = strlen(dir) + strlen("/build.galo") + 1;
        char* possible_build_file = malloc(path_length);
        snprintf(possible_build_file, path_length, "%s/build.galo", dir);
        
        if (file_exists(possible_build_file)) {
            if (build_file != NULL) {
                printf("Error: Multiple build.galo files found for `build` option (`%s` and `%s`). Use `galo help` for more information.\n", build_file, possible_build_file);
                free(possible_build_file);
                exit(1);
            }
            build_file = possible_build_file;
        } else {
            free(possible_build_file);
        }
    }

    if (build_file == NULL) {
        printf("Error: No build.galo file found in provided input directories for `build` option. Use `galo help` for more information.\n");
        exit(1);
    }
}

void check_run_option_compatibility(BuildArguments* build_arguments) {
    if (build_arguments->emit_c || build_arguments->emit_obj || build_arguments->debug_symbols) {
        printf("Error: `run` option cannot be used with `compile`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->target_lang != NULL || build_arguments->emit_headers || build_arguments->single_file) {
        printf("Error: `run` option cannot be used with `transpile`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->max_steps != -1 || build_arguments->interpret_debug) {
        printf("Error: `run` option cannot be used with `interpret`-specific flags. Use `galo help` for more information.\n");
        exit(1);
    }
    if (build_arguments->passthrough_flags->size > 0) {
        printf("Error: `run` option cannot be used with pass-through flags. Use `galo help` for more information.\n");
        exit(1);
    }

    if (build_arguments->input_files->size == 0 || build_arguments->input_files == NULL) {
        printf("Error: No input file provided for `run` option. Use `galo help` for more information.\n");
        exit(1);
    }

    char* main_file = NULL;
    for (int i = 0; i < build_arguments->input_files->size; i++) {
        char* file = get_string(build_arguments->input_files, i);

        if (!file_exists(file)) {
            printf("Error: Input file `%s` does not exist. Use `galo help` for more information.\n", file);
            exit(1);
        }

        if (build_arguments->input_files->size == 1) {
            main_file = file;
            break;
        }

        size_t file_name_length = strlen(file);
        if (file_name_length >= 9 && strcmp(&file[file_name_length - 9], "main.galo") == 0) {
            if (main_file != NULL) {
                printf("Error: Multiple main.galo files found for `run` option (`%s` and `%s`). Use `galo help` for more information.\n", main_file, file);
                exit(1);
            }
            main_file = file;
        }
    }

    if (main_file == NULL) {
        if (build_arguments->input_files->size == 1) {
            printf("Error: No galo file found in provided input files for `run` option. Use `galo help` for more information.\n");            
        } else {
            printf("Error: No main.galo file found in provided input files for `run` option. Use `galo help` for more information.\n");
        }
        exit(1);
    }

    StringList* unused_list1 = create_string_list();
    StringList* unused_list2 = create_string_list();
    preprocess_with_build_options(main_file, unused_list1, unused_list2, build_arguments->file_build_options);

    if (build_arguments->file_build_options->size == 0) {
        printf("Error: No build options found in main.galo for `run` option. Use `galo help` for more information.\n");
        exit(1);
    }

    if (build_arguments->file_build_options->size < 1) {
        printf("Error: Insufficient build options found in main.galo for `run` option. Use `galo help` for more information.\n");
        exit(1);
    }

    char* command = get_string(build_arguments->file_build_options, 1);
    if (strcmp(command, "interpret") != 0 && strcmp(command, "transpile") != 0 && strcmp(command, "compile") != 0) {
        printf("Error: Invalid build option `%s` found in main.galo for `run` option. Only `interpret`, `transpile`, or `compile` are allowed. Use `galo help` for more information.\n", command);
        exit(1);
    }

    free_string_list(unused_list1);
    free_string_list(unused_list2);

    build_arguments->main_file_path = main_file;
}

int get_current_directory(char *buffer, size_t size) {
    if (getcwd(buffer, size) != NULL) {
        return 1;
    } else {
        printf("Error: Unable to get current working directory.\n");
        return 0;
    }
}

static int dir_exists(const char* path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

static int file_exists(const char* path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISREG(s.st_mode);
}

void free_build_arguments(BuildArguments* build_arguments) {
    if (build_arguments->input_dirs != NULL) {
        free_string_owned_list(build_arguments->input_dirs);
    }
    if (build_arguments->input_files != NULL) {
        free_string_owned_list(build_arguments->input_files);
    }
    if (build_arguments->passthrough_flags != NULL) {
        free_string_owned_list(build_arguments->passthrough_flags);
    }
    if (build_arguments->file_build_options != NULL) {
        free_string_owned_list(build_arguments->file_build_options);
    }
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
        case OPTION_VERSION:
            return "version";
        case OPTION_HELP:
            return "help";
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
    printf("START OF BUILD ARGUMENTS:\n");
    printf(" Build Option: %s\n", build_option_to_string(build_arguments->build_option));
    printf("Universal Arguments:\n");
    print_option_string(" Output File", build_arguments->output_file);
    print_option_list(" Input Files", build_arguments->input_files->strings, build_arguments->input_files->size);
    print_option_list(" Input Dirs", build_arguments->input_dirs->strings, build_arguments->input_dirs->size);
    print_option_list(" Passthrough Flags", build_arguments->passthrough_flags->strings, build_arguments->passthrough_flags->size);
    printf("Universal Flags:\n");
    print_option_bool(" Emit Tokens", build_arguments->emit_tokens);
    print_option_bool(" Emit AST", build_arguments->emit_ast);
    print_option_bool(" Emit Validator", build_arguments->emit_validator);
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
