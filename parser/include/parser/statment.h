#pragma once

#include "parser/nice_ints.h"
#include "parser/tokens.h"

typedef struct _Scope Scope;
typedef u64 functionId;

typedef enum _StatementKind {
    sk_invalid = 0,
    sk_assignment,
    sk_expression,
    sk_return,
    sk_if,
    sk_while,
} StatementKind;

typedef struct _Statement {
    StatementKind kind;
    union {
        void* data;
    };
} Statement;

Statement createStatmentFromTokens(Token** tokens, functionId functionId, Scope* scope);

Statement createExpressionStatement(Token** tokens, functionId functionId, Scope* scope);

Statement createAssignmentStatement(Token** tokens, functionId functionId, Scope* scope);
