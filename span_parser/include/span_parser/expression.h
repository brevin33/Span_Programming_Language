#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"

typedef struct _SpanVariable SpanVariable;
typedef struct _SpanExpression SpanExpression;
typedef struct _SpanScope SpanScope;

typedef enum _SpanExpressionType {
    et_invalid = 0,
    et_number_literal,
    et_variable,
    et_biop,
} SpanExpressionType;

typedef struct _SpanExpressionNumberLiteral {
    char* number;
} SpanExpressionNumberLiteral;

typedef struct _SpanExpressionVariable {
    SpanVariable* variable;
} SpanExpressionVariable;

typedef struct _SpanExpression {
    SpanAst* ast;
    SpanExpressionType exprType;
    SpanType type;
    union {
        SpanExpressionNumberLiteral numberLiteral;
        SpanExpressionVariable variable;
    };
} SpanExpression;

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanNumberLiteralExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope);
