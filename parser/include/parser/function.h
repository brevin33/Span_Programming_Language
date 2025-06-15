#pragma once
#include "parser/expression.h"
#include "parser/map.h"
#include "parser/nice_ints.h"
#include "parser/scope.h"
#include "parser/type.h"
#include "parser/tokens.h"

struct Project;
typedef struct _Project Project;

struct _Scope;
typedef struct _Scope Scope;

typedef enum _FunctionType : u8 {
    ft_normal,
    ft_extern,
    ft_extern_c,
} FunctionType;

typedef struct _Function {
    typeId returnType;
    FunctionType type;
    bool variadic;
    char* name;
    char* mangledName;
    typeId* parameters;
    char** parameterNames;
    u64 parameterCount;
    Token* startToken;
    Scope* scope;
    //TODO: add body once we have stuff for that
} Function;

typedef u64 functionId;

typedef struct _FunctionsWithSameName {
    functionId* functions;
    u64 count;
    u64 capacity;
} FunctionsWithSameName;

extern Function* gFunctions;
extern u64 gFunctionCount;
extern u64 gFunctionCapacity;
extern map gNameToFunctionsWithSameName;

functionId createFunctionFromTokens(Token* token, Project* project);

functionId createFunction(typeId returnType, char* name, typeId* parameters, char** parameterNames, u64 parameterCount, Token* startToken, Project* project, bool variadic, FunctionType type);

void implementFunction(functionId funcId, Project* project);


char* getMangledName(Function* function, Project* project);

typeId getBiopTypeResult(BiopExpresstion* biop, Project* project);

bool canImplCast(typeId from, typeId to, Project* project);

functionId getFunctionForBiop(BiopExpresstion* biop, Project* project);

Function* getFunctionFromId(functionId functionId);

functionId getFunctionFromNameAndParmeters(const char* name, typeId* parameters, u64 parameterCount, Project* project, Token* tokenForError);
