#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/statment.h"
#include "span_parser/type.h"

typedef struct _SpanVariable {
    char* name;
    SpanType type;
    SpanAst* ast;
} SpanVariable;

typedef struct _SpanScope SpanScope;
typedef struct _SpanScope {
    SpanAst* ast;
    SpanStatement* statments;
    u64 statmentsCount;
    SpanScope* parent;
    SpanVariable* variables;
    u64 variablesCount;
    u64 variablesCapacity;
} SpanScope;

SpanScope createSpanScope(SpanAst* ast, SpanScope* parent);
SpanVariable* addVariableToScope(SpanScope* scope, char* name, SpanType type, SpanAst* ast);
SpanVariable* getVariableFromScope(SpanScope* scope, char* name);
void compileScope(SpanScope* scope);
