#include "span_parser.h"

SpanStatement createSpanStatement(SpanAst* ast, SpanScope* scope, SpanFunction* function) {
    switch (ast->type) {
        case ast_return:
            return createSpanReturnStatement(ast, scope, function);
        CASE_AST_EXPR:
            return createSpanExpressionStatement(ast, scope);
        case ast_assignment:
            return createSpanAssignStatement(ast, scope);
        case ast_scope:
            return createSpanScopeStatement(ast, scope, function);
        case ast_end_statement:
            return createSpanEndStatement(ast, scope, function);
        default:
            massert(false, "not implemented");
            break;
    }
    massert(false, "not implemented");
    SpanStatement err = { 0 };
    return err;
}

SpanStatement createSpanEndStatement(SpanAst* ast, SpanScope* scope, SpanFunction* function) {
    SpanStatement statement = { 0 };
    statement.type = st_end_statement;
    statement.ast = ast;
    return statement;
}

SpanStatement createSpanExpressionStatement(SpanAst* ast, SpanScope* scope) {
    SpanStatement statement = { 0 };
    massert(AstIsExpression(ast), "should be an expression");
    statement.type = st_expression;
    statement.ast = ast;
    statement.expression.expression = createSpanExpression(ast, scope);
    return statement;
}

SpanStatement createSpanScopeStatement(SpanAst* ast, SpanScope* scope, SpanFunction* function) {
    massert(ast->type == ast_scope, "should be a scope");
    SpanStatement statement = { 0 };
    statement.type = st_scope;
    statement.ast = ast;
    statement.scope.scope = allocArena(context.arena, sizeof(SpanScope));
    *statement.scope.scope = createSpanScope(ast, scope, function);
    addStatmentsToScope(ast, statement.scope.scope, function);
    return statement;
}

bool compileStatement(SpanStatement* statement, SpanScope* scope, SpanFunction* function) {
    switch (statement->type) {
        case st_expression:
            compileStatementExpression(statement, scope, function);
            return false;
        case st_return:
            compileReturn(statement, scope, function);
            return true;
        case st_assign:
            compileAssignStatement(statement, scope, function);
            return false;
        case st_end_statement:
            return false;
        default:
            massert(false, "not implemented");
            break;
    }
}

void compileStatementExpression(SpanStatement* statement, SpanScope* scope, SpanFunction* function) {
    compileExpression(&statement->expression.expression, scope, function);
}
void compileReturn(SpanStatement* statement, SpanScope* scope, SpanFunction* function) {
    compileExpression(&statement->return_.expression, scope, function);
    LLVMBuildRet(context.builder, statement->return_.expression.llvmValue);
}
void compileAssignStatement(SpanStatement* statement, SpanScope* scope, SpanFunction* function) {
    LLVMValueRef assigneesLLVMValue[BUFFER_SIZE];
    for (u64 i = 0; i < statement->assign.assigneesCount; i++) {
        Assignee* assignee = &statement->assign.assignees[i];
        if (!assignee->isVariableDeclaration) {
            SpanType* type = &assignee->expression.type;
            massert(isTypeReference(type), "should be a reference");
            compileExpression(&assignee->expression, scope, function);
            assigneesLLVMValue[i] = assignee->expression.llvmValue;
        } else {
            SpanVariable* variable = assignee->variable;
            assigneesLLVMValue[i] = variable->llvmValue;
        }
    }

    if (statement->assign.value.exprType == et_none) return;
    compileExpression(&statement->assign.value, scope, function);
    LLVMValueRef value = statement->assign.value.llvmValue;
    // set assignes to value
    if (statement->assign.assigneesCount == 1) {
        Assignee* assignee = &statement->assign.assignees[0];
        if (assignee->isVariableDeclaration && assignee->variable->isReference) {
            SpanVariable* variable = assignee->variable;
            variable->llvmValue = value;
        } else {
            LLVMBuildStore(context.builder, value, assigneesLLVMValue[0]);
        }
    } else
        massert(false, "not implemented");
}

SpanStatement createSpanReturnStatement(SpanAst* ast, SpanScope* scope, SpanFunction* function) {
    SpanStatement statement = { 0 };

    massert(ast->type == ast_return, "should be a return");
    SpanAst* expr = ast->return_.value;
    statement.type = st_return;
    statement.ast = ast;
    statement.return_.expression = createSpanExpression(expr, scope);
    if (statement.return_.expression.exprType == et_invalid) {
        SpanStatement err = { 0 };
        return err;
    }
    SpanType* returnType = &function->functionType->function.returnType;
    implicitlyCast(&statement.return_.expression, returnType, true);
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
            if (isTypeInvalid(&a.variable->type)) {
                SpanStatement err = { 0 };
                return err;
            }
        } else {
            a.isVariableDeclaration = false;
            a.expression = createSpanExpression(assignee, scope);
            if (a.expression.exprType == et_invalid) {
                SpanStatement err = { 0 };
                return err;
            }
            SpanType* type = &a.expression.type;
            if (!isTypeReference(type)) {
                logErrorAst(assignee, "can't assign to a non-reference");
            }
        }
        statement.assign.assignees[i] = a;
    }
    SpanAst* value = ast->assignment.value;
    if (value == NULL) {
        statement.assign.value = createSpanNoneExpression();
        return statement;
    }
    statement.assign.value = createSpanExpression(value, scope);
    if (statement.assign.value.exprType == et_invalid) {
        SpanStatement err = { 0 };
        return err;
    }
    if (statement.assign.assigneesCount == 1) {
        Assignee* assignee = &statement.assign.assignees[0];
        SpanType assigneeType;
        if (assignee->isVariableDeclaration) {
            SpanVariable* variable = assignee->variable;
            if (variable->isReference) {
                assigneeType = getReferenceType(&variable->type);
            } else {
                assigneeType = variable->type;
            }
        } else {
            assigneeType = assignee->expression.type;
            assigneeType = dereferenceType(&assigneeType);
        }
        implicitlyCast(&statement.assign.value, &assigneeType, true);
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
