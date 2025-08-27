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
    et_none,
    et_number_literal,
    et_variable,
    et_biop,
    et_cast,
    et_functionCall,
    et_struct_access,
    et_get_ptr,
    et_get_val,
} SpanExpressionType;

typedef struct _SpanExpressionNumberLiteral {
    char* number;
} SpanExpressionNumberLiteral;

typedef struct _SpanExpressionGetPtr {
    SpanExpression* value;
} SpanExpressionGetPtr;

typedef struct _SpanExpressionGetVal {
    SpanExpression* value;
} SpanExpressionGetVal;

typedef struct _SpanExpressionStructAccess {
    SpanExpression* value;
    u64 memberIndex;
} SpanExpressionStructAccess;

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
        SpanExpressionStructAccess structAccess;
        SpanExpressionGetPtr getPtr;
        SpanExpressionGetVal getVal;
    };
    LLVMValueRef llvmValue;
} SpanExpression;

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanNumberLiteralExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanBinaryExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanFunctionCallExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanStructAccessExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value);
SpanExpression createMemberAccessExpression(SpanAst* ast, SpanScope* scope);
SpanExpression createSpanNoneExpression();
SpanExpression createSpanGetPtrExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value);
SpanExpression createSpanGetValExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value);
SpanExpression createSpanMethodCallExpression(SpanAst* ast, SpanScope* scope);


void makeCastExpression(SpanExpression* expr, SpanType* type);

// return value is number of dereferences to do the cast
// -1 can't be cast
// 0 is no need to do a cast
// 1 is cast
// 2 is one dereference
// 3 is two dereferences
// ... etc
int canImplicitlyCast(SpanType* fromType, SpanType* toType, bool logError, SpanAst* ast);
bool implicitlyCast(SpanExpression* expression, SpanType* type, bool logError);

void completeAddExpression(SpanExpression* expression, SpanScope* scope);

void compileExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);

void compileNumberLiteralExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileVariableExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileCastExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileBinaryExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileFunctionCallExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileStructAccessExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileGetPtrExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
void compileGetValExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);



void compileAddExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function);
