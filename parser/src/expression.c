#include "parser.h"
#include "parser/type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Expression derefExpression(Expression* expression, Scope* scope) {
    Expression deref = { 0 };
    deref.type = ek_deref;
    deref.token = expression->token;
    deref.tokenCount = expression->tokenCount;
    Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
    *e = *expression;
    deref.deref = e;
    Type* type = getTypeFromId(expression->tid);
    assert(type->kind == tk_ref || type->kind == tk_pointer);
    if (type->kind == tk_ref) {
        deref.tid = type->pointedToType;
    } else {
        deref.tid = getRefType(type->pointedToType);
    }
    return deref;
}

Expression implicitCastExpressionHelper(Expression* expression, Scope* scope, typeId type) {
    Expression implicitCast;
    implicitCast.type = ek_implicit_cast;
    implicitCast.token = expression->token;
    implicitCast.tokenCount = expression->tokenCount;
    implicitCast.tid = type;
    Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
    *e = *expression;
    implicitCast.implicitCast = e;
    return implicitCast;
}

Expression createFunctionCall(Token** tokens, functionId fid, Scope* scope) {
    Token* token = *tokens;
    Token* start = token;
    Expression expression = { 0 };

    if (token->type != tt_id) {
        logErrorTokens(token, 1, "Expected function name");
        return expression;
    }
    char* name = token->str;
    token++;

    // template definition
    TemplateDefinition templateDefinition = getTemplateDefinitionFromTokens(&token, scope->arena, false);

    if (token->type != tt_lparen) {
        logErrorTokens(token, 1, "Expected parameter list");
        return expression;
    }
    token++;

    // parameter list
    Expression* parameters = arenaAlloc(scope->arena, sizeof(Expression) * 4);
    u64 numParameters = 0;
    u64 paramCapacity = 4;
    OurTokenType delimiters[] = { tt_rparen, tt_comma };
    if (token->type != tt_rparen) {
        while (true) {
            parameters[numParameters] = createExpressionFromTokens(&token, delimiters, 2, fid, scope);
            if (parameters[numParameters].type == ek_invalid) {
                return expression;
            }
            numParameters++;
            if (token->type == tt_rparen) {
                break;
            } else if (token->type == tt_comma) {
                token++;
            }
        }
    }
    assert(token->type == tt_rparen);
    token++;


    // find the function we are calling
    FunctionList* list = getFunctionsByName(name);
    if (list == NULL) {
        logErrorTokens(token, 1, "No function with name %s", name);
        return expression;
    }

    functionId callId = BAD_ID;

    if (templateDefinition.numTypes > 0) {
        assert(false && "Not implemented");
    }

    // first check if we directly type match any existing functions
    for (u64 i = 0; i < list->count; i++) {
        Function* function = getFunctionFromId(list->functions[i]);
        if (function->numParams != numParameters) {
            continue;
        }
        if (function->templateDefinition != NULL) {
            continue;
        }
        bool fullMatch = true;
        for (u64 j = 0; j < function->numParams; j++) {
            typeId paramType = parameters[i].tid;
            typeId actualType = getActualTypeId(paramType);
            typeId functionParamType = function->paramTypes[i];
            bool match = functionParamType == paramType || functionParamType == actualType;
            if (!match) {
                fullMatch = false;
                break;
            }
        }
        if (fullMatch) {
            if (callId != BAD_ID) {
                // getting the tokens for both function for error logging
                Function* func1 = getFunctionFromId(callId);
                Function* func2 = getFunctionFromId(list->functions[i]);
                Token buf[1024];
                Token* t = func1->functionStart;
                u64 bufIndex = 0;
                while (t->type != tt_endl || t->type != tt_lbrace) {
                    t++;
                    if (bufIndex >= 1024) {
                        break;
                    }
                    buf[bufIndex++] = *t;
                }
                t = func2->functionStart;
                while (t->type != tt_endl || t->type != tt_lbrace) {
                    t++;
                    if (bufIndex >= 1024) {
                        break;
                    }
                    buf[bufIndex++] = *t;
                }
                logErrorTokens(buf, bufIndex, "Function call could mean multiple functions");
                return expression;
            }
            callId = list->functions[i];
        }
    }
    if (callId == BAD_ID) {
        // check if we can implicitly cast the parameters
        // or if we can template the function
        for (u64 i = 0; i < list->count; i++) {
            Function* function = getFunctionFromId(list->functions[i]);
            if (function->numParams != numParameters) {
                continue;
            }
            bool goodMatch = true;
            if (function->templateDefinition != NULL) {
                assert(false && "Not implemented");
            } else {
                for (u64 j = 0; j < function->numParams; j++) {
                    typeId functionParamType = function->paramTypes[i];
                    Expression implCast = implicitCastNoError(&parameters[j], functionParamType, scope, fid);
                    if (implCast.type == ek_invalid) {
                        goodMatch = false;
                        break;
                    }
                }
            }
            if (goodMatch) {
                if (callId != BAD_ID) {
                    // getting the tokens for both function for error logging
                    Function* func1 = getFunctionFromId(callId);
                    Function* func2 = getFunctionFromId(list->functions[i]);
                    Token buf[1024];
                    Token* t = func1->functionStart;
                    u64 bufIndex = 0;
                    while (t->type != tt_endl || t->type != tt_lbrace) {
                        t++;
                        if (bufIndex >= 1024) {
                            break;
                        }
                        buf[bufIndex++] = *t;
                    }
                    t = func2->functionStart;
                    while (t->type != tt_endl || t->type != tt_lbrace) {
                        t++;
                        if (bufIndex >= 1024) {
                            break;
                        }
                        buf[bufIndex++] = *t;
                    }
                    logErrorTokens(buf, bufIndex, "Function call could mean multiple functions");
                    return expression;
                }
                callId = list->functions[i];
            }
        }
    }
    if (callId == BAD_ID) {
        Token* end = token;
        logErrorTokens(start, end - start, "No function with name %s and these parameter types", name);
        return expression;
    }
    Function* function = getFunctionFromId(callId);

    for (u64 i = 0; i < function->numParams; i++) {
        parameters[i] = implicitCast(&parameters[i], function->paramTypes[i], scope, BAD_ID);
        assert(parameters[i].type != ek_invalid && "Invalid parameter");
    }

    FunctionCallData* data = arenaAlloc(scope->arena, sizeof(FunctionCallData));
    data->parameters = parameters;
    data->numParameters = numParameters;
    data->functionId = callId;
    expression.functionCallData = data;
    expression.type = ek_function_call;
    expression.token = start;
    Token* end = token;
    expression.tokenCount = end - start;
    expression.tid = function->returnType;
    *tokens = token;
    return expression;
}

