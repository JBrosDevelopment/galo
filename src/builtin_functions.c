#include "builtin_functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

void runtime_error(Interpreter_Object* interp, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("Runtime error: ");
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    interp->did_exit = true;
}

static void append_to_string(char** buffer, size_t* capacity, size_t* length, const char* text) {
    size_t add = strlen(text);

    if (*length + add + 1 > *capacity) {
        *capacity = (*capacity + add + 1) * 2;
        char* new_buf = realloc(*buffer, *capacity);
        if (!new_buf) {
            printf("Out of memory\n");
            exit(1);
        }
        *buffer = new_buf;
    }

    memcpy(*buffer + *length, text, add);
    *length += add;
    (*buffer)[*length] = '\0';
}

BUILTIN_FUNCTION(to_string) {
    GaloObject object = args[0];
    if (object.type_id == STRING_TYPE) {
        char* string = strdup((char*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == INT_TYPE) {
        char* string = calloc(32, sizeof(char));
        sprintf(string, "%d", *(int*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == FLOAT_TYPE) {
        char* string = calloc(32, sizeof(char));
        sprintf(string, "%f", *(float*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == BOOLEAN_TYPE) {
        char* string = calloc(6, sizeof(char));
        if (*(bool*)object.data) {
            sprintf(string, "true");
        } else {
            sprintf(string, "false");
        }
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == BYTE_TYPE) {
        char* string = calloc(8, sizeof(char));
        sprintf(string, "%d", *(unsigned char*)object.data);
        GaloObject new_object = string_object_value(string);
        return new_object;
    } else if (object.type_id == LIST_TYPE) {
        size_t cap = 64;
        size_t len = 0;
        char* string = malloc(cap);
        string[0] = '\0';
    
        append_to_string(&string, &cap, &len, "[");
    
        ObjectList* list = (ObjectList*)object.data;

        for (int i = 0; i < list->size; i++) {
            GaloObject* item = get_object(list, i);
            GaloObject item_str = builtin_to_string(interp, item, 1);
    
            append_to_string(&string, &cap, &len, (char*)item_str.data);
    
            if (i != list->size - 1) {
                append_to_string(&string, &cap, &len, ", ");
            }
    
            free(item_str.data);
        }
    
        append_to_string(&string, &cap, &len, "]");
    
        return string_object_value(string);
    } else {
        size_t cap = 64;
        size_t len = 0;
        char* string = malloc(cap);
        string[0] = '\0';
    
        append_to_string(&string, &cap, &len, "{");
    
        StructDeclaration* decl = get_struct_from_id(object.type_id, interp->validator_object);
    
        for (int i = 0; i < decl->field_count; i++) {
            GaloObject field = get_field_in_struct(interp, &object, object.type_id, decl->fields[i].name.name);
    
            GaloObject field_str = builtin_to_string(interp, &field, 1);
    
            append_to_string(&string, &cap, &len, decl->fields[i].name.name->value);
            append_to_string(&string, &cap, &len, " = ");
            append_to_string(&string, &cap, &len, (char*)field_str.data);
    
            if (i != decl->field_count - 1) {
                append_to_string(&string, &cap, &len, ", ");
            }
    
            free(field_str.data);
        }
    
        append_to_string(&string, &cap, &len, "}");
    
        return string_object_value(string);
    }    
}

BUILTIN_FUNCTION(print) {
    for (int i = 0; i < arg_count; i++) {
        GaloObject arg = args[i];
        GaloObject arg_to_string = builtin_to_string(interp, &arg, 1);
        char* string = (char*)arg_to_string.data;
        printf("%s", string);
        free(arg_to_string.data);
    }

    return void_object_value();
}

BUILTIN_FUNCTION(println) {
    builtin_print(interp, args, arg_count);
    printf("\n");

    return void_object_value();
}

BUILTIN_FUNCTION(exit) {
    int exit_code = *(int*)args[0].data;
    interp->did_exit = true;
    interp->exit_code = exit_code;
    return void_object_value();
}

BUILTIN_FUNCTION(input) {
    size_t capacity = 64;
    size_t length = 0;
    char* buffer = malloc(capacity);

    if (!buffer) {
        perror("malloc");
        exit(1);
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
            if (!buffer) {
                perror("realloc");
                exit(1);
            }
        }
        buffer[length++] = (char)c;
    }

    buffer[length] = '\0';

    GaloObject input_object = string_object_value(buffer);
    return input_object;
}

BUILTIN_FUNCTION(clear) {
    system("clear");
    return void_object_value();
}

BUILTIN_FUNCTION(cast) {   
    int target_type = *(int*)args[0].data;
    GaloObject src = args[1];

    if (target_type == src.type_id) {
        GaloObject copy;
        copy.type_id = src.type_id;
        copy.size = src.size;
        copy.data = malloc(src.size);
        memcpy(copy.data, src.data, src.size);
        return copy;
    }

    GaloObject result;
    result.type_id = target_type;

    switch (target_type) {

    case INT_TYPE: {
        int value;
        char* end;

        switch (src.type_id) {
        case FLOAT_TYPE:
            value = (int)(*(float*)src.data);
            break;
        case BYTE_TYPE:
            value = (int)(*(unsigned char*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1 : 0;
            break;
        case STRING_TYPE:
            value = (int)strtol((char*)src.data, &end, 10);
            if (*end != '\0') {
                runtime_error(interp, "Invalid int cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            break;
        default:
            runtime_error(interp, "Cannot cast type %d to int", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(int);
        result.data = malloc(sizeof(int));
        *(int*)result.data = value;
        return result;
    }

    case FLOAT_TYPE: {
        float value;
        char* end;

        switch (src.type_id) {
        case INT_TYPE:
            value = (float)(*(int*)src.data);
            break;
        case BYTE_TYPE:
            value = (float)(*(unsigned char*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1.0f : 0.0f;
            break;
        case STRING_TYPE:
            value = strtof((char*)src.data, &end);
            if (*end != '\0') {
                runtime_error(interp, "Invalid float cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            break;
        default:
            runtime_error(interp, "Cannot cast type %d to float", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(float);
        result.data = malloc(sizeof(float));
        *(float*)result.data = value;
        return result;
    }

    case BYTE_TYPE: {
        unsigned char value;

        switch (src.type_id) {
        case INT_TYPE:
            value = (unsigned char)(*(int*)src.data);
            break;
        case FLOAT_TYPE:
            value = (unsigned char)(*(float*)src.data);
            break;
        case BOOLEAN_TYPE:
            value = *(bool*)src.data ? 1 : 0;
            break;
        case STRING_TYPE: {
            char* end;
            long tmp = strtol((char*)src.data, &end, 10);
            if (*end != '\0' || tmp < 0 || tmp > 255) {
                runtime_error(interp, "Invalid byte cast from string: '%s'", (char*)src.data);
                return void_object_value();
            }
            value = (unsigned char)tmp;
            break;
        }
        default:
            runtime_error(interp, "Cannot cast type %d to byte", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(unsigned char);
        result.data = malloc(sizeof(unsigned char));
        *(unsigned char*)result.data = value;
        return result;
    }

    case BOOLEAN_TYPE: {
        bool value;

        switch (src.type_id) {
        case INT_TYPE:
            value = (*(int*)src.data) != 0;
            break;
        case FLOAT_TYPE:
            value = (*(float*)src.data) != 0.0f;
            break;
        case BYTE_TYPE:
            value = (*(unsigned char*)src.data) != 0;
            break;
        case STRING_TYPE: {
            char* s = (char*)src.data;
            value = !(s[0] == '\0' ||
                      strcmp(s, "0") == 0 ||
                      strcmp(s, "false") == 0);
            break;
        }
        default:
            runtime_error(interp, "Cannot cast type %d to bool", src.type_id);
            return void_object_value();
        }

        result.size = sizeof(bool);
        result.data = malloc(sizeof(bool));
        *(bool*)result.data = value;
        return result;
    }

    case STRING_TYPE:
        return builtin_to_string(interp, &args[1], 1);

    default:
        runtime_error(interp, "Invalid cast target type id: %d", target_type);
        return void_object_value();
    }
}

BUILTIN_FUNCTION(format) {
    if (arg_count == 0) {
        return string_object_value(strdup(""));
    }

    size_t capacity = 64;
    size_t length = 0;
    char* buffer = malloc(capacity);

    buffer[0] = '\0';

    for (int i = 0; i < arg_count; i++) {
        GaloObject arg = args[i];
        GaloObject str_obj = builtin_to_string(interp, &arg, 1);
        char* str = (char*)str_obj.data;
        size_t str_len = strlen(str);

        while (length + str_len + 1 > capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
            if (!buffer) {
                printf("ERROR: Out of memory in format()\n");
                exit(1);
            }
        }

        memcpy(buffer + length, str, str_len);
        length += str_len;
        buffer[length] = '\0';

        free(str_obj.data);
    }

    return string_object_value(buffer);
}

BUILTIN_FUNCTION(string_length) {
    GaloObject string_object = args[0];

    char* string = (char*)string_object.data;
    int length = strlen(string);
    
    return int_object_value(length);
}

BUILTIN_FUNCTION(string_index) {
    char* string = (char*)args[0].data;
    int index = *(int*)args[1].data;

    unsigned char byte = string[index];

    return byte_object_value(byte);
}

BUILTIN_FUNCTION(string_contains) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strstr(string, substring) != NULL;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_starts_with) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strncmp(string, substring, strlen(substring)) == 0;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_ends_with) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;

    bool result = strncmp(string + strlen(string) - strlen(substring), substring, strlen(substring)) == 0;

    return bool_object_value(result);
}

BUILTIN_FUNCTION(string_replace) {
    char* string = (char*)args[0].data;
    char* substring = (char*)args[1].data;
    char* replacement = (char*)args[2].data;

    size_t string_length = strlen(string);
    size_t substring_length = strlen(substring);
    size_t replacement_length = strlen(replacement);

    if (substring_length == 0) {
        return string_object_value(strdup(string));
    }

    size_t count = 0;
    char* cursor = string;
    while ((cursor = strstr(cursor, substring)) != NULL) {
        count++;
        cursor += substring_length;
    }

    if (count == 0) {
        return string_object_value(strdup(string));
    }

    size_t new_length = string_length + count * (replacement_length - substring_length);

    char* new_string = malloc(new_length + 1);
    if (!new_string) {
        printf("ERROR: Out of memory in string replace()\n");
        exit(1);
    }

    char* src = string;
    char* dst = new_string;

    while ((cursor = strstr(src, substring)) != NULL) {
        size_t bytes = cursor - src;
        memcpy(dst, src, bytes);
        dst += bytes;

        memcpy(dst, replacement, replacement_length);
        dst += replacement_length;

        src = cursor + substring_length;
    }

    strcpy(dst, src);

    return string_object_value(new_string);
}

BUILTIN_FUNCTION(string_sub) {
    char* string = (char*)args[0].data;
    int start = *(int*)args[1].data;
    int end = *(int*)args[2].data;

    char* substring = malloc(end - start + 1);
    if (!substring) {
        printf("ERROR: Out of memory in string sub()\n");
        exit(1);
    }

    memcpy(substring, string + start, end - start);
    substring[end - start] = '\0';

    return string_object_value(substring);
}

BUILTIN_FUNCTION(string_split) {
    char* string = (char*)args[0].data;
    char separator = ((char*)args[1].data)[0];

    ObjectList* result = create_object_list();

    char* start = string;
    char* cursor;

    while ((cursor = strchr(start, separator)) != NULL) {
        size_t len = cursor - start;

        char* part = malloc(len + 1);
        if (!part) {
            printf("ERROR: Out of memory in string_split()\n");
            exit(1);
        }

        memcpy(part, start, len);
        part[len] = '\0';

        GaloObject* object = malloc(sizeof(GaloObject));
        *object = string_object_value(part);

        add_object(result, object, sizeof(GaloObject));

        free(object);

        start = cursor + 1;
    }

    size_t len = strlen(start);
    char* part = malloc(len + 1);
    if (!part) {
        printf("ERROR: Out of memory in string_split()\n");
        exit(1);
    }

    memcpy(part, start, len);
    part[len] = '\0';

    GaloObject* object = malloc(sizeof(GaloObject));
    *object = string_object_value(part);

    add_object(result, object, sizeof(GaloObject));

    free(object);
    
    return list_object_value(result);
}

BUILTIN_FUNCTION(string_concat) {
    if (arg_count == 0) {
        return string_object_value(strdup(""));
    }

    size_t total_length = 0;
    for (int i = 0; i < arg_count; i++) {
        total_length += strlen((char*)args[i].data);
    }

    char* result = malloc(total_length + 1);
    if (!result) {
        printf("ERROR: Out of memory in string_concat()\n");
        exit(1);
    }

    char* cursor = result;
    for (int i = 0; i < arg_count; i++) {
        char* s = (char*)args[i].data;
        size_t len = strlen(s);
        memcpy(cursor, s, len);
        cursor += len;
    }

    result[total_length] = '\0';

    return string_object_value(result);
}

BUILTIN_FUNCTION(string_trim) {
    char* string = (char*)args[0].data;
    char* start = string;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    size_t length = (size_t)(end - start);
    char* result = malloc(length + 1);
    if (!result) {
        perror("malloc");
        exit(1);
    }

    memcpy(result, start, length);
    result[length] = '\0';
    return string_object_value(result);
}

BUILTIN_FUNCTION(is_type) {
    int type = *(int*)args[0].data;
    int arg_type = args[1].type_id;
    return bool_object_value(arg_type == type);
}

BUILTIN_FUNCTION(list_init) {
    // this parameter is here for syntax reasons
    // the list can hold any type and will, but for ease of looking at what the code is supposed to do, the type is specified
    // for example: `list init(int, 10)` will create a list that can hold 10 integers, but it could hold 10 of anything
    // it's just there so the code is more readable
    int type = *(int*)args[0].data;  
    type = type; // unused, here to avoid compiler warning
    
    int capacity = *(int*)args[1].data;
    
    if (capacity == 0) {
        runtime_error(interp, "In list init(), capacity must be greater than 0");
        return void_object_value();
    }

    ObjectList* list = malloc(sizeof(ObjectList));
    list->size = 0;
    list->capacity = capacity;
    list->objects = malloc(sizeof(void*) * list->capacity);

    return list_object_value(list);
}

BUILTIN_FUNCTION(list_append) { 
    ObjectList* list = (ObjectList*)args[0].data;
    GaloObject object = args[1];
    
    GaloObject* copy = malloc(sizeof(GaloObject));
    copy->type_id = object.type_id;
    copy->size = object.size;
    copy->data = malloc(copy->size);
    memcpy(copy->data, object.data, copy->size);

    add_object(list, copy, sizeof(GaloObject));
    free(copy);
    return list_object_value(list);
}

BUILTIN_FUNCTION(list_remove) {
    ObjectList* list = (ObjectList*)args[0].data;
    int index = *(int*)args[1].data;

    free(get_object(list, index));

    for (int i = index + 1; i < list->size; i++) {
        list->objects[i - 1] = list->objects[i];
    }

    list->size--;
    return list_object_value(list);
}

BUILTIN_FUNCTION(list_get) {
    ObjectList* list = (ObjectList*)args[0].data;
    int index = *(int*)args[1].data;

    if (index < 0 || index >= list->size) {
        runtime_error(interp, "Index out `%d` of bounds", index);
        return void_object_value();
    }

    GaloObject* object = get_object(list, index);

    GaloObject copy;
    copy.type_id = object->type_id;
    copy.size = object->size;
    copy.data = malloc(copy.size);
    memcpy(copy.data, object->data, copy.size);

    return copy;
}

BUILTIN_FUNCTION(list_length) {
    ObjectList* list = (ObjectList*)args[0].data;
    return int_object_value(list->size);
}

BUILTIN_FUNCTION(list_contains) {
    ObjectList* list = (ObjectList*)args[0].data;
    GaloObject value = args[1];

    for (int i = 0; i < list->size; i++) {
        GaloObject* item = get_object(list, i);
        if (item == NULL) {
            runtime_error(interp, "NULL item in list");
            return void_object_value();
        }
        if (item->type_id == value.type_id && item->size == value.size && memcmp(item->data, value.data, value.size) == 0) {
            return bool_object_value(1);
        }
    }

    return bool_object_value(0);
}

BUILTIN_FUNCTION(list_index) {
    ObjectList* list = (ObjectList*)args[0].data;
    GaloObject value = args[1];

    for (int i = 0; i < list->size; i++) {
        GaloObject* item = get_object(list, i);
        if (item == NULL) {
            runtime_error(interp, "NULL item in list");
            return int_object_value(-1);
        }
        if (item->type_id == value.type_id && item->size == value.size && memcmp(item->data, value.data, value.size) == 0) {
            return int_object_value(i);
        }
    }

    return int_object_value(-1);
}

BUILTIN_FUNCTION(list_set) {
    ObjectList* list = (ObjectList*)args[0].data;
    int index = *(int*)args[1].data;
    GaloObject value = args[2];

    if (index < 0 || index >= list->size) {
        runtime_error(interp, "Index out of bounds");
        return void_object_value();
    }    

    GaloObject* item = get_object(list, index);
    if (item == NULL) {
        runtime_error(interp, "NULL item in list");
        return void_object_value();
    }

    free(item->data);

    item->type_id = value.type_id;
    item->size = value.size;
    item->data = malloc(item->size);
    memcpy(item->data, value.data, item->size);

    return list_object_value(list);
}

BUILTIN_FUNCTION(list_insert) {
    ObjectList* list = (ObjectList*)args[0].data;
    int index = *(int*)args[1].data;
    GaloObject value = args[2];

    if (index < 0 || index > list->size) {
        runtime_error(interp, "Index out of bounds");
        return void_object_value();
    }

    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->objects = realloc(list->objects, sizeof(GaloObject) * list->capacity);
    }

    for (int i = list->size; i > index; i--) {
        list->objects[i] = list->objects[i - 1];
    }

    GaloObject* obj = malloc(sizeof(GaloObject));
    obj->type_id = value.type_id;
    obj->size = value.size;
    obj->data = malloc(value.size);
    memcpy(obj->data, value.data, value.size);

    list->objects[index] = obj;
    list->size++;

    return list_object_value(list);
}

BUILTIN_FUNCTION(list_clear) {
    ObjectList* list = (ObjectList*)args[0].data;

    for (int i = 0; i < list->size; i++) {
        GaloObject* item = get_object(list, i);
        if (item == NULL) {
            runtime_error(interp, "NULL item in list");
            return void_object_value();
        }
        free(item->data);
    }

    list->size = 0;

    return list_object_value(list);
}

BUILTIN_FUNCTION(list) {
    ObjectList* list = create_object_list();

    for (int i = 1; i < arg_count; i++) {
        GaloObject* obj = malloc(sizeof(GaloObject));
        obj->type_id = args[i].type_id;
        obj->size = args[i].size;
        obj->data = malloc(obj->size);
        memcpy(obj->data, args[i].data, obj->size);

        add_object(list, obj, sizeof(GaloObject));
    }

    return list_object_value(list);
}
