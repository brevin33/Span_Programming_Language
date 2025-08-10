#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/files.h"
#include "span_parser/logging.h"
#include "span_parser/project.h"
#include "span_parser/tokens.h"
#include "span_parser/type.h"
#include "span_parser/utils.h"
#include "span_parser/ast.h"
#include "span_parser/function.h"
#include "span_parser/expression.h"
#include "span_parser/statment.h"
#include "span_parser/variable.h"
#include "span_parser/scope.h"
#include "span_parser/llvm.h"


#define BUFFER_SIZE 4096

SpanProject createSpanProject(Arena arena, char* path);

typedef struct _SpanContext {
    Arena arena;
    SpanProject* activeProject;
    u32 namespaceCounter;
    bool initialized;
    SpanTypeBase* baseTypes;
    u64 baseTypesCount;
    u64 baseTypesCapacity;
    SpanFunction* functions;
    u64 functionsCount;
    u64 functionsCapacity;
} SpanContext;

extern SpanContext context;