Expression getSingleExpressionFromTokens(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    Expression expression = { 0 };
    Token* start = token;
    expression.token = start;
    switch (token->type) {
        case tt_float:
        case tt_int: {
            expression.type = ek_number;
            expression.number = token->str;
            expression.tid = constNumberType;
            expression.tokenCount = 1;
            expression.token = start;
            token++;
            break;
        }
        case tt_id: {
            char* str = token->str;
            typeId type = getTypeIdFromTokens(&token);
            Token* start = token;
            if (type != BAD_ID) {
                expression.type = ek_type;
                expression.tid = typeType;
                expression.typeType = type;
                expression.tokenCount = 1;
                expression.token = start;
                break;
            }
            token++;
            TemplateInstance templateInstance = getTemplateInstanceFromTokens(&token, scope->arena, false);
            if (templateInstance.numTypes != 0 || token->type == tt_lparen) {
                token = start;
                expression = createFunctionCall(&token, functionId, scope);
                if (expression.type == ek_invalid) {
                    return expression;
                }
                break;
            }
            // variable
            expression.type = ek_variable;
            Variable* variable = getVariableByName(scope, str);
            if (variable == NULL) {
                logErrorTokens(token - 1, 1, "Variable not found");
                expression.type = ek_invalid;
                return expression;
            }
            char* typeName = getTypeFromId(variable->type)->name;
            expression.variable = str;
            expression.tid = getRefType(variable->type);
            expression.tokenCount = 1;
            expression.token = start;
            break;
        }
        case tt_lbrace: {
            OurTokenType delimiters[] = { tt_rbrace };
            token++;
            expression = createExpressionFromTokens(&token, delimiters, 1, functionId, scope);
            if (expression.type == ek_invalid) {
                return expression;
            }
            expression.tokenCount = expression.tokenCount + 1;
            expression.token = start;
            break;
        }
        case tt_string: {
            Token* start = token;
            u64 numStrings = 1;
            token++;
            while (token->type == tt_str_expr_start) {
                token++;
                u64 exprsStart = 1;
                while (true) {
                    if (token->type == tt_str_expr_end) {
                        exprsStart--;
                    }
                    if (token->type == tt_str_expr_start) {
                        exprsStart++;
                    }
                    if (exprsStart == 0) {
                        break;
                    }
                }
                token++;
                assert(token->type == tt_string);
                numStrings++;
                token++;
            }
            token = start;
            char** strings = arenaAlloc(scope->arena, sizeof(char*) * numStrings);
            Expression* expressions = NULL;
            if (numStrings > 1) {
                expressions = arenaAlloc(scope->arena, sizeof(Expression) * (numStrings - 1));
            }
            int i = 0;
            while (true) {
                char* str = token->str;
                strings[i] = str;
                token++;
                if (token->type != tt_str_expr_start) {
                    break;
                }
                token++;
                OurTokenType delimiters[] = { tt_str_expr_end };
                Expression expr = createExpressionFromTokens(&token, delimiters, 1, functionId, scope);
                if (expr.type == ek_invalid) {
                    return expr;
                }

                // TODO: cast to string
                assert(false && "Not implemented");

                expressions[i] = expr;
                i++;
                token++;
            }
            assert(i == numStrings - 1);
            StringData* stringData = arenaAlloc(scope->arena, sizeof(StringData));
            stringData->strings = strings;
            stringData->expressions = expressions;
            stringData->numStrings = numStrings;
            expression.type = ek_string;
            expression.token = start;
            expression.tokenCount = token - start;
            expression.stringData = stringData;
            expression.tid = invalidType;
            break;
        }
        case tt_lparen: {
            token++;
            OurTokenType delimiters[] = { tt_rparen, tt_endl };
            u64 numDelimiters = 2;
            expression = createExpressionFromTokens(&token, delimiters, numDelimiters, functionId, scope);
            if (expression.type == ek_invalid) {
                return expression;
            }
            if (token->type == tt_endl) {
                logErrorTokens(start, 1, "Unterminated parenthesis");
            }
            expression.tokenCount = expression.tokenCount + 1;
            expression.token = start;
            token++;
            break;
        }
        default: {
            logErrorTokens(token, 1, "Can't parse this as a value");
            return expression;
        }
    }
    *tokens = token;
    return expression;
}

