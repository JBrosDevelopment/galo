#pragma once
#include "galo_headers.h"

#define BUILTIN_FUNCTION(name) GaloObject builtin_##name(Interpreter_Object* interp, GaloObject* args, int arg_count)

BUILTIN_FUNCTION(to_string);
BUILTIN_FUNCTION(print);
BUILTIN_FUNCTION(println);
BUILTIN_FUNCTION(exit);
BUILTIN_FUNCTION(input);
BUILTIN_FUNCTION(clear);
BUILTIN_FUNCTION(cast);
BUILTIN_FUNCTION(format);

BUILTIN_FUNCTION(string_length);
BUILTIN_FUNCTION(string_index);
BUILTIN_FUNCTION(string_contains);
BUILTIN_FUNCTION(string_starts_with);
BUILTIN_FUNCTION(string_ends_with);
BUILTIN_FUNCTION(string_replace);
BUILTIN_FUNCTION(string_sub);
BUILTIN_FUNCTION(string_split);
BUILTIN_FUNCTION(string_concat);

BUILTIN_FUNCTION(list_init);
BUILTIN_FUNCTION(list_append);
BUILTIN_FUNCTION(list_remove);
BUILTIN_FUNCTION(list_get);
BUILTIN_FUNCTION(list_length);
BUILTIN_FUNCTION(list_contains);
BUILTIN_FUNCTION(list_index);
BUILTIN_FUNCTION(list_set);
BUILTIN_FUNCTION(list_insert);
BUILTIN_FUNCTION(list_clear);
BUILTIN_FUNCTION(list);

BUILTIN_FUNCTION(is_type);