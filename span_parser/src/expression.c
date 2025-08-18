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
        return;
    } else if (isTypeReference(rhsType)) {
        SpanType derefType = dereferenceType(rhsType);
        makeCastExpression(rhs, &derefType);
        completeAddExpression(expression, scope);
        return;
    } else if (isTypeReference(lhsType)) {
        SpanType derefType = dereferenceType(lhsType);
        makeCastExpression(lhs, &derefType);
        completeAddExpression(expression, scope);
        return;
    }

    massert(false, "not implemented");
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
    SpanFunction* matchFunctions[BUFFER_SIZE];
    u64 matchFunctionsCount = 0;
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
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            match = matchFunctions[0];
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
                for (u64 i = 0; i < matchFunctionsCount; i++) {
                    SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
                    logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
                }
                SpanExpression err = { 0 };
                return err;
            }
            expression.functionCall.function = match;
            expression.type = match->functionType->function.returnType;
            return expression;
        }
        logErrorAst(ast, "ambiguous function call");
        for (u64 i = 0; i < matchFunctionsCount; i++) {
            SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
            logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
        }
        SpanExpression err = { 0 };
        return err;
    }


    // dereference match
    matchFunctionsCount = 0;
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
                if (!isTypeReference(&paramType2)) {
                    paramTypesMatch = false;
                    break;
                }
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
        for (u64 i = 0; i < matchFunctionsCount; i++) {
            SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
            logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
        }
        SpanExpression err = { 0 };
        return err;
    }



    // make a copy of args so we can undo implicit casts
    SpanExpression argsCopyBuffer[BUFFER_SIZE];
    memcpy(argsCopyBuffer, expression.functionCall.args, sizeof(SpanExpression) * expression.functionCall.argsCount);

    // implicit cast match
    matchFunctionsCount = 0;
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
                // implicit cast
                bool worked = implicitlyCast(&expression.functionCall.args[j], &paramType1, false);
                if (!worked) {
                    paramTypesMatch = false;
                    break;
                }
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
        //undo implicit casts
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanExpression* arg = &expression.functionCall.args[j];
            *arg = argsCopyBuffer[j];
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            match = matchFunctions[0];
            expression.functionCall.function = match;
            expression.type = match->functionType->function.returnType;
            SpanFunction* function = match;
            for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
                implicitlyCast(&expression.functionCall.args[j], &function->functionType->function.paramTypes[j], false);
            }
            return expression;
        }
        logErrorAst(ast, "ambiguous function call");
        for (u64 i = 0; i < matchFunctionsCount; i++) {
            SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
            logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
        }
        SpanExpression err = { 0 };
        return err;
    }

    char errorMessage[BUFFER_SIZE];
    sprintf(errorMessage, "could not find function called %s that works with arguments of:", ast->functionCall.name);
    for (u64 i = 0; i < expression.functionCall.argsCount; i++) {
        char typeNameBuffer[BUFFER_SIZE];
        char* argTypeName = getTypeName(&expression.functionCall.args[i].type, typeNameBuffer);
        if (i == 0) {
            sprintf(errorMessage, "%s %s", errorMessage, argTypeName);
        } else {
            sprintf(errorMessage, "%s, %s", errorMessage, argTypeName);
        }
    }
    logErrorAst(ast, errorMessage);
    SpanExpression err = { 0 };
    return err;
}

SpanExpression createSpanStructAccessExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    massert(ast->type == ast_member_access, "should be a member access");
    SpanExpression expr = { 0 };
    expr.ast = ast;
    expr.exprType = et_struct_access;
    expr.structAccess.value = allocArena(context.arena, sizeof(SpanExpression));
    *expr.structAccess.value = *value;

    char* memberName = ast->memberAccess.memberName;
    bool isNumber = stringIsUint(memberName);
    SpanType* type = &value->type;
    SpanTypeBase* baseType = type->base;
    SpanTypeStruct* structType = &baseType->struct_;
    massert(baseType->type == t_struct, "should be a struct");
    massert(isTypeReference(type) || isTypePointer(type) || isTypeStruct(type), "should be a reference or pointer or struct");
    if (isNumber) {
        u64 i = stringToUint(memberName);
        if (i >= structType->fieldsCount) {
            logErrorAst(ast, "struct does not have member at index %u", i);
            SpanExpression err = { 0 };
            return err;
        }
        expr.structAccess.memberIndex = i;
        if (isTypeReference(type) || isTypePointer(type)) {
            // get reference since we are ptr or ref
            expr.type = getReferenceType(&structType->fields[i]);
        } else {
            // can't get reference as we have no pointer
            expr.type = structType->fields[i];
        }
        return expr;
    } else {
        for (u64 i = 0; i < structType->fieldsCount; i++) {
            char* fieldName = structType->fieldsNames[i];
            SpanType* fieldType = &structType->fields[i];
            if (strcmp(fieldName, memberName) == 0) {
                expr.structAccess.memberIndex = i;
                if (isTypeReference(&value->type) || isTypePointer(&value->type)) {
                    // get reference since we are ptr or ref
                    expr.type = getReferenceType(fieldType);
                } else {
                    // can't get reference as we have no pointer
                    expr.type = *fieldType;
                }
                return expr;
            }
        }
        logErrorAst(ast, "struct does not have member named %s", memberName);
        SpanExpression err = { 0 };
        return err;
    }
}