i64 getPrecedence(Token* token) {
    switch (token->type) {
        case tt_dot:
            return 0;
        case tt_mul:
        case tt_div:
        case tt_mod:
            return 1;
        case tt_add:
        case tt_sub:
            return 2;
        case tt_lshift:
        case tt_rshift:
            return 3;
        case tt_lt:
        case tt_leq:
        case tt_gt:
        case tt_geq:
            return 4;
        case tt_eq:
        case tt_neq:
            return 5;
        case tt_bit_and:
            return 6;
        case tt_xor:
            return 7;
        case tt_bit_or:
            return 8;
        case tt_and:
            return 9;
        case tt_or:
            return 10;
        case tt_comma:
            return 11;
        default:
            return -1;  // error
    }
}


void expressionAcutalType(Expression* expression, Scope* scope) {
    while (getTypeFromId(expression->tid)->kind == tk_ref) {
        Expression deref = derefExpression(expression, scope);
        *expression = deref;
    }
}

char* opToString(OurTokenType operator) {
    switch (operator) {
        case tt_add:
            return "+";
        case tt_sub:
            return "-";
        case tt_mul:
            return "*";
        case tt_div:
            return "/";
        case tt_mod:
            return "%";
        case tt_eq:
            return "==";
        case tt_neq:
            return "!=";
        case tt_lt:
            return "<";
        case tt_leq:
            return "<=";
        case tt_gt:
            return ">";
        case tt_geq:
            return ">=";
        case tt_bit_and:
            return "&";
        case tt_bit_or:
            return "|";
        case tt_xor:
            return "^";
        case tt_and:
            return "&&";
        case tt_or:
            return "||";
        case tt_comma:
            return ",";
        case tt_dot:
            return ".";
        default:
            return "(lazy dev forgot to add a string for this operator)";
    }
}


void applyCastsForConstNumber(Expression* expr, typeId type, Scope* scope) {
    assert(getTypeFromId(expr->tid)->kind == tk_const_number);
    if (expr->type == ek_biop) {
        Expression* left = expr->biopData->left;
        Expression* right = expr->biopData->right;
        expr->tid = type;
        applyCastsForConstNumber(left, type, scope);
        applyCastsForConstNumber(right, type, scope);
    } else {
        Expression* newExpr = arenaAlloc(scope->arena, sizeof(Expression));
        *newExpr = *expr;
        expr->type = ek_implicit_cast;
        expr->tid = type;
        expr->implicitCast = newExpr;
    }
}


