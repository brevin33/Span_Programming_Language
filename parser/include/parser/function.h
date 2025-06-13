#pragma once
#include "parser/expression.h"
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
    typeId* parameters;
    char** parameterNames;
    u64 parameterCount;
    Token* startToken;
    Scope* scope;
    //TODO: add body once we have stuff for that
} Function;


Function* createFunctionFromTokens(Token* token, Project* project);

Function* createFunction(typeId returnType, char* name, typeId* parameters, char** parameterNames, u64 parameterCount, Token* startToken, Project* project, bool variadic, FunctionType type);

void implementFunction(Function* function, Project* project);

typeId getBiopTypeResult(BiopExpresstion* biop, Project* project);

bool canStaticCast(typeId from, typeId to, Project* project);

Function* getFunctionForBiop(BiopExpresstion* biop, Project* project);

Function* getFunctionFromNameAndParmeters(const char* name, typeId* parameters, u64 parameterCount, Project* project, Token* tokenForError);
