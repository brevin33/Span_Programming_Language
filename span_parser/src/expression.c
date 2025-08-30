#include "span_parser/expression.h"
#include "span_parser.h"
#include "span_parser/ast.h"
#include "span_parser/function.h"
#include "span_parser/logging.h"
#include "span_parser/type.h"
#include "span_parser/utils.h"
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <math.h>

SpanExpression intrisicCompleteAddExpression(SpanExpression* expression, SpanScope* scope) {
    SpanExpression expr = *expression;
    SpanExpression* lhs = allocArena(context.arena, sizeof(SpanExpression));
    *lhs = *expression->biop.lhs;
    SpanExpression* rhs = allocArena(context.arena, sizeof(SpanExpression));
    *rhs = *expression->biop.rhs;
    expr.biop.lhs = lhs;
    expr.biop.rhs = rhs;
    SpanExpression ogLhs = *lhs;
    SpanExpression ogRhs = *rhs;
    SpanType* lhsType = &lhs->type;
    SpanType* rhsType = &rhs->type;

    if (isNumbericType(lhsType) && isNumbericType(rhsType)) {
        bool worked = implicitlyCast(lhs, &rhs->type, false);
        if (worked) {
            expr.type = lhs->type;
            return expr;
        }
        worked = implicitlyCast(rhs, &lhs->type, false);
        if (worked) {
            expr.type = lhs->type;
            return expr;
        }
        massert(false, "cast should work here");
        SpanExpression err = { 0 };
        return err;
    }

    SpanExpression err = { 0 };
    return err;
}

bool completeAddExpression(SpanExpression* expression, SpanScope* scope) {
    SpanExpression* lhs = expression->biop.lhs;
    SpanExpression* rhs = expression->biop.rhs;
    SpanExpression ogLhs = *lhs;
    SpanExpression ogRhs = *rhs;
    SpanType* lhsType = &lhs->type;
    SpanType* rhsType = &rhs->type;

    SpanExpression res = intrisicCompleteAddExpression(expression, scope);
    if (res.exprType != et_invalid) {
        *expression = res;
        return true;
    }

    if (isTypeReference(lhsType)) {
        SpanType derefType = dereferenceType(lhsType);
        SpanExpression ogLhs = *lhs;
        makeCastExpression(lhs, &derefType);
        res = intrisicCompleteAddExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *lhs = ogLhs;
    }
    if (isTypeReference(rhsType)) {
        SpanType derefType = dereferenceType(rhsType);
        SpanExpression ogRhs = *rhs;
        makeCastExpression(rhs, &derefType);
        res = intrisicCompleteAddExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *rhs = ogRhs;
    }
    if (isTypeReference(lhsType) && isTypeReference(rhsType)) {
        SpanType derefTypelhs = dereferenceType(lhsType);
        SpanType derefTyperhs = dereferenceType(rhsType);
        SpanExpression ogLhs = *lhs;
        SpanExpression ogRhs = *rhs;
        makeCastExpression(lhs, &derefTypelhs);
        makeCastExpression(rhs, &derefTyperhs);
        res = intrisicCompleteAddExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *lhs = ogLhs;
        *rhs = ogRhs;
    }
    // not intrinsic see if overloaded
    SpanType addTypes[2] = { lhs->type, rhs->type };
    SpanFunctionInstance* addFunction = findFunction("add", context.activeProject->namespace_, addTypes, 2, expression->ast, false);
    if (addFunction != NULL) {
        expression->exprType = et_functionCall;
        expression->type = addFunction->function->functionType->function.returnType;
        SpanExpressionFunctionCall* functionCall = &expression->functionCall;
        functionCall->args = allocArena(context.arena, sizeof(SpanExpression) * 2);
        functionCall->argsCount = 2;
        functionCall->function = addFunction;
        implicitlyCast(lhs, &functionCall->function->function->functionType->function.paramTypes[0], true);
        implicitlyCast(rhs, &functionCall->function->function->functionType->function.paramTypes[1], true);
        functionCall->args[0] = *lhs;
        functionCall->args[1] = *rhs;
        return true;
    }
    logErrorAst(expression->ast, "can't add types %s and %s", lhsType->base->name, rhsType->base->name);
    return false;
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
        case tt_add: {
            bool worked = completeAddExpression(&expression, scope);
            if (!worked) {
                SpanExpression err = { 0 };
                return err;
            }
            break;
        }
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
    expression.functionCall.argsCount = args->callParamerterList.paramsCount;
    if (expression.functionCall.argsCount > 0) {
        expression.functionCall.args = allocArena(context.arena, sizeof(SpanExpression) * args->callParamerterList.paramsCount);
    } else {
        expression.functionCall.args = NULL;
    }

    bool expressionError = false;
    for (u64 i = 0; i < args->callParamerterList.paramsCount; i++) {
        SpanAst* arg = &args->callParamerterList.params[i];
        SpanExpression argExpr = createSpanExpression(arg, scope);
        if (argExpr.exprType == et_invalid) {
            expressionError = true;
        }
        expression.functionCall.args[i] = argExpr;
    }
    if (expressionError) {
        SpanExpression err = { 0 };
        return err;
    }

    SpanType types[BUFFER_SIZE];
    u64 typesCount = 0;
    for (u64 i = 0; i < expression.functionCall.argsCount; i++) {
        SpanExpression* arg = &expression.functionCall.args[i];
        SpanType* argType = &arg->type;
        types[typesCount++] = *argType;
    }

    SpanFunctionInstance* function = findFunction(ast->functionCall.name, context.activeProject->namespace_, types, typesCount, ast, true);
    if (function->function == NULL) {
        SpanExpression err = { 0 };
        return err;
    }
    // implicitly cast function args
    for (u64 i = 0; i < expression.functionCall.argsCount; i++) {
        implicitlyCast(&expression.functionCall.args[i], &function->function->functionType->function.paramTypes[i], true);
    }
    expression.functionCall.function = function;
    expression.type = function->function->functionType->function.returnType;
    return expression;
}