Expression _createBiopExpression(Expression* left, Expression* right, OurTokenType operator, Scope * scope) {
    Expression expression = { 0 };
    if (left->type == ek_grouped_data) {
        logErrorTokens(left->token, left->tokenCount, "Can't use grouped data with this biops");
        return expression;
    }
    if (right->type == ek_grouped_data) {
        logErrorTokens(right->token, right->tokenCount, "Can't use grouped data with this biops");
        return expression;
    }
    if (left->type == ek_string || right->type == ek_string) {
        logErrorTokens(left->token, left->tokenCount, "Can't use string with this biops");
        return expression;
    }
    expressionAcutalType(left, scope);
    expressionAcutalType(right, scope);
    expression.type = ek_biop;
    expression.biopData = arenaAlloc(scope->arena, sizeof(BiopData));
    expression.biopData->left = left;
    expression.biopData->right = right;
    expression.biopData->operator= operator;
    expression.token = left->token;
    expression.tokenCount = right->token - left->token + right->tokenCount;
    bool leftIsNumber = isNumberType(left->tid);
    bool rightIsNumber = isNumberType(right->tid);
    TypeKind leftKind = getTypeFromId(left->tid)->kind;
    TypeKind rightKind = getTypeFromId(right->tid)->kind;
    if (leftIsNumber && rightIsNumber) {
        u64 leftSize = getTypeFromId(left->tid)->numberSize;
        u64 rightSize = getTypeFromId(right->tid)->numberSize;
        if (leftKind == tk_int && rightKind == tk_int || leftKind == tk_uint && rightKind == tk_uint || leftKind == tk_float && rightKind == tk_float) {
            if (leftSize == rightSize) {
                expression.tid = left->tid;
            } else if (leftSize < rightSize) {
                Expression* newLeft = arenaAlloc(scope->arena, sizeof(Expression));
                *newLeft = implicitCastExpressionHelper(left, scope, right->tid);
                expression.biopData->left = newLeft;
                expression.tid = right->tid;
            } else {
                Expression* newRight = arenaAlloc(scope->arena, sizeof(Expression));
                *newRight = implicitCastExpressionHelper(right, scope, left->tid);
                expression.biopData->right = newRight;
                expression.tid = left->tid;
            }
            return expression;
        }
        if (leftKind == tk_float && rightKind == tk_int || leftKind == tk_float && rightKind == tk_uint) {
            Expression* newRight = arenaAlloc(scope->arena, sizeof(Expression));
            *newRight = implicitCastExpressionHelper(right, scope, left->tid);
            expression.biopData->right = newRight;
            expression.tid = left->tid;
            return expression;
        }
        if (leftKind == tk_int && rightKind == tk_float || leftKind == tk_uint && rightKind == tk_float) {
            Expression* newLeft = arenaAlloc(scope->arena, sizeof(Expression));
            *newLeft = implicitCastExpressionHelper(left, scope, right->tid);
            expression.biopData->left = newLeft;
            expression.tid = right->tid;
            return expression;
        }
    }
    if (leftKind == tk_const_number && rightKind == tk_const_number) {
        expression.tid = constNumberType;
        return expression;
    }
    if (leftKind == tk_const_number && rightIsNumber) {
        bool valid = constExpressionNumberWorksWithType(left, right->tid, scope->arena);
        if (!valid) {
            u64 tokenCount = expression.tokenCount;
            logErrorTokens(left->token, tokenCount, "Can't use add constant with this type");
            expression.type = ek_invalid;
            return expression;
        }
        applyCastsForConstNumber(left, right->tid, scope);
        expression.tid = right->tid;
        return expression;
    }
    if (rightKind == tk_const_number && leftIsNumber) {
        bool valid = constExpressionNumberWorksWithType(right, left->tid, scope->arena);
        if (!valid) {
            u64 tokenCount = left->token - right->token + left->tokenCount;
            logErrorTokens(right->token, tokenCount, "Can't use add constant with this type");
            expression.type = ek_invalid;
            return expression;
        }
        applyCastsForConstNumber(right, left->tid, scope);
        expression.tid = left->tid;
        return expression;
    }

    if (leftKind == tk_pointer && rightKind == tk_pointer) {
        if (left->tid == right->tid && operator== tt_sub) {
            expression.tid = getUintType(64);
            return expression;
        }
    }

    if (leftKind == tk_pointer && (rightKind == tk_int || rightKind == tk_uint)) {
        if (operator== tt_add || operator== tt_sub) {
            if (operator== tt_add) {
                expression.tid = left->tid;
            } else {
                expression.tid = getIntType(64);
            }
            return expression;
        }
    }
    if ((leftKind == tk_int || leftKind == tk_uint) && rightKind == tk_pointer) {
        if (operator== tt_add || operator== tt_sub) {
            if (operator== tt_add) {
                expression.tid = right->tid;
            } else {
                expression.tid = getIntType(64);
            }
            return expression;
        }
    }
    if (leftKind == tk_pointer && rightKind == tk_const_number) {
        if (operator== tt_add || operator== tt_sub) {
            bool isNeg = right->number[0] == '-';
            bool valid;
            if (isNeg) {
                typeId intType = getIntType(64);
                valid = constExpressionNumberWorksWithType(left, intType, scope->arena);
            } else {
                typeId uintType = getUintType(64);
                valid = constExpressionNumberWorksWithType(left, uintType, scope->arena);
            }
            if (valid) {
                typeId type = isNeg ? getIntType(64) : getUintType(64);
                applyCastsForConstNumber(right, type, scope);
                if (operator== tt_add) {
                    expression.tid = left->tid;
                } else if (operator== tt_sub) {
                    expression.tid = getIntType(64);
                }
                return expression;
            }
        }
    }
    if (rightKind == tk_pointer && leftKind == tk_const_number) {
        if (operator== tt_add || operator== tt_sub) {
            bool isNeg = right->number[0] == '-';
            bool valid;
            if (isNeg) {
                typeId intType = getIntType(64);
                valid = constExpressionNumberWorksWithType(left, intType, scope->arena);
            } else {
                typeId uintType = getUintType(64);
                valid = constExpressionNumberWorksWithType(left, uintType, scope->arena);
            }
            if (valid) {
                typeId type = isNeg ? getIntType(64) : getUintType(64);
                applyCastsForConstNumber(left, type, scope);
                if (operator== tt_add) {
                    expression.tid = right->tid;
                } else if (operator== tt_sub) {
                    expression.tid = getIntType(64);
                }
                return expression;
            }
        }
    }

    char* op = opToString(operator);
    Token* end = right->token + right->tokenCount;
    Token* start = left->token;
    u64 tokenCount = end - start;
    logErrorTokens(left->token, tokenCount, "Can't %s %s %s", getTypeFromId(left->tid)->name, op, getTypeFromId(right->tid)->name);
    expression.type = ek_invalid;
    return expression;
}

