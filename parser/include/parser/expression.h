#pragma once

#include "parser/nice_ints.h"
#include "parser/scope.h"
#include "parser/tokens.h"
#include "parser/type.h"

typedef struct _Expression Expression;

typedef enum _ExpressionKind {
    ek_invalid = 0,
    ek_number,
    ek_struct,
    ek_struct_parcial,
    ek_variable,
    ek_type,
    ek_biop,
    ek_deref,
    ek_implicit_cast,
} ExpressionKind;

typedef struct _BiopData {
    Expression* left;
    Expression* right;
    OurTokenType operator;
    functionId functionId;  // BAD_ID for intrinsic like adding two ints
} BiopData;

typedef struct _Expression {
    Token* token;
    u64 tokenCount;
    ExpressionKind type;
    typeId tid;
    union {
        void* data;
        char* number;
        char* variable;
        BiopData* biopData;
        Expression* implicitCast;
        Expression* deref;
    };
} Expression;

Expression createExpressionFromTokens(Token** tokens, OurTokenType* delimiters, u64 numDelimiters, functionId functionId, Scope* scope);

void expressionAcutalType(Expression* expression, Scope* scope);

Expression boolCast(Expression* expression, Scope* scope, functionId functionId);

Expression implicitCast(Expression* expression, typeId type, Scope* scope, functionId functionId);