SpanExpression createSpanStructAccessExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    massert(ast->type == ast_member_access, "should be a member access");
    SpanExpression expr = { 0 };
    expr.ast = ast;
    expr.exprType = et_struct_access;

    char* memberName = ast->memberAccess.memberName;
    bool isNumber = stringIsUint(memberName);
    SpanType* type = &value->type;
    SpanTypeBase* baseType = type->base;
    SpanTypeStruct* structType = &baseType->struct_;
    if (isTypeReference(type) && type->modsCount == 2) {
        SpanType derefType = dereferenceType(type);
        makeCastExpression(value, &derefType);
        type = &value->type;
    }
    expr.structAccess.value = allocArena(context.arena, sizeof(SpanExpression));
    *expr.structAccess.value = *value;
    massert(baseType->type == t_struct, "should be a struct");
    if (isTypeReference(type) || isTypePointer(type) || isTypeStruct(type) || type->modsCount <= 1) {
    } else {
        logErrorAst(ast, "should be a reference, pointer, value of struct");
        SpanExpression err = { 0 };
        return err;
    }
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
    if (value.exprType == et_type) {
        if (strcmp(accessedMemberName, "size") == 0) {
            return createSpanTypeSizeExpression(ast, scope, &value);
        }
    }
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
        case ast_expr_word: {
            SpanExpression expression = createSpanVariableExpression(ast, scope, false);
            if (expression.exprType != et_invalid) return expression;
            // seeing if this could have been a type
            ast->type = ast_type;
            ast->type_.name = ast->exprWord.word;
            ast->type_.modsCount = 0;
            ast->type_.mods = NULL;
            expression = createSpanTypeExpression(ast, scope, false);
            if (expression.exprType != et_invalid) return expression;
            logErrorAst(ast, "Is not a variable or type");
            SpanExpression err = { 0 };
            return err;
        }
        case ast_expr_number_literal:
            return createSpanNumberLiteralExpression(ast, scope);
        case ast_function_call:
            return createSpanFunctionCallExpression(ast, scope);
        case ast_member_access:
            return createMemberAccessExpression(ast, scope);
        case ast_method_call:
            return createSpanMethodCallExpression(ast, scope);
        case ast_type:
            return createSpanTypeExpression(ast, scope, true);
        case ast_cast_call:
            return createSpanCastCallExpression(ast, scope);
        case ast_index:
            return createSpanIndexExpression(ast, scope, NULL);
        default:
            massert(false, "not implemented");
            break;
    }
    massert(false, "not implemented");
    SpanExpression err = { 0 };
    return err;
}

