#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"

typedef struct _SpanVariable SpanVariable;
typedef struct _SpanExpression SpanExpression;
typedef struct _SpanScope SpanScope;
typedef struct _SpanFunction SpanFunction;

typedef enum _SpanExpressionType {
    et_invalid = 0,
    et_number_literal,
    et_variable,
    et_biop,
    et_cast,
} SpanExpressionType;

typedef struct _SpanExpressionNumberLiteral {
    char* number;
} SpanExpressionNumberLiteral;

typedef struct _SpanExpressionVariable {
    SpanVariable* variable;
} SpanExpressionVariable;

typedef struct _SpanExpressionCast {
    SpanExpression* expression;
} SpanExpressionCast;

typedef struct _SpanExpression {
    SpanAst* ast;
    SpanExpressionType exprType;
    SpanType type;
    union {
        SpanExpressionNumberLiteral numberLiteral;
        SpanExpressionVariable variable;
        SpanExpressionCast cast;
    };
    LLVMValueRef llvmValue;
} SpanExpression;

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanNumberLiteralExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createCastExpression(SpanExpression* expr, SpanType* type);
void implicitlyCast(SpanExpression* expression, SpanType* type);

void compileExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);

void compileNumberLiteralExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileVariableExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileCastExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
