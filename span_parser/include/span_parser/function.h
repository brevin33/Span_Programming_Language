#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"
#include "span_parser/scope.h"

typedef struct _SpanFunctionInstance SpanFunctionInstance;

typedef struct _SpanFunction {
    SpanTypeBase* functionType;
    SpanAst* ast;
    char* name;
    char* scrambledName;
    char** paramNames;
    SpanScope scope;
    bool isExtern;
    SpanFunctionInstance* functionInstances;
    u64 functionInstancesCount;
    u64 functionInstancesCapacity;
} SpanFunction;

typedef struct _SpanFunctionInstance {
    SpanFunction* function;
    SpanTypeSubstitution* substitutions;
    u64 substitutionsCount;
    LLVMValueRef llvmFunc;
} SpanFunctionInstance;

SpanFunction* addFunction(SpanFunction* function);

SpanFunction** findFunctions(char* name, u32 namespace_, SpanFunction** buffer, u32* functionsCountOut);

SpanFunctionInstance* findFunction(char* name, u32 namespace_, SpanType* types, u32 typesCount, SpanAst* ast, bool logError);

SpanFunction* prototypeFunction(SpanAst* ast);

SpanFunctionInstance* findCastFunction(SpanType* valueType, SpanType* castType, u32 namespace_, bool logError, SpanAst* ast);

SpanFunctionInstance* getIfFunctionAlreadyHasInstance(SpanFunction* function, SpanFunctionInstance* instance);

bool SpanFunctionInstanceIsEqual(SpanFunctionInstance* instance1, SpanFunctionInstance* instance2);

void implementFunction(SpanFunction* function);

void compileFunction(SpanFunctionInstance* function);

void compileRealMainFunction(SpanFunction* mainToCall);
