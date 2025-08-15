#include "span_parser.h"

SpanStatement createSpanStatement(SpanAst* ast, SpanScope* scope) {
    switch (ast->type) {
        case ast_return:
            return createSpanReturnStatement(ast, scope);
        CASE_AST_EXPR:
            return createSpanExpressionStatement(ast, scope);
        case ast_assignment:
            return createSpanAssignStatement(ast, scope);
        default:
            massert(false, "not implemented");
            break;
    }
    massert(false, "not implemented");
    SpanStatement err = { 0 };
    return err;
}
SpanStatement createSpanExpressionStatement(SpanAst* ast, SpanScope* scope) {
    SpanStatement statement = { 0 };
    massert(AstIsExpression(ast), "should be an expression");
    statement.type = st_expression;
    statement.ast = ast;
    statement.expression.expression = createSpanExpression(ast, scope);
    return statement;
}

SpanStatement createSpanReturnStatement(SpanAst* ast, SpanScope* scope) {
    SpanStatement statement = { 0 };
    massert(ast->type == ast_return, "should be a return");
    SpanAst* expr = ast->return_.value;
    statement.type = st_return;
    statement.ast = ast;
    statement.return_.expression = createSpanExpression(expr, scope);
    return statement;
}

SpanStatement createSpanAssignStatement(SpanAst* ast, SpanScope* scope) {
    SpanStatement statement = { 0 };
    massert(ast->type == ast_assignment, "should be an assignment");
    SpanAst* assignees = ast->assignment.assignees;
    u64 assigneesCount = ast->assignment.assigneesCount;
    statement.type = st_assign;
    statement.ast = ast;
    statement.assign.assignees = allocArena(context.arena, sizeof(Assignee) * assigneesCount);
    statement.assign.assigneesCount = assigneesCount;
    for (u64 i = 0; i < assigneesCount; i++) {
        SpanAst* assignee = &assignees[i];
        Assignee a;
        if (assignee->type == ast_variable_declaration) {
            a.isVariableDeclaration = true;
            a.variable = declareVariable(assignee, scope);
        } else {
            a.isVariableDeclaration = false;
            a.expression = createSpanExpression(assignee, scope);
        }
        statement.assign.assignees[i] = a;
    }
    SpanAst* value = ast->assignment.value;
    statement.assign.value = createSpanExpression(value, scope);
    if (statement.assign.assigneesCount == 1) {
        implicitlyCast(&statement.assign.value, &statement.assign.assignees[0].variable->type);
        return statement;
    }
    massert(false, "not implemented");
    return statement;
}

SpanVariable* declareVariable(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_variable_declaration, "should be a variable declaration");
    char* name = ast->variableDeclaration.name;
    SpanType type = getType(ast->variableDeclaration.type);
    return addVariableToScope(scope, name, type, ast);
}
