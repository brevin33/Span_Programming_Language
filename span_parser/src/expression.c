#include "span_parser/expression.h"
#include "span_parser.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"

void completeAddExpression(SpanExpression* expression, SpanScope* scope) {
    SpanExpression* lhs = expression->biop.lhs;
    SpanExpression* rhs = expression->biop.rhs;
    SpanType* lhsType = &lhs->type;
    SpanType* rhsType = &rhs->type;
    if (isNumbericType(lhsType) && isNumbericType(rhsType)) {
        bool worked = implicitlyCast(lhs, &rhs->type, false);
        if (worked) {
            expression->type = lhs->type;
            return;
        }
        worked = implicitlyCast(rhs, &lhs->type, false);
        if (worked) {
            expression->type = lhs->type;
            return;
        }
        massert(false, "cast should work here");
    }

    if (isTypeReference(lhsType) && isTypeReference(rhsType)) {
        SpanType derefTypelhs = dereferenceType(lhsType);
        SpanType derefTyperhs = dereferenceType(rhsType);
        makeCastExpression(lhs, &derefTypelhs);
        makeCastExpression(rhs, &derefTyperhs);
        completeAddExpression(expression, scope);
    } else if (isTypeReference(rhsType)) {
        SpanType derefType = dereferenceType(rhsType);
        makeCastExpression(rhs, &derefType);
        completeAddExpression(expression, scope);
    } else if (isTypeReference(lhsType)) {
        SpanType derefType = dereferenceType(lhsType);
        makeCastExpression(lhs, &derefType);
        completeAddExpression(expression, scope);
    }
}

SpanExpression createSpanBinaryExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_expr_biop, "should be a binary expression");
    SpanAstExprBiop* biop = &ast->exprBiop;
    SpanExpression expression;
    expression.ast = ast;
    expression.exprType = et_biop;
    expression.type = getInvalidType();
    expression.biop.lhs = allocArena(context.arena, sizeof(SpanExpression));
    *expression.biop.lhs = createSpanExpression(biop->lhs, scope);
    if (expression.biop.lhs->exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    expression.biop.rhs = allocArena(context.arena, sizeof(SpanExpression));
    *expression.biop.rhs = createSpanExpression(biop->rhs, scope);
    if (expression.biop.rhs->exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    expression.biop.op = biop->op;

    switch (biop->op) {
        case tt_add:
            completeAddExpression(&expression, scope);
            break;
        default:
            massert(false, "not a biop compiler error");
            break;
    }



    return expression;
}

SpanExpression createSpanFunctionCallExpression(SpanAst* ast, SpanScope* scope) {
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_functionCall;
    // parsing args
    SpanAst* args = ast->functionCall.args;
    massert(args->type == ast_call_paramerter_list, "should be a call paramerter list");
    expression.functionCall.args = allocArena(context.arena, sizeof(SpanExpression) * args->callParamerterList.paramsCount);
    expression.functionCall.argsCount = args->callParamerterList.paramsCount;

    for (u64 i = 0; i < args->callParamerterList.paramsCount; i++) {
        SpanAst* arg = &args->callParamerterList.params[i];
        SpanExpression argExpr = createSpanExpression(arg, scope);
        if (argExpr.exprType == et_invalid) {
            SpanExpression err = { 0 };
            return err;
        }
        expression.functionCall.args[i] = argExpr;
    }

    // finding function
    u32 matchingFunctionsCount = 0;
    SpanFunction* buffer[BUFFER_SIZE];
    SpanFunction** matchingFunctions = findFunctions(ast->functionCall.name, context.activeProject->namespace_, buffer, &matchingFunctionsCount);

    SpanFunction* match = NULL;

    // perfect match
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != expression.functionCall.argsCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = function->functionType->function.paramTypes[j];
            SpanType paramType2 = expression.functionCall.args[j].type;
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                paramTypesMatch = false;
                break;
            }
        }
        if (paramTypesMatch) {
            match = function;
            break;
        }
    }

    if (match != NULL) {
        expression.functionCall.function = match;
        expression.type = match->functionType->function.returnType;
        return expression;
    }

    SpanFunction* matchFunctions[BUFFER_SIZE];
    u64 matchFunctionsCount = 0;

    // dereference match
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != expression.functionCall.argsCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = function->functionType->function.paramTypes[j];
            SpanType paramType2 = expression.functionCall.args[j].type;
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                // dereference
                SpanType derefType = dereferenceType(&paramType2);
                if (isTypeEqual(&paramType1, &derefType) == false) {
                    paramTypesMatch = false;
                    break;
                }
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            match = matchFunctions[0];
            expression.functionCall.function = match;
            expression.type = match->functionType->function.returnType;
            // add dereference for all type doing that
            SpanFunction* function = match;
            for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
                SpanType paramType1 = function->functionType->function.paramTypes[j];
                SpanType paramType2 = expression.functionCall.args[j].type;
                if (isTypeEqual(&paramType1, &paramType2) == false) {
                    // dereference
                    makeCastExpression(&expression.functionCall.args[j], &paramType1);
                }
            }
            return expression;
        }
        logErrorAst(ast, "ambiguous function call");
        SpanExpression err = { 0 };
        return err;
    }

    // implicit cast match
    // TODO: implicit cast match
    massert(false, "not implemented");
}