SpanExpression intrinsicCompleteIndexExpression(SpanExpression* expression, SpanScope* scope) {
    SpanExpression expr = *expression;
    SpanExpression* index = expression->index.index;
    SpanExpression* value = expression->index.value;
    SpanType* indexType = &index->type;
    SpanType* valueType = &value->type;

    if (isTypePointer(valueType)) {
        if (isIntType(indexType)) {
            SpanType derefType = dereferenceType(valueType);
            expr.type = derefType;
            return expr;
        }
        if (isUintType(indexType)) {
            SpanType derefType = dereferenceType(valueType);
            expr.type = derefType;
            return expr;
        }
        if (isTypeNumbericLiteral(indexType)) {
            SpanType i64Type = getIntType(64);
            makeCastExpression(index, &i64Type);
            SpanType derefType = dereferenceType(valueType);
            expr.type = derefType;
            return expr;
        }
    }
    SpanExpression err = { 0 };
    return err;
}

bool completeIndexExpression(SpanExpression* expression, SpanScope* scope) {
    massert(expression->exprType == et_index, "should be an index");
    SpanExpression* index = expression->index.index;
    SpanExpression* value = expression->index.value;
    SpanType* indexType = &index->type;
    SpanType* valueType = &value->type;

    SpanExpression res = intrinsicCompleteIndexExpression(expression, scope);
    if (res.exprType != et_invalid) {
        *expression = res;
        return true;
    }

    if (isTypeReference(valueType)) {
        SpanType derefType = dereferenceType(valueType);
        SpanExpression ogLhs = *value;
        makeCastExpression(value, &derefType);
        res = intrinsicCompleteIndexExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *value = ogLhs;
    }
    if (isTypeReference(indexType)) {
        SpanType derefType = dereferenceType(indexType);
        SpanExpression ogLhs = *index;
        makeCastExpression(index, &derefType);
        res = intrinsicCompleteIndexExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *index = ogLhs;
    }
    if (isTypeReference(valueType) && isTypeReference(indexType)) {
        SpanType derefTypelhs = dereferenceType(valueType);
        SpanType derefTyperhs = dereferenceType(indexType);
        SpanExpression ogLhs = *value;
        SpanExpression ogRhs = *index;
        makeCastExpression(value, &derefTypelhs);
        makeCastExpression(index, &derefTyperhs);
        res = intrinsicCompleteIndexExpression(expression, scope);
        if (res.exprType != et_invalid) {
            *expression = res;
            return true;
        }
        *value = ogLhs;
        *index = ogRhs;
    }
    // not intrinsic see if overloaded
    SpanType indexTypes[2] = { value->type, index->type };
    SpanFunctionInstance* addFunction = findFunction("index", context.activeProject->namespace_, indexTypes, 2, expression->ast, false);
    if (addFunction != NULL) {
        expression->exprType = et_functionCall;
        expression->type = addFunction->function->functionType->function.returnType;
        SpanExpressionFunctionCall* functionCall = &expression->functionCall;
        functionCall->args = allocArena(context.arena, sizeof(SpanExpression) * 2);
        functionCall->argsCount = 2;
        functionCall->function = addFunction;
        implicitlyCast(value, &functionCall->function->function->functionType->function.paramTypes[0], true);
        implicitlyCast(index, &functionCall->function->function->functionType->function.paramTypes[1], true);
        functionCall->args[0] = *value;
        functionCall->args[1] = *index;
        return true;
    }
    logErrorAst(expression->ast, "can't index type of %s with %s", valueType->base->name, indexType->base->name);
    return false;
}

