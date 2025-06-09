#pragma once

#include "nice_ints.h"
#include "parser/type.h"
#include "parser/tokens.h"

typedef struct _Value {
    typeId type;
    union {
        char* number;
        char* str;
        struct _Value* items;
        char* varName;
    };

} Value;


struct _Scope;
struct _Project;
typedef struct _Scope Scope;
typedef struct _Project Project;

Value getValueFromTokens(Token* tokens, Scope* scope, Project* project);
