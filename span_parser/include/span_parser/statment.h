#pragma once


#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/expression.h"


typedef struct _SpanVariable SpanVariable;
typedef struct _SpanScope SpanScope;
typedef struct _SpanFunction SpanFunction;

typedef enum _SpanStatmentType {
    st_invalid = 0,
    st_expression,
    st_return,
    st_assign,
} SpanStatmentType;

typedef struct _SpanStatmentExpression {
    SpanExpression expression;
} SpanStatmentExpression;

typedef struct _SpanStatmentReturn {
    SpanExpression expression;
} SpanStatmentReturn;

typedef struct _Assignee {
    bool isVariableDeclaration;
    union {
        SpanExpression expression;
        SpanVariable* variable;
    };
} Assignee;

typedef struct _SpanAssignStatement {
    Assignee* assignees;
    u64 assigneesCount;
    SpanExpression value;
} SpanAssignStatement;

typedef struct _SpanStatement {
    SpanStatmentType type;
    SpanAst* ast;
    union {
        SpanStatmentExpression expression;
        SpanStatmentReturn return_;
        SpanAssignStatement assign;
    };
} SpanStatement;

SpanStatement createSpanStatement(SpanAst* ast, SpanScope* scope);
SpanStatement createSpanExpressionStatement(SpanAst* ast, SpanScope* scope);
SpanStatement createSpanReturnStatement(SpanAst* ast, SpanScope* scope);
SpanStatement createSpanAssignStatement(SpanAst* ast, SpanScope* scope);
SpanVariable* declareVariable(SpanAst* ast, SpanScope* scope);


void compileStatementExpression(SpanStatement* statement, SpanScope* scope, SpanFunction* function);
void compileReturn(SpanStatement* statement, SpanScope* scope, SpanFunction* function);
void compileAssignStatement(SpanStatement* statement, SpanScope* scope, SpanFunction* function);

bool compileStatement(SpanStatement* statement, SpanScope* scope, SpanFunction* function);
