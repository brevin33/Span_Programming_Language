#pragma once

#include "parser/nice_ints.h"
#include "parser/tokens.h"

typedef struct _Expression Expression;

typedef struct _Scope Scope;
typedef u64 functionId;
typedef struct _Statement Statement;
typedef struct _Scope Scope;

typedef enum _StatementKind {
    sk_invalid = 0,
    sk_assignment,
    sk_expression,
    sk_return,
    sk_if,
    sk_while,
    sk_break,
} StatementKind;

typedef struct _AssignmentStatementData {
    Expression* assignee;
    u64 numAssignee;
    Expression* values;
    u64 numValues;
} AssignmentStatementData;

typedef struct _IfStatementData {
    Expression* condition;
    Scope* body;
    Scope* elseBody;
} IfStatementData;

typedef struct _WhileStatementData {
    Expression* condition;
    Scope* body;
} WhileStatementData;


typedef struct _Statement {
    StatementKind kind;
    Token* tokens;
    u64 tokenCount;
    union {
        void* data;
        Expression* expressionData;
        AssignmentStatementData* assignmentData;
        IfStatementData* ifData;
        WhileStatementData* whileData;
        u64 breakAmount;
    };
} Statement;

Statement createStatmentFromTokens(Token** tokens, functionId functionId, Scope* scope);

Statement createExpressionStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createIfStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createBreakStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createWhileStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createReturnStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createAssignmentStatement(Token** tokens, functionId functionId, Scope* scope);