SpanExpression createSpanIndexExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    massert(ast->type == ast_index, "should be a index");
    SpanExpression index = createSpanExpression(ast->index.index, scope);
    if (index.exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    SpanExpression valueExpr = createSpanExpression(ast->index.value, scope);
    if (valueExpr.exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    SpanExpression expression;
    expression.ast = ast;
    expression.exprType = et_index;
    expression.index.value = allocArena(context.arena, sizeof(SpanExpression));
    *expression.index.value = valueExpr;
    expression.index.index = allocArena(context.arena, sizeof(SpanExpression));
    *expression.index.index = index;

    if (!completeIndexExpression(&expression, scope)) {
        SpanExpression err = { 0 };
        return err;
    }
    return expression;
}

SpanExpression createSpanCastCallExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_cast_call, "should be a cast call");

    SpanType type = getType(ast->castCall.type, true);
    if (type.base->type == t_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    SpanExpression value = createSpanExpression(ast->castCall.value, scope);
    if (value.exprType == et_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    SpanType* valueType = &value.type;


    if (isNumbericType(valueType) && isNumbericType(&type)) {
        makeCastExpression(&value, &type);
        return value;
    }

    if (isTypePointer(valueType) && isTypePointer(&type)) {
        makeCastExpression(&value, &type);
        return value;
    }

    if (isTypeReference(valueType)) {
        SpanType derefType = dereferenceType(valueType);
        if (isNumbericType(&derefType) && isNumbericType(&type)) {
            makeCastExpression(&value, &derefType);
            makeCastExpression(&value, &type);
            return value;
        }
        if (isTypePointer(&derefType) && isTypePointer(&type)) {
            makeCastExpression(&value, &derefType);
            makeCastExpression(&value, &type);
            return value;
        }
    }

    //TODO: more casts

    // if not core cast interpret like function call
    SpanType* castType = &type;
    SpanFunctionInstance* castFunction;
    if (isTypeReference(valueType)) {
        SpanType derefType = dereferenceType(valueType);
        castFunction = findCastFunction(valueType, castType, context.activeProject->namespace_, false, ast);
        if (castFunction == NULL) {
            castFunction = findCastFunction(&derefType, castType, context.activeProject->namespace_, false, ast);
            if (castFunction->function == NULL) {
                // run again with error to put out error
                // not the best way to do this but it works
                castFunction = findCastFunction(valueType, castType, context.activeProject->namespace_, true, ast);
                castFunction = findCastFunction(&derefType, castType, context.activeProject->namespace_, true, ast);
                SpanExpression err = { 0 };
                return err;
            }
            makeCastExpression(&value, &derefType);
        }
    } else {
        castFunction = findCastFunction(valueType, castType, context.activeProject->namespace_, true, ast);
        if (castFunction == NULL) {
            SpanExpression err = { 0 };
            return err;
        }
    }
    SpanExpression functionCall = { 0 };
    functionCall.ast = ast;
    functionCall.exprType = et_functionCall;
    functionCall.functionCall.args = allocArena(context.arena, sizeof(SpanExpression));
    functionCall.functionCall.argsCount = 1;
    functionCall.functionCall.function = castFunction;
    functionCall.functionCall.args[0] = value;
    functionCall.type = functionCall.functionCall.function->function->functionType->function.returnType;
    return functionCall;
}


SpanExpression createSpanTypeExpression(SpanAst* ast, SpanScope* scope, bool logError) {
    SpanExpression expression = { 0 };
    massert(ast->type == ast_type, "should be a type");
    expression.ast = ast;
    expression.exprType = et_type;
    expression.typeType.type = getType(ast, logError);
    if (expression.typeType.type.base->type == t_invalid) {
        SpanExpression err = { 0 };
        return err;
    }
    expression.type = getTypeType();
    return expression;
}

SpanExpression createSpanTypeSizeExpression(SpanAst* ast, SpanScope* scope, SpanExpression* value) {
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_type_size;
    massert(value->exprType == et_type, "should be a type");
    expression.typeSize.type = value->typeType.type;
    expression.type = getNumbericLiteralType();
    return expression;
}

SpanExpression createSpanMethodCallExpression(SpanAst* ast, SpanScope* scope) {
    massert(ast->type == ast_method_call, "should be a method call");
    // normal cases
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_functionCall;
    // parsing args
    SpanAst* args = ast->methodCall.args;
    massert(args->type == ast_call_paramerter_list, "should be a call paramerter list");
    expression.functionCall.argsCount = args->callParamerterList.paramsCount + 1;  // +1 for the this pointer
    if (expression.functionCall.argsCount > 0) {
        expression.functionCall.args = allocArena(context.arena, sizeof(SpanExpression) * expression.functionCall.argsCount);
    } else {
        expression.functionCall.args = NULL;
    }

    bool expressionError = false;
    SpanAst* value = ast->methodCall.expr;
    SpanExpression thisExpr = createSpanExpression(value, scope);
    if (thisExpr.exprType == et_invalid) {
        expressionError = true;
    }
    expression.functionCall.args[0] = thisExpr;

    for (u64 i = 1; i < expression.functionCall.argsCount; i++) {
        SpanAst* arg = &args->callParamerterList.params[i - 1];
        SpanExpression argExpr = createSpanExpression(arg, scope);
        if (argExpr.exprType == et_invalid) {
            expressionError = true;
        }
        expression.functionCall.args[i] = argExpr;
    }
    if (expressionError) {
        SpanExpression err = { 0 };
        return err;
    }

    SpanType types[BUFFER_SIZE];
    types[0] = thisExpr.type;
    u64 typesCount = 1;
    for (u64 i = 1; i < expression.functionCall.argsCount; i++) {
        SpanExpression* arg = &expression.functionCall.args[i - 1];
        SpanType* argType = &arg->type;
        types[typesCount++] = *argType;
    }
    char* methodName = ast->methodCall.methodName;

    SpanFunctionInstance* function = findFunction(ast->methodCall.methodName, context.activeProject->namespace_, types, typesCount, ast, true);
    if (function == NULL) {
        SpanExpression err = { 0 };
        return err;
    }
    // implicitly cast function args
    for (u64 i = 0; i < expression.functionCall.argsCount; i++) {
        implicitlyCast(&expression.functionCall.args[i], &function->function->functionType->function.paramTypes[i], true);
    }
    expression.functionCall.function = function;
    expression.type = function->function->functionType->function.returnType;
    return expression;
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
        case et_type_size:
            compileTypeSizeExpression(expression, scope, function);
            break;
        case et_type:
            compileTypeExpression(expression, scope, function);
            break;
        case et_index:
            compileIndexExpression(expression, scope, function);
            break;
        default:
            massert(false, "not implemented");
            break;
    }
}

void compileIndexExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    compileExpression(expression->index.value, scope, function);
    compileExpression(expression->index.index, scope, function);
    SpanExpression* value = expression->index.value;
    SpanExpression* index = expression->index.index;
    SpanType* valueType = &value->type;
    SpanType* indexType = &index->type;
    LLVMValueRef indexValue = index->llvmValue;
    LLVMValueRef valueValue = value->llvmValue;
    massert(isTypePointer(valueType), "should be a pointer");
    SpanType withoutPointer = *valueType;
    withoutPointer.modsCount = withoutPointer.modsCount - 1;
    LLVMTypeRef underlyingType = getLLVMType(&withoutPointer);
    expression->llvmValue = LLVMBuildGEP2(context.builder, underlyingType, valueValue, &indexValue, 1, "indextmp");
}

void compileTypeSizeExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_type_size, "should be a type size");
    // do nothing
}
void compileTypeExpression(SpanExpression* expression, SpanScope* scope, SpanFunction* function) {
    massert(expression->exprType == et_type, "should be a type");
    // do nothing
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
    SpanFunctionInstance* functionToCall = expression->functionCall.function;
    LLVMValueRef functionToCallLLVM = functionToCall->llvmFunc;
    LLVMTypeRef functionToCallType = functionToCall->function->functionType->llvmType;

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
    massert(structType->modsCount >= 1, "should be a reference or pointer");
    if (isTypeReference(structType)) {
        expression->llvmValue = LLVMBuildStructGEP2(context.builder, structTypeLLVM, structValue, memberIndex, "structaccess");
    } else if (isTypePointer(structType)) {
        expression->llvmValue = LLVMBuildStructGEP2(context.builder, structTypeLLVM, structValue, memberIndex, "structaccess");
    } else {
        expression->llvmValue = LLVMBuildExtractValue(context.builder, structValue, memberIndex, "structaccess");
    }
}

char* getNumberLiteralNumber(SpanExpression* expression) {
    return expression->numberLiteral.number;
}
char* getTypeSizeNumber(SpanExpression* expression) {
    massert(expression->exprType == et_type_size, "should be a type size");
    LLVMTargetDataRef dataLayout = context.activeProject->dataLayout;
    LLVMTypeRef type = getLLVMType(&expression->typeSize.type);
    if (type == NULL) {
        return "0";
    }
    i64 size = LLVMABISizeOfType(dataLayout, type);
    char buffer[BUFFER_SIZE];
    uintToString(size, buffer);
    char* number = allocArena(context.arena, strlen(buffer) + 1);
    memcpy(number, buffer, strlen(buffer) + 1);
    return number;
}

char* getBiopNumber(SpanExpression* expression) {
    SpanExpression* lhs = expression->biop.lhs;
    SpanExpression* rhs = expression->biop.rhs;
    char* lhsNumber = getNumbericLiteralNumber(lhs);
    char* rhsNumber = getNumbericLiteralNumber(rhs);
    switch (expression->biop.op) {
        case tt_add:
            return addNumbers(lhsNumber, rhsNumber);
        default:
            massert(false, "not implemented");
            break;
    }
    return NULL;
}

