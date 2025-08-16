#include "span_parser/expression.h"
#include "span_parser.h"
#include "span_parser/type.h"

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope) {
    switch (ast->type) {
        case ast_expr_biop:
            massert(false, "not implemented");
        case ast_expr_word:
            return createSpanVariableExpression(ast, scope);
        case ast_expr_number_literal:
            return createSpanNumberLiteralExpression(ast, scope);
        default:
            massert(false, "not implemented");
            break;
    }
    massert(false, "not implemented");
    SpanExpression err = { 0 };
    return err;
}

SpanExpression createSpanNumberLiteralExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_expr_number_literal, "should be a number literal");
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_number_literal;
    expression.numberLiteral.number = ast->numberLiteral.word;
    expression.type = getNumbericLiteralType();
    return expression;
}

void compileExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    switch (expression->exprType) {
        case et_number_literal:
            compileNumberLiteralExpression(expression, scope, function);
            break;
        case et_variable:
            compileVariableExpression(expression, scope, function);
            break;
        case et_cast:
            compileCastExpression(expression, scope, function);
            break;
        default:
            massert(false, "not implemented");
            break;
    }
}

void compileNumberLiteralExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_number_literal, "should be a number literal");
    // nothing to do
}

void compileVariableExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_variable, "should be a variable");

    LLVMValueRef variableValue = expression->variable.variable->llvmValue;
    expression->llvmValue = variableValue;
}

void compileCastExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_cast, "should be a cast");
    compileExpression(expression->cast.expression, scope, function);

    SpanType* currentType = &expression->type;
    SpanType* fromType = &expression->cast.expression->type;
    SpanExpression* fromExpr = expression->cast.expression;

    if (isTypeNumbericLiteral(fromType)) {
        if (isIntType(&expression->type)) {
            LLVMTypeRef intType = getLLVMType(&expression->type);
            char* number = fromExpr->numberLiteral.number;
            expression->llvmValue = LLVMConstIntOfString(intType, number, 10);
            return;
        }
        if (isUintType(&expression->type)) {
            LLVMTypeRef uintType = getLLVMType(&expression->type);
            char* number = fromExpr->numberLiteral.number;
            expression->llvmValue = LLVMConstIntOfString(uintType, number, 10);
            return;
        }
        if (isFloatType(&expression->type)) {
            LLVMTypeRef floatType = getLLVMType(&expression->type);
            char* number = fromExpr->numberLiteral.number;
            expression->llvmValue = LLVMConstRealOfString(floatType, number);
            return;
        }
        massert(false, "not implemented");
    }
    //TODO: more casts
    massert(false, "not implemented");
}

SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_expr_word, "should be a variable");
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_variable;
    SpanVariable* variable = getVariableFromScope(scope, ast->exprWord.word);
    if (variable == NULL) {
        logErrorAst(ast, "variable does not exist");
        SpanExpression err = { 0 };
        return err;
    }
    expression.variable.variable = variable;
    expression.type = variable->type;
    return expression;
}

SpanExpression createCastExpression(SpanExpression* expr, SpanType* type) {
    SpanExpression expression;
    expression.ast = expr->ast;
    expression.exprType = et_cast;
    expression.type = *type;
    expression.cast.expression = expr;
    return expression;
}

void implicitlyCast(SpanExpression* expression, SpanType* type) {
    SpanType* currentType = &expression->type;
    if (isTypeEqual(currentType, type)) return;

    if (isTypeNumbericLiteral(currentType)) {
        if (isIntType(type)) {
            *expression = createCastExpression(expression, type);
            return;
        }
        if (isUintType(type)) {
            *expression = createCastExpression(expression, type);
            return;
        }
        if (isFloatType(type)) {
            *expression = createCastExpression(expression, type);
            return;
        }
    }

    while (isTypeReference(currentType)) {
        *expression = createCastExpression(expression, type);
        implicitlyCast(expression, type);
    }
    char buffer[BUFFER_SIZE];
    char* currentTypeName = getTypeName(currentType, buffer);
    char buffer2[BUFFER_SIZE];
    char* typeName = getTypeName(type, buffer2);
    logErrorAst(expression->ast, "cannot implicitly cast %s to %s", currentTypeName, typeName);
}
