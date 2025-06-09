#pragma once
#include "nice_ints.h"
#include "parser/scope.h"


typedef enum _ExpresstionType : u8 {
    et_error,
    et_variable,
    et_type,
    et_int,
    et_float,
    et_string,
    et_struct,
    et_biop,
    et_function,
} ExpresstionType;

struct _Expresstion;
typedef struct _Expresstion Expresstion;


typedef struct _StructConstant {
    Expresstion* expresstions;
    u64 expresstionCount;
} StructConstant;

typedef struct _BiopExpresstion {
    Expresstion* left;
    Expresstion* right;
    OurTokenType operator;
} BiopExpresstion;

typedef struct _FunctionExpresstion {
    Function* func;
    Expresstion* parameters;
    u64 parameterCount;
} FunctionExpresstion;

typedef struct _Expresstion {
    ExpresstionType type;
    typeId tid;
    union {
        char* variable;
        char* number;
        char* string;
        StructConstant* structConstant;
        BiopExpresstion* biopExpresstion;
        FunctionExpresstion* functionExpresstion;
    };
} Expresstion;

struct _Function;
typedef struct _Function Function;

Expresstion createExpresstionFromTokensDels(Token** tokens, OurTokenType* dels, u64 delsSize, Function* function, Scope* scope, Project* project);
Expresstion createExpresstionFromTokens(Token** tokens, OurTokenType dels, Function* function, Scope* scope, Project* project);
