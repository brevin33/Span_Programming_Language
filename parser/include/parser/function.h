#pragma once

#include "parser/map.h"
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/scope.h"
#include "parser/template.h"
#include "parser/tokens.h"

typedef u64 typeId;
typedef u64 sourceCodeId;
typedef u64 functionId;
typedef u64 projectId;
typedef u64 functionId;

extern Pool functionPool;
extern map functionMap;

typedef struct _Function {
    bool isExtern;
    bool isExternC;
    Arena* arena;
    char* name;
    char* mangledName;
    typeId returnType;
    typeId* paramTypes;
    char** paramNames;
    u64 numParams;
    Token* functionStart;
    TemplateDefinition* templateDefinition;
    typeId methodType;
    Scope scope;
} Function;

typedef struct _FunctionList {
    functionId* functions;
    u64 count;
    u64 capacity;
} FunctionList;

char* getFunctionMangledName(char* name, typeId* paramTypes, u64 numParams, Arena* arena);

Function* getFunctionFromId(functionId functionId);

FunctionList* getFunctionsByName(char* name);

functionId createFunction(
    char* name, typeId returnType, typeId* paramTypes, char** paramNames, u64 numParams, bool isExtern, bool isExternC, Token* functionStart, TemplateDefinition* templateDefinition, typeId methodType, Arena* arena);

functionId prototypeFunction(Token** tokens);

void implementFunction(functionId functionId);
