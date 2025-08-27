#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"
#include "span_parser/scope.h"

typedef struct _SpanFunction {
    SpanTypeBase* functionType;
    SpanAst* ast;
    char* name;
    char* scrambledName;
    char** paramNames;
    SpanScope scope;
    bool isExtern;
    LLVMValueRef llvmFunc;
} SpanFunction;

SpanFunction* addFunction(SpanFunction* function);

SpanFunction** findFunctions(char* name, u32 namespace_, SpanFunction** buffer, u32* functionsCountOut);

SpanFunction* findFunction(char* name, u32 namespace_, SpanType* types, u32 typesCount, SpanAst* ast, bool logError);

SpanFunction* prototypeFunction(SpanAst* ast);

void implementFunction(SpanFunction* function);

void compileFunction(SpanFunction* function);

void compileRealMainFunction(SpanFunction* mainToCall);
