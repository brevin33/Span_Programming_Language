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

SpanProject createSpanProject(Arena arena, char* path);

typedef struct _SpanContext {
    Arena arena;
    SpanProject* activeProject;
    u32 namespaceCounter;
    bool initialized;
} SpanContext;

extern SpanContext context;
