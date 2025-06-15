#pragma once

#include "nice_ints.h"
#include "type.h"


typedef u64 functionId;

struct _Statment;
typedef struct _Statment Statment;

struct _Project;
typedef struct _Project Project;

struct _Function;
typedef struct _Function Function;

typedef struct _Variable {
    char* name;
    typeId type;
} Variable;

typedef struct _Scope {
    Statment* statements;
    u64 statementCount;
    u64 statementCapacity;
    Variable* variables;
    u64 variableCount;
    u64 variableCapacity;
    struct _Scope* parent;
} Scope;


void addStatementToScope(Scope* scope, Statment* statement, Project* project);

void addVariableToScope(Scope* scope, Variable* variable, Project* project);

Variable* getVariableFromScope(Scope* scope, const char* name);

Variable* getVariableFromTopScope(Scope* scope, const char* name);

void implementScope(Scope* scope, functionId funcId, Token* startToken, Project* project);