char* getNumbericLiteralNumber(SpanExpression* expression) {
    massert(expression->type.base->type == t_numberic_literal, "should be a number literal");

    switch (expression->exprType) {
        case et_number_literal:
            return getNumberLiteralNumber(expression);
        case et_biop:
            return getBiopNumber(expression);
        case et_type_size:
            return getTypeSizeNumber(expression);
        default:
            massert(false, "not implemented");
            return "0";
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
            char* number = getNumbericLiteralNumber(fromExpr);
            expression->llvmValue = LLVMConstIntOfString(intType, number, 10);
            return;
        }
        if (isUintType(&expression->type)) {
            LLVMTypeRef uintType = getLLVMType(&expression->type);
            char* number = getNumbericLiteralNumber(fromExpr);
            expression->llvmValue = LLVMConstIntOfString(uintType, number, 10);
            return;
        }
        if (isFloatType(&expression->type)) {
            LLVMTypeRef floatType = getLLVMType(&expression->type);
            char* number = getNumbericLiteralNumber(fromExpr);
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

    if (isTypePointer(fromType) && isTypePointer(currentType)) {
        expression->llvmValue = LLVMBuildBitCast(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "casted_ptr");
        return;
    }
    if (isTypePointer(fromType) && isIntType(currentType)) {
        expression->llvmValue = LLVMBuildPtrToInt(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "ptrtoint");
        return;
    }
    if (isIntType(fromType) && isTypePointer(currentType)) {
        expression->llvmValue = LLVMBuildIntToPtr(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "inttoptr");
        return;
    }
    if (isTypePointer(fromType) && isUintType(currentType)) {
        expression->llvmValue = LLVMBuildPtrToInt(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "ptrtoint");
        return;
    }
    if (isUintType(fromType) && isTypePointer(currentType)) {
        expression->llvmValue = LLVMBuildIntToPtr(context.builder, fromExpr->llvmValue, getLLVMType(currentType), "inttoptr");
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

SpanExpression createSpanVariableExpression(SpanAst* ast, SpanScope* scope, bool logError) {
    massert(ast->type == ast_expr_word, "should be a variable");
    SpanExpression expression = { 0 };
    expression.ast = ast;
    expression.exprType = et_variable;
    SpanVariable* variable = getVariableFromScope(scope, ast->exprWord.word);
    if (variable == NULL) {
        if (logError) {
            logErrorAst(ast, "variable does not exist");
        }
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


int canImplicitlyCast(SpanType* fromType, SpanType* toType, bool logError, SpanAst* ast) {
    if (isTypeEqual(fromType, toType)) return 0;

    if (isTypeNumbericLiteral(fromType)) {
        if (isIntType(toType)) {
            return 1;
        }
        if (isUintType(toType)) {
            return 1;
        }
        if (isFloatType(toType)) {
            return 1;
        }
    }

    if (isIntType(fromType) && isIntType(toType)) {
        u64 currentSize = fromType->base->int_.size;
        u64 typeSize = toType->base->int_.size;
        if (currentSize < typeSize) {
            return 1;
        }
    }
    if (isUintType(fromType) && isUintType(toType)) {
        u64 currentSize = fromType->base->uint.size;
        u64 typeSize = toType->base->uint.size;
        if (currentSize < typeSize) {
            return 1;
        }
    }
    if (isFloatType(fromType) && isFloatType(toType)) {
        return 1;
    }

    bool fromTypeIsVoidPtr = isTypePointer(fromType) && fromType->base->type == t_void && fromType->modsCount == 1;
    if (fromTypeIsVoidPtr && isTypePointer(toType)) {
        return 1;
    }

    if (isTypeReference(fromType)) {
        SpanType derefType = dereferenceType(fromType);
        int depth = canImplicitlyCast(&derefType, toType, false, ast);
        if (depth >= 0) return depth + 1;
    }
    if (logError) {
        char buffer[BUFFER_SIZE];
        char* currentTypeName = getTypeName(fromType, buffer);
        char buffer2[BUFFER_SIZE];
        char* typeName = getTypeName(toType, buffer2);
        logErrorAst(ast, "cannot implicitly cast %s to %s", currentTypeName, typeName);
    }
    return -1;
}

bool implicitlyCast(SpanExpression* expression, SpanType* type, bool logError) {
    SpanType* currentType = &expression->type;
    if (isTypeEqual(currentType, type)) return true;
    int depth = canImplicitlyCast(&expression->type, type, logError, expression->ast);
    if (depth == -1) return false;
    if (depth == 0) return true;
    for (u64 i = 1; i < depth; i++) {
        SpanType derefType = dereferenceType(&expression->type);
        makeCastExpression(expression, &derefType);
    }
    makeCastExpression(expression, type);
    return true;
}
