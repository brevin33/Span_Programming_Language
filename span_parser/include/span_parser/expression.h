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
    et_functionCall,
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

typedef struct _SpanExpressionFunctionCall {
    SpanFunction* function;
    SpanExpression* args;
    u64 argsCount;
} SpanExpressionFunctionCall;

typedef struct _SpanExpressionBiop {
    SpanExpression* lhs;
    SpanExpression* rhs;
    OurTokenType op;
} SpanExpressionBiop;

typedef struct _SpanExpression {
    SpanAst* ast;
    SpanExpressionType exprType;
    SpanType type;
    union {
        SpanExpressionNumberLiteral numberLiteral;
        SpanExpressionVariable variable;
        SpanExpressionCast cast;
        SpanExpressionBiop biop;
        SpanExpressionFunctionCall functionCall;
    };
    LLVMValueRef llvmValue;
} SpanExpression;

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanNumberLiteralExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanBinaryExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanFunctionCallExpression(SpanAst* ast, SpanScope* scope);
void makeCastExpression(SpanExpression* expr, SpanType* type);

bool implicitlyCast(SpanExpression* expression, SpanType* type, bool logError);

void completeAddExpression(SpanExpression* expression, SpanScope* scope);

void compileExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);

void compileNumberLiteralExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileVariableExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileCastExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileBinaryExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileFunctionCallExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);

void compileAddExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