Expression createBiopExpression(Expression* left, Expression* right, OurTokenType operator, Scope * scope) {
    Expression e = _createBiopExpression(left, right, operator, scope);
    if (e.type == ek_invalid) {
        return e;
    }
    switch (operator) {
        case tt_leq:
        case tt_geq:
        case tt_lt:
        case tt_gt:
        case tt_eq:
        case tt_neq: {
            Type* currentType = getTypeFromId(e.tid);
            if (currentType->kind == tk_const_number) {
                u64 numberSize = 256;
                bool valid = constExpressionNumberWorksWithType(&e, getIntType(numberSize), scope->arena);
                if (valid) {
                    applyCastsForConstNumber(&e, getIntType(numberSize), scope);
                } else {
                    logErrorTokens(e.token, e.tokenCount, "constant doesn't fit in 256 bit int");
                }
            }
            e.tid = boolType;
            break;
        }
        default: {
            break;
        }
    }
    return e;
}


Expression _createExpressionFromTokens(Token** tokens, OurTokenType* delimiters, u64 numDelimiters, functionId functionId, Scope* scope, i64 precedence) {
    Token* token = *tokens;
    Expression left = getSingleExpressionFromTokens(&token, functionId, scope);
    if (left.type == ek_invalid) {
        return left;
    }
    while (true) {
        for (u64 i = 0; i < numDelimiters; i++) {
            if (token->type == delimiters[i]) {
                *tokens = token;
                return left;
            }
        }

        i64 newPrecedence = getPrecedence(token);
        if (newPrecedence == -1) {
            logErrorTokens(token, 1, "Invalid biop token");
            left.type = ek_invalid;
            return left;
        }
        if (newPrecedence >= precedence) {
            *tokens = token;
            return left;
        }
        OurTokenType op = token->type;
        token++;

        // special handeling for these operators
        switch (op) {
            case tt_comma: {
                token++;
                u64 exprsCount = 1;
                u64 exprsCapacity = 3;
                Expression* exprs = arenaAlloc(scope->arena, sizeof(Expression) * exprsCapacity);
                exprs[0] = left;
                OurTokenType* delimiters = arenaAlloc(scope->arena, sizeof(OurTokenType) * numDelimiters + 1);
                memcpy(delimiters, delimiters, sizeof(OurTokenType) * numDelimiters);
                delimiters[numDelimiters] = tt_comma;
                while (true) {
                    Expression expr = createExpressionFromTokens(&token, delimiters, numDelimiters, functionId, scope);
                    if (expr.type == ek_invalid) {
                        return expr;
                    }
                    if (exprsCount == exprsCapacity) {
                        exprs = arenaRealloc(scope->arena, exprs, sizeof(Expression) * exprsCapacity, sizeof(Expression) * exprsCapacity * 2);
                        exprsCapacity *= 2;
                    }
                    exprs[exprsCount] = expr;
                    exprsCount++;
                    if (token->type == tt_comma) {
                        continue;
                    }
                    break;
                }
                GroupedData* groupedData = arenaAlloc(scope->arena, sizeof(GroupedData));
                groupedData->expressions = exprs;
                groupedData->numFields = exprsCount;
                left.type = ek_grouped_data;
                left.groupedData = groupedData;
                left.token = left.token;
                Token* start = left.token;
                Token* end = exprs[exprsCount - 1].token + exprs[exprsCount - 1].tokenCount;
                left.tokenCount = end - start;
                left.tid = BAD_ID;  // bad because this should always be parsed out to somthing else
                *tokens = token;
                return left;
            }
            case tt_dot: {
                switch (token->type) {
                    case tt_int:
                    case tt_id: {
                        char* name = token->str;
                        token++;
                        if (left.tid == typeType) {
                            assert(false && "Not implemented");
                        }
                        typeId leftType = left.tid;
                        Type* leftTypePtr = getTypeFromId(leftType);
                        char* leftTypeName = getTypeFromId(leftType)->name;
                        bool isRef = getTypeFromId(left.tid)->kind == tk_ref;
                        u64 nameLength = strlen(name);
                        if (isRef) {
                            if (strcmp(name, "ptr") == 0) {
                                Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                                memcpy(e, &left, sizeof(Expression));
                                Expression new;
                                new.type = ek_ptr;
                                new.token = left.token;
                                Token* start = left.token;
                                Token* end = token;
                                new.tokenCount = end - start;
                                typeId actualType = getActualTypeId(left.tid);
                                new.tid = getPtrType(actualType);
                                char* newTypeName = getTypeFromId(new.tid)->name;
                                new.expressionOfPtr = e;
                                left = new;
                                continue;
                            }

                            typeId underlyingType = leftTypePtr->pointedToType;
                            Type* underlyingTypePtr = getTypeFromId(underlyingType);
                            if (underlyingTypePtr->kind == tk_struct) {
                                bool shouldContinue = false;
                                for (u64 i = 0; i < underlyingTypePtr->structData->numFields; i++) {
                                    if (shouldContinue) {
                                        break;
                                    }
                                    // first check just getting by number
                                    char* fieldName = underlyingTypePtr->structData->fieldNames[i];
                                    char iAsString[1024];
                                    sprintf(iAsString, "%llu", i);
                                    if (strcmp(name, fieldName) == 0 || strcmp(name, iAsString) == 0) {
                                        Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                                        *e = left;
                                        Expression getStructVal = getStructValue(e, i, scope, left.token, left.tokenCount);
                                        left = getStructVal;
                                        shouldContinue = true;
                                        continue;
                                    }
                                }
                                if (shouldContinue) {
                                    continue;
                                }
                            }
                        }
                        expressionAcutalType(&left, scope);
                        bool isPtr = getTypeFromId(left.tid)->kind == tk_pointer;
                        char* typeName = getTypeFromId(left.tid)->name;
                        if (isPtr) {
                            if (strcmp(name, "val") == 0) {
                                Expression deref = derefExpression(&left, scope);
                                Token* start = left.token;
                                Token* end = token;
                                deref.tokenCount = end - start;
                                left = deref;
                                continue;
                            }
                            typeId underlyingType = leftTypePtr->pointedToType;
                            Type* underlyingTypePtr = getTypeFromId(underlyingType);
                            if (underlyingTypePtr->kind == tk_struct) {
                                for (u64 i = 0; i < underlyingTypePtr->structData->numFields; i++) {
                                    // first check just getting by number
                                    char* fieldName = underlyingTypePtr->structData->fieldNames[i];
                                    char iAsString[1024];
                                    sprintf(iAsString, "%llu", i);
                                    if (strcmp(name, fieldName) == 0 || strcmp(name, iAsString) == 0) {
                                        Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                                        *e = left;
                                        Expression getStructVal = getStructValue(e, i, scope, left.token, left.tokenCount);
                                        left = getStructVal;
                                        continue;
                                    }
                                }
                            }
                        }
                        bool isStruct = getTypeFromId(left.tid)->kind == tk_struct;
                        if (isStruct) {
                            for (u64 i = 0; i < getTypeFromId(left.tid)->structData->numFields; i++) {
                                // first check just getting by number
                                char* fieldName = getTypeFromId(left.tid)->structData->fieldNames[i];
                                char iAsString[1024];
                                sprintf(iAsString, "%llu", i);
                                if (strcmp(name, fieldName) == 0 || strcmp(name, iAsString) == 0) {
                                    Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                                    *e = left;
                                    Expression getStructVal = getStructValue(e, i, scope, left.token, left.tokenCount);
                                    left = getStructVal;
                                    continue;
                                }
                            }
                        }
                        logErrorTokens(token, 1, "Can't use dot operator with this name: %s", name);
                        left.type = ek_invalid;
                        return left;
                    }
                    default: {
                        logErrorTokens(token, 1, "Can't use dot operator with this token");
                        left.type = ek_invalid;
                        return left;
                    }
                }
            }
            default: {
                break;
            }
        }


        Expression right = _createExpressionFromTokens(&token, delimiters, numDelimiters, functionId, scope, precedence);
        if (right.type == ek_invalid) {
            return right;
        }
        Expression* r = arenaAlloc(scope->arena, sizeof(Expression));
        *r = right;
        Expression* l = arenaAlloc(scope->arena, sizeof(Expression));
        *l = left;
        left = createBiopExpression(l, r, op, scope);
        if (left.type == ek_invalid) {
            return left;
        }
    }
}

