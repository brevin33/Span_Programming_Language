#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"

typedef struct _SpanFunction {
    SpanTypeBase* functionType;
    char* name;
    char* scrambledName;
} SpanFunction;

SpanFunction* addFunction(SpanFunction* function);

SpanFunction* findFunction(char* name, u32 namespace);

SpanFunction* implementFunction(SpanAst* ast);