SpanExpression createSpanNoneExpression() {
    SpanExpression expression = { 0 };
    expression.exprType = et_none;
    expression.type = getInvalidType();
    expression.llvmValue = NULL;
    expression.ast = NULL;
    return expression;
}

SpanExpression createMemberAccessExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_member_access, "should be a member access");
    SpanExpression value = createSpanExpression(ast->memberAccess.value, scope);
    if (value.exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    char* accessedMemberName = ast->memberAccess.memberName;
    // intrinsic
    if (isTypeReference(&value.type) && strcmp(accessedMemberName, "ptr") == 0) {
        return createSpanGetPtrExpression(ast, scope, &value);
    }
    bool wordIsVal = strcmp(accessedMemberName, "val") == 0;
    if (wordIsVal) {
        if (isTypePointer(&value.type)) {
            return createSpanGetValExpression(ast, scope, &value);
        }
        if (isTypeReference(&value.type)) {
            SpanType derefType = dereferenceType(&value.type);
            if (isTypePointer(&derefType)) {
                return createSpanGetValExpression(ast, scope, &value);
            }
        }
    }


    // struct
    SpanTypeBase* baseType = value.type.base;
    if (baseType->type == t_struct) {
        return createSpanStructAccessExpression(ast, scope, &value);
    }
    char buffer[BUFFER_SIZE];
    char* typeName = getTypeName(&value.type, buffer);
    logErrorAst(ast, "can't access member %s of type: %s", accessedMemberName, typeName);
    SpanExpression err = { 0 };
    return err;
}

SpanExpression createSpanGetPtrExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    massert(isTypeReference(&value->type), "should be a reference");
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_get_ptr;
    expression.getPtr.value = allocArena(context.arena, sizeof(SpanExpression));
    *expression.getPtr.value = *value;
    SpanType derefType = dereferenceType(&value->type);
    expression.type = getPointerType(&derefType);
    return expression;
}

SpanExpression createSpanGetValExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    if (isTypePointer(&value->type)) {
        massert(isTypePointer(&value->type), "should be a pointer");
        SpanExpression expression = { 0 };
        expression.ast = ast;
        expression.exprType = et_get_val;
        expression.getVal.value = allocArena(context.arena, sizeof(SpanExpression));
        *expression.getVal.value = *value;
        expression.type = dereferenceType(&value->type);
        return expression;
    } else if (isTypeReference(&value->type)) {
        SpanType derefType = dereferenceType(&value->type);
        massert(isTypePointer(&derefType), "should be a pointer");
        makeCastExpression(value, &derefType);
        SpanExpression expression = { 0 };
        expression.ast = ast;
        expression.exprType = et_get_val;
        expression.getVal.value = allocArena(context.arena, sizeof(SpanExpression));
        *expression.getVal.value = *value;
        expression.type = dereferenceType(&value->type);
        return expression;
    }
    massert(false, "should be a pointer or reference to ptr");
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
        case ast_member_access:
            return createMemberAccessExpression(ast, scope);
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
        case et_struct_access:
            compileStructAccessExpression(expression, scope, function);
            break;
        case et_none:
            break;
        case et_get_ptr:
            compileGetPtrExpression(expression, scope, function);
            break;
        case et_get_val:
            compileGetValExpression(expression, scope, function);
            break;
        default:
            massert(false, "not implemented");
            break;
    }
}

void compileGetPtrExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_get_ptr, "should be a get ptr");
    compileExpression(expression->getPtr.value, scope, function);
    expression->llvmValue = expression->getPtr.value->llvmValue;
}

void compileGetValExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_get_val, "should be a get val");
    compileExpression(expression->getPtr.value, scope, function);
    expression->llvmValue = expression->getPtr.value->llvmValue;
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

void compileStructAccessExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    compileExpression(expression->structAccess.value, scope, function);
    u64 memberIndex = expression->structAccess.memberIndex;
    LLVMValueRef structValue = expression->structAccess.value->llvmValue;
    SpanType* structType = &expression->structAccess.value->type;
    LLVMTypeRef structTypeLLVM = structType->base->llvmType;
    if (isTypeReference(structType)) {
        expression->llvmValue = LLVMBuildStructGEP2(context.builder, structTypeLLVM, structValue, memberIndex, "structaccess");
    } else if (isTypePointer(structType)) {
        // dereference
        structValue = LLVMBuildLoad2(context.builder, structTypeLLVM, structValue, "deref");
        // we can now access the struct
        expression->llvmValue = LLVMBuildStructGEP2(context.builder, structTypeLLVM, structValue, memberIndex, "structaccess");
    } else {
        expression->llvmValue = LLVMBuildExtractValue(context.builder, structValue, memberIndex, "structaccess");
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

    if ((isIntType(fromType) || isUintType(fromType)) && isIntType(currentType)) {
        u64 fromSize = fromType->base->int_.size;
        u64 currentSize = currentType->base->int_.size;
        if (fromSize < currentSize) {
            expression->llvmValue = LLVMBuildSExt(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "sexttmp");
        } else if (fromSize > currentSize) {
            expression->llvmValue = LLVMBuildTrunc(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "trunctmp");
        } else {
            expression->llvmValue = fromExpr->llvmValue;
        }
        return;
    }

    if ((isUintType(fromType) || isIntType(fromType)) && isUintType(currentType)) {
        u64 fromSize = fromType->base->uint.size;
        u64 currentSize = currentType->base->uint.size;
        if (fromSize < currentSize) {
            expression->llvmValue = LLVMBuildZExt(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "zexttmp");
        } else if (fromSize > currentSize) {
            expression->llvmValue = LLVMBuildTrunc(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "trunctmp");
        } else {
            expression->llvmValue = fromExpr->llvmValue;
        }
        return;
    }

    if (isFloatType(fromType) && isFloatType(currentType)) {
        u64 fromSize = fromType->base->float_.size;
        u64 currentSize = currentType->base->float_.size;
        if (fromSize < currentSize) {
            expression->llvmValue = LLVMBuildFPExt(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "fpexttmp");
        } else if (fromSize > currentSize) {
            expression->llvmValue = LLVMBuildFPTrunc(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "fptrunctmp");
        } else {
            expression->llvmValue = fromExpr->llvmValue;
        }
        return;
    }
    if (isUintType(fromType) && isFloatType(currentType)) {
        expression->llvmValue = LLVMBuildUIToFP(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "uitofptmp");
        return;
    }
    if (isIntType(fromType) && isFloatType(currentType)) {
        expression->llvmValue = LLVMBuildSIToFP(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "sitofptmp");
        return;
    }
    if (isFloatType(fromType) && isUintType(currentType)) {
        expression->llvmValue = LLVMBuildFPToUI(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "fptouitmp");
        return;
    }
    if (isFloatType(fromType) && isIntType(currentType)) {
        expression->llvmValue = LLVMBuildFPToSI(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "fptositmp");
        return;
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

    if (isIntType(currentType) && isIntType(type)) {
        u64 currentSize = currentType->base->int_.size;
        u64 typeSize = type->base->int_.size;
        if (currentSize < typeSize) {
            makeCastExpression(expression, type);
            return true;
        }
    }
    if (isUintType(currentType) && isUintType(type)) {
        u64 currentSize = currentType->base->uint.size;
        u64 typeSize = type->base->uint.size;
        if (currentSize < typeSize) {
            makeCastExpression(expression, type);
            return true;
        }
    }
    if (isFloatType(currentType) && isFloatType(type)) {
        makeCastExpression(expression, type);
        return true;
    }

    if (isTypeReference(currentType)) {
        SpanType derefType = dereferenceType(currentType);
        makeCastExpression(expression, &derefType);
        implicitlyCast(expression, type, false);
        currentType = &expression->type;
        if (isTypeEqual(currentType, type)) return true;
        // undo the dereference if it failed to implicitly cast
        SpanExpression* oldExpr = expression->cast.expression;
        *expression = *oldExpr;
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