Expression createExpressionFromTokens(Token** tokens, OurTokenType* delimiters, u64 numDelimiters, functionId functionId, Scope* scope) {
    return _createExpressionFromTokens(tokens, delimiters, numDelimiters, functionId, scope, INT64_MAX);
}

Expression getStructValue(Expression* expression, u64 field, Scope* scope, Token* token, u64 tokenCount) {
    Expression newExpression = { 0 };
    typeId tid = expression->tid;
    bool isRef = getTypeFromId(tid)->kind == tk_ref;
    tid = getActualTypeId(tid);
    Type* type = getTypeFromId(tid);
    assert(type->kind == tk_struct && "Not a struct");
    StructData* structData = type->structData;
    StructValueData* structValueData = arenaAlloc(scope->arena, sizeof(StructValueData));
    structValueData->field = field;
    structValueData->expression = expression;
    newExpression.type = ek_struct_value;
    newExpression.structValueData = structValueData;
    newExpression.token = token;
    newExpression.tokenCount = tokenCount;
    if (isRef) {
        newExpression.tid = getRefType(structData->fields[field]);
    } else {
        newExpression.tid = structData->fields[field];
    }
    return newExpression;
}

Expression makeStruct(Expression* expressions, u64 numExpressions, typeId tid, Scope* scope, Token* token, u64 tokenCount) {
    Expression newExpression = { 0 };
    Type* type = getTypeFromId(tid);
    assert(type->kind == tk_struct && "Not a struct");
    StructData* structData = type->structData;
    u64 numFields = structData->numFields;
    assert(numFields == numExpressions && "Number of fields in struct doesn't match number of expressions");
    Expression* expressionForStruct = arenaAlloc(scope->arena, sizeof(Expression) * numFields);
    for (u64 i = 0; i < numFields; i++) {
        expressionForStruct[i] = implicitCast(&expressions[i], structData->fields[i], scope, BAD_ID);
        if (expressionForStruct[i].type == ek_invalid) {
            return newExpression;
        }
    }
    newExpression.type = ek_make_struct;
    newExpression.token = token;
    newExpression.tokenCount = tokenCount;
    newExpression.tid = tid;
    newExpression.makeStructExpressions = expressionForStruct;
    return newExpression;
}