SpanExpression createSpanExpression(SpanAst* ast, SpanScope* scope) {
    switch (ast->type) {
        case ast_expr_biop:
            return createSpanBinaryExpression(ast, scope);
        case ast_expr_word:
            return createSpanVariableExpression(ast, scope);
        case ast_expr_number_literal:
            return createSpanNumberLiteralExpression(ast, scope);
        case ast_function_call:
            return createSpanFunctionCallExpression(ast, scope);
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
        case et_biop:
            compileBinaryExpression(expression, scope, function);
            break;
        case et_functionCall:
            compileFunctionCallExpression(expression, scope, function);
            break;
        default:
            massert(false, "not implemented");
            break;
    }
}

void compileFunctionCallExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_functionCall, "should be a function call");
    LLVMValueRef valueBuffer[BUFFER_SIZE];
    for (u64 i = 0; i < expression->functionCall.argsCount; i++) {
        SpanExpression* arg = &expression->functionCall.args[i];
        compileExpression(arg, scope, function);
        valueBuffer[i] = arg->llvmValue;
    }
    SpanFunction* functionToCall = expression->functionCall.function;
    LLVMValueRef functionToCallLLVM = functionToCall->llvmFunc;
    LLVMTypeRef functionToCallType = functionToCall->functionType->llvmType;

    if (functionToCallLLVM == NULL) {
        // function doesn't exist so compile it
        compileFunction(functionToCall);
        functionToCallLLVM = functionToCall->llvmFunc;
    }

    expression->llvmValue = LLVMBuildCall2(context.builder, functionToCallType, functionToCallLLVM, valueBuffer, expression->functionCall.argsCount, "call");
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

void compileAddExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    compileExpression(expression->biop.lhs, scope, function);
    compileExpression(expression->biop.rhs, scope, function);
    SpanType* lhsType = &expression->biop.lhs->type;
    SpanType* rhsType = &expression->biop.rhs->type;
    LLVMValueRef lhsValue = expression->biop.lhs->llvmValue;
    LLVMValueRef rhsValue = expression->biop.rhs->llvmValue;
    if (isIntType(lhsType) && isIntType(rhsType)) {
        u64 lhsSize = lhsType->base->int_.size;
        u64 rhsSize = rhsType->base->int_.size;
        massert(lhsSize == rhsSize, "should be same size");
        expression->llvmValue = LLVMBuildAdd(context.builder, lhsValue, rhsValue, "addtmp");
        return;
    }
    if (isFloatType(lhsType) && isFloatType(rhsType)) {
        u64 lhsSize = lhsType->base->float_.size;
        u64 rhsSize = rhsType->base->float_.size;
        massert(lhsSize == rhsSize, "should be same size");
        expression->llvmValue = LLVMBuildFAdd(context.builder, lhsValue, rhsValue, "addtmp");
        return;
    }
    if (isUintType(lhsType) && isUintType(rhsType)) {
        u64 lhsSize = lhsType->base->uint.size;
        u64 rhsSize = rhsType->base->uint.size;
        massert(lhsSize == rhsSize, "should be same size");
        expression->llvmValue = LLVMBuildAdd(context.builder, lhsValue, rhsValue, "addtmp");
        return;
    }
}

void compileBinaryExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    switch (expression->biop.op) {
        case tt_add:
            compileAddExpression(expression, scope, function);
            break;
        default:
            massert(false, "not implemented");
            break;
    }
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

    if (typeIsReferenceOf(fromType, currentType)) {
        LLVMTypeRef llvmType = getLLVMType(currentType);
        LLVMValueRef val = LLVMBuildLoad2(context.builder, llvmType, fromExpr->llvmValue, "deref");
        expression->llvmValue = val;
        return;
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
    expression.type = getReferenceType(&variable->type);
    return expression;
}

void makeCastExpression(SpanExpression* expr, SpanType* type) {
    SpanExpression* oldExpr = allocArena(context.arena, sizeof(SpanExpression));
    *oldExpr = *expr;
    expr->exprType = et_cast;
    expr->type = *type;
    expr->cast.expression = oldExpr;
}

bool implicitlyCast(SpanExpression* expression, SpanType* type, bool logError) {
    SpanType* currentType = &expression->type;
    if (isTypeEqual(currentType, type)) return true;

    if (isTypeNumbericLiteral(currentType)) {
        if (isIntType(type)) {
            makeCastExpression(expression, type);
            return true;
        }
        if (isUintType(type)) {
            makeCastExpression(expression, type);
            return true;
        }
        if (isFloatType(type)) {
            makeCastExpression(expression, type);
            return true;
        }
    }

    if (isTypeReference(currentType)) {
        makeCastExpression(expression, type);
        implicitlyCast(expression, type, logError);
        currentType = &expression->type;
        if (isTypeEqual(currentType, type)) return true;
    }
    if (logError) {
        char buffer[BUFFER_SIZE];
        char* currentTypeName = getTypeName(currentType, buffer);
        char buffer2[BUFFER_SIZE];
        char* typeName = getTypeName(type, buffer2);
        logErrorAst(expression->ast, "cannot implicitly cast %s to %s", currentTypeName, typeName);
    }
    return false;
}
