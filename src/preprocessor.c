#include "galo_headers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* skip_spaces(char* s); // Forward declaration
static char* read_word(char* s, char* out, int max); // Forward declaration
void trim_trailing_whitespace(char* s); // Forward declaration

void preprocess(char* file_name, FileList* source_code_file_names, FileList* source_code_files, char* file_build_options) {
    const char* source_code = read_file(file_name);
    if (!source_code) {
        exit(1);
    }

    ObjectList* shebang_pointers = create_object_list();
    char done = 0;

    for (int i = 0; source_code[i] != '\0'; ) {
        int start = -1;

        if (i == 0 && source_code[i] == '#' && source_code[i + 1] == '!') {
            start = i + 2;
        }
        else if ((source_code[i] == '\n' || source_code[i] == '\r') && source_code[i + 1] == '#' && source_code[i + 2] == '!') {
            if (done) {
                fprintf(stderr, "ERROR: All shebangs must be at the start of the file with no new lines in between or before: `%s`\n", file_name);
                exit(1);
            }

            start = i + 3;
        }

        if (start == -1) {
            done = 1;
            i++;
            continue;
        }

        int end = start;
        while (source_code[end] != '\0' && source_code[end] != '\n') {
            end++;
        }

        int length = end - start;

        char* copied = malloc(length + 1);
        memcpy(copied, &source_code[start], length);
        copied[length] = '\0';

        add_object(shebang_pointers, &copied, sizeof(char*));

        i = end;
    }
    
    for (int i = 0; i < shebang_pointers->size; i++) {
        char* s = *(char**)get_object(shebang_pointers, i);
        s = skip_spaces(s);
    
        char directive[32];
        char* rest = read_word(s, directive, sizeof(directive));
        rest = skip_spaces(rest);
    
        if (strcmp(directive, "include") == 0) {
            if (*rest == '\0') {
                fprintf(stderr, "ERROR: include requires a file name in file: `%s`\n", file_name);
                exit(1);
            }
    
            char* filename;
            if (*rest == '"') {
                rest++;
                char* end = strchr(rest, '"');
                if (!end) {
                    fprintf(stderr, "ERROR: Unterminated string in include in file: `%s`\n", file_name);
                    exit(1);
                }
                *end = '\0';
                filename = rest;
            } else {
                char* end = strchr(rest, '\r');
                if (end) {
                    *end = '\0';
                }
                filename = rest;
            }
    
            if (filename == NULL) {
                printf("ERROR: Internal error in preprocessing, failed to get child file from parent file: `%s`\n", file_name);
            }
            char* child_file_build_options = NULL;
            if (contains_file(source_code_file_names, filename)) {
                continue;
            }
            preprocess(filename, source_code_file_names, source_code_files, child_file_build_options);
            if (child_file_build_options) {
                printf("WARNING: Child file `%s` has build options `%s` which are ignored\n", filename, child_file_build_options);
            }
        } else if (strcmp(directive, "galo") == 0) {
            if (*rest == '\0') {
                fprintf(stderr, "ERROR: galo requires arguments in file: `%s`\n", file_name);
                exit(1);
            }
    
            char* args = strdup(rest);
            file_build_options = args;
        } else {
            fprintf(stderr, "ERROR: Unknown shebang directive `%s` in file: `%s`\n", directive, file_name);
            exit(1);
        }
    }

    for (int i = 0; i < shebang_pointers->size; i++) {
        free(*(char**)get_object(shebang_pointers, i));
    }
    free_object_list(shebang_pointers);

    add_file(source_code_file_names, file_name);
    add_file(source_code_files, source_code);
}

static char* skip_spaces(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}
static char* read_word(char* s, char* out, int max) {
    int i = 0;
    while (*s && *s != ' ' && *s != '\t' && i < max - 1) {
        out[i++] = *s++;
    }
    out[i] = '\0';
    return s;
}
void trim_trailing_whitespace(char* s) {
    int len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

const char* read_file(char* filename) {
    FILE* f = fopen(filename, "rb");

    if (!f) {
        printf("Failed to open file: '");
        for (int i = 0; i < (int)strlen(filename); i++) {
            printf("%c", filename[i]);
        }
        printf("'\n");
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        printf("Failed to seek to end of file: %s\n", filename);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        printf("Failed to get file size: %s\n", filename);
        return NULL;
    }
    rewind(f);

    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        printf("Failed to allocate buffer: %s\n", filename);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);

    if (read != (size_t)size) {
        free(buffer);
        printf("Failed to read file contents: %s\n", filename);
        return NULL;
    }

    printf("File '%s' read successfully, size: %ld bytes\n", filename, size);

    buffer[size] = '\0';
    return (const char*)buffer; // Caller must free: free((void*)ptr)
}