Expression _implicitCast(Expression* expression, typeId type, Scope* scope, functionId functionId, bool logError) {
    if (expression->tid == type) {
        return *expression;
    }
    Expression newExpression = { 0 };
    if (expression->type == ek_grouped_data) {
        type = getActualTypeId(type);
        Type* t = getTypeFromId(type);
        if (t->kind != tk_struct) {
            if (logError) logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from grouped data to non struct");
        }
        if (t->structData->numFields != expression->groupedData->numFields) {
            if (logError) logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from grouped data to struct with different number of fields");
            return newExpression;
        }
        Expression* exprs = expression->groupedData->expressions;
        Expression mStruct = makeStruct(exprs, expression->groupedData->numFields, type, scope, expression->token, expression->tokenCount);
        if (mStruct.type == ek_invalid) {
            return newExpression;
        }
        return mStruct;
    } else if (expression->type == ek_string) {
        Type* ttype = getTypeFromId(type);
        bool isCharPtr = ttype->kind == tk_pointer;
        if (isCharPtr) {
            Type* underlyingType = getTypeFromId(ttype->pointedToType);
            u64 intSize = underlyingType->numberSize;
            isCharPtr = intSize == 8 && underlyingType->kind == tk_uint;
        }
        if (isCharPtr) {
            newExpression = implicitCastExpressionHelper(expression, scope, type);
            return newExpression;
        }
        logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from compiler string to %s", getTypeFromId(type)->name);
        return newExpression;
    }
    expressionAcutalType(expression, scope);
    type = getActualTypeId(type);
    // need to check this after getting the acutal type as well
    if (expression->tid == type) {
        return *expression;
    }
    bool eisNumber = isNumberType(expression->tid);
    bool tisNumber = isNumberType(type);
    TypeKind ekind = getTypeFromId(expression->tid)->kind;
    TypeKind tkind = getTypeFromId(type)->kind;
    if (eisNumber && tisNumber) {
        u64 esize = getTypeFromId(expression->tid)->numberSize;
        u64 tsize = getTypeFromId(type)->numberSize;
        // print out type names
        char* eTypeName = getTypeFromId(expression->tid)->name;
        char* tTypeName = getTypeFromId(type)->name;
        if (ekind == tkind) {
            if (esize == tsize) {
                return *expression;
            }
            if (esize < tsize) {
                return implicitCastExpressionHelper(expression, scope, type);
            }
        }
        if (tkind == tk_float && (ekind == tk_int || ekind == tk_uint)) {
            return implicitCastExpressionHelper(expression, scope, type);
        }
    }
    if (type == boolType) {
        bool isNumber = isNumberType(expression->tid);
        bool isPtr = getTypeFromId(expression->tid)->kind == tk_pointer;
        bool isConstNumber = getTypeFromId(expression->tid)->kind == tk_const_number;
        if (isNumber || isPtr) {
            return implicitCastExpressionHelper(expression, scope, boolType);
        } else if (isConstNumber) {
            // not the best to cast here like this but don't want to deal with case as it is never used by anyone
            // and it is optimized out anyway
            u64 numberSize = 256;
            bool valid = constExpressionNumberWorksWithType(expression, getIntType(numberSize), scope->arena);
            if (valid) {
                applyCastsForConstNumber(expression, getIntType(numberSize), scope);
                return implicitCastExpressionHelper(expression, scope, boolType);
            }
            if (logError) logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from %s to %s", getTypeFromId(expression->tid)->name, getTypeFromId(getIntType(numberSize))->name);
            return newExpression;
        }
    }
    if (ekind == tk_const_number && tisNumber) {
        bool valid = constExpressionNumberWorksWithType(expression, type, scope->arena);
        if (valid) {
            applyCastsForConstNumber(expression, type, scope);
            return *expression;
        }
        if (logError) logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from %s to %s", getTypeFromId(expression->tid)->name, getTypeFromId(type)->name);
        return newExpression;
    }

    if (logError) logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast from %s to %s", getTypeFromId(expression->tid)->name, getTypeFromId(type)->name);
    return newExpression;
}

Expression implicitCast(Expression* expression, typeId type, Scope* scope, functionId functionId) {
    return _implicitCast(expression, type, scope, functionId, true);
}

Expression implicitCastNoError(Expression* expression, typeId type, Scope* scope, functionId functionId) {
    return _implicitCast(expression, type, scope, functionId, false);
}
