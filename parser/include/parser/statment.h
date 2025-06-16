#pragma once
#include "parser/expression.h"
#include "parser/scope.h"

typedef enum _StatmentType : u8 {
    st_error,
    st_expression,
    st_if,
    st_else,
    st_while,
    st_for,
    st_return,
    st_break,
    st_continue,
    st_switch,
    st_scope,
    st_assignment,
} StatmentType;

typedef enum _opType {
    op_add,
    op_sub,
    op_mul,
    op_div,
    op_mod,
    op_and,
    op_or,
    op_xor,
    op_to,
    op_as,
    op_eq,
    op_bit_and,
    op_bit_or,
    op_shift_left,
    op_shift_right,
    op_index,
} opType;

int opPrecedence(opType op);

typedef enum _AssignmentLeftSideElementType {
    ase_error,
    ase_variable,
    ase_declaration,
} AssignmentLeftSideElementType;


typedef struct _AssignmentLeftSideElement {
    AssignmentLeftSideElementType type;
    Variable variable;
} AssignmentLeftSideElement;

typedef struct _Assignment {
    AssignmentLeftSideElement* leftSideElements;
    u64 numLeftSideElements;
    Expresstion* value;
} Assignment;

typedef struct _Statment {
    StatmentType type;
    union {
        Scope* scope;
        Expresstion* expression;
        Assignment* assignment;
        Expresstion* returnValue;
        Expresstion* ifCondition;
        u64 breakLevel;
        u64 continueLevel;
    };
} Statment;


struct _Function;
typedef struct _Function Function;

Statment createStatmentFromTokens(Token** tokens, functionId function, Scope* scope, Project* project);

Assignment createAssignmentFromTokens(Token** tokens, functionId function, Scope* scope, Project* project);
