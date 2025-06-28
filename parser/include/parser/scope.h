#pragma once

#include "nice_ints.h"
#include "statment.h"
#include "map.h"

typedef u64 typeId;
typedef u64 functionId;

typedef struct _Variable {
    union {
        Token* token;
        void* overrideable;
    };
    union {
        char* name;
        void* overrideable2;
    };
    typeId type;
    bool initializedToZero;
} Variable;

typedef struct _Scope Scope;
typedef struct _Scope {
    functionId function;
    Arena* arena;
    Variable* variables;
    u64 varilablesCount;
    union {
        u64 varilablesCapacity;
        void* overrideable;
    };
    map nameToVariable;
    Scope* parent;
    Scope* children;
    u64 childrenCount;
    u64 childrenCapacity;
    Statement* statements;
    u64 statementsCount;
    union {
        u64 statementsCapacity;
        void* overrideable2;
    };
    bool isLoop;
} Scope;

extern Scope globalScope;

Scope createScope(functionId function, Arena* arena);

void addVariableToScope(Scope* scope, Token* token, char* name, typeId type);

void addStatementToScope(Scope* scope, Statement* statement);

void addChildToScope(Scope* scope, Scope* child);

Variable* getVariableByName(Scope* scope, char* name);

void implementScope(Scope* scope, Token** tokens);

bool variableExistsInScope(Scope* scope, char* name);
