#include "parser.h"
#include "parser/type.h"
#include <stdlib.h>
#include <string.h>

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
                // TODO: function call
                assert(false && "Not implemented function call");
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
        Expression deref = { 0 };
        deref.type = ek_deref;
        deref.token = expression->token;
        deref.tokenCount = expression->tokenCount;
        Type* type = getTypeFromId(expression->tid);
        deref.tid = type->pointedToType;
        Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
        *e = *expression;
        deref.deref = e;
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
            return "lazy dev forgot to add a string for this operator";
    }
}


Expression createBiopExpression(Expression* left, Expression* right, OurTokenType operator, Scope * scope) {
    Expression expression = { 0 };
    if (left->type == ek_grouped_data) {
        logErrorTokens(left->token, left->tokenCount, "Can't use grouped data with this biops");
        return expression;
    }
    if (right->type == ek_grouped_data) {
        logErrorTokens(right->token, right->tokenCount, "Can't use grouped data with this biops");
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
                newLeft->type = ek_implicit_cast;
                newLeft->tid = right->tid;
                newLeft->token = left->token;
                newLeft->tokenCount = left->tokenCount;
                newLeft->implicitCast = left;
                expression.biopData->left = newLeft;
                expression.tid = right->tid;
            } else {
                Expression* newRight = arenaAlloc(scope->arena, sizeof(Expression));
                newRight->type = ek_implicit_cast;
                newRight->tid = left->tid;
                newRight->token = right->token;
                newRight->tokenCount = right->tokenCount;
                newRight->implicitCast = right;
                expression.biopData->right = newRight;
                expression.tid = left->tid;
            }
        }
        if (leftKind == tk_float && rightKind == tk_int || leftKind == tk_float && rightKind == tk_uint) {
            Expression* newRight = arenaAlloc(scope->arena, sizeof(Expression));
            newRight->type = ek_implicit_cast;
            newRight->tid = left->tid;
            newRight->token = right->token;
            newRight->tokenCount = right->tokenCount;
            newRight->implicitCast = right;
            expression.biopData->right = newRight;
            expression.tid = left->tid;
        }
        if (leftKind == tk_int && rightKind == tk_float || leftKind == tk_uint && rightKind == tk_float) {
            Expression* newLeft = arenaAlloc(scope->arena, sizeof(Expression));
            newLeft->type = ek_implicit_cast;
            newLeft->tid = right->tid;
            newLeft->token = left->token;
            newLeft->tokenCount = left->tokenCount;
            newLeft->implicitCast = left;
            expression.biopData->left = newLeft;
            expression.tid = right->tid;
        }
        return expression;
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
        Expression* newLeft = arenaAlloc(scope->arena, sizeof(Expression));
        newLeft->type = ek_implicit_cast;
        newLeft->tid = right->tid;
        newLeft->token = left->token;
        newLeft->implicitCast = left;
        expression.biopData->left = newLeft;
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
        Expression* newRight = arenaAlloc(scope->arena, sizeof(Expression));
        newRight->type = ek_implicit_cast;
        newRight->tid = left->tid;
        newRight->token = right->token;
        newRight->implicitCast = right;
        expression.biopData->right = newRight;
        expression.tid = left->tid;
        return expression;
    }

    if (leftKind == tk_pointer && rightKind == tk_pointer) {
        if (left->tid == right->tid) {
            expression.tid = left->tid;
            return expression;
        }
    }

    if (leftKind == tk_pointer && (rightKind == tk_int || rightKind == tk_uint)) {
        expression.tid = left->tid;
        return expression;
    }
    if ((leftKind == tk_int || leftKind == tk_uint) && rightKind == tk_pointer) {
        expression.tid = right->tid;
        return expression;
    }
    if (leftKind == tk_pointer && rightKind == tk_const_number) {
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
            expression.tid = right->tid;
            return expression;
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
                        typeId leftType = getTypeFromId(left.tid)->pointedToType;
                        char* leftTypeName = getTypeFromId(leftType)->name;
                        bool isRef = getTypeFromId(left.tid)->kind == tk_ref;
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
                        }
                        expressionAcutalType(&left, scope);
                        bool isPtr = getTypeFromId(left.tid)->kind == tk_pointer;
                        char* typeName = getTypeFromId(left.tid)->name;
                        if (isPtr && strcmp(name, "val") == 0) {
                            Expression deref = { 0 };
                            deref.type = ek_deref;
                            deref.token = left.token;
                            Token* start = left.token;
                            Token* end = token;
                            deref.tokenCount = end - start;
                            Type* type = getTypeFromId(left.tid);
                            deref.tid = type->pointedToType;
                            Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                            *e = left;
                            deref.deref = e;
                            left = deref;
                            continue;
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
        left = createBiopExpression(l, r, token->type, scope);
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

Expression boolCast(Expression* expression, Scope* scope, functionId functionId) {
    Expression newExpression = { 0 };
    if (expression->type == ek_grouped_data) {
        logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly grouped data to bool");
        return newExpression;
    }
    expressionAcutalType(expression, scope);
    bool isNumber = isNumberType(expression->tid);
    bool isPtr = getTypeFromId(expression->tid)->kind == tk_pointer;
    bool isConstNumber = getTypeFromId(expression->tid)->kind == tk_const_number;
    if (isNumber || isPtr || isConstNumber) {
        newExpression.type = ek_implicit_cast;
        newExpression.tid = boolType;
        newExpression.token = expression->token;
        newExpression.tokenCount = expression->tokenCount;
        Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
        *e = *expression;
        newExpression.implicitCast = e;
        return newExpression;
    }
    logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast to bool");
    return newExpression;
}

Expression implicitCast(Expression* expression, typeId type, Scope* scope, functionId functionId) {
    if (expression->tid == type) {
        return *expression;
    }
    Expression newExpression = { 0 };
    if (expression->type == ek_grouped_data) {
        type = getActualTypeId(type);
        Type* t = getTypeFromId(type);
        if (t->kind != tk_struct) {
            logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast grouped data to non struct");
        }
        if (t->structData->numFields != expression->groupedData->numFields) {
            logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast grouped data to struct with different number of fields");
            return newExpression;
        }
        Expression* exprs = expression->groupedData->expressions;
        Expression mStruct = makeStruct(exprs, expression->groupedData->numFields, type, scope, expression->token, expression->tokenCount);
        if (mStruct.type == ek_invalid) {
            return newExpression;
        }
        return mStruct;
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
                newExpression.type = ek_implicit_cast;
                newExpression.tid = type;
                newExpression.token = expression->token;
                newExpression.tokenCount = expression->tokenCount;
                Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
                *e = *expression;
                newExpression.implicitCast = e;
                return newExpression;
            }
        }
        if (tkind == tk_float && (ekind == tk_int || ekind == tk_uint)) {
            newExpression.type = ek_implicit_cast;
            newExpression.tid = type;
            newExpression.token = expression->token;
            newExpression.tokenCount = expression->tokenCount;
            Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
            *e = *expression;
            newExpression.implicitCast = e;
            return newExpression;
        }
    }
    if (type == boolType) {
        return boolCast(expression, scope, functionId);
    }
    if (ekind == tk_const_number && tisNumber) {
        bool valid = constExpressionNumberWorksWithType(expression, type, scope->arena);
        if (valid) {
            newExpression.type = ek_implicit_cast;
            newExpression.tid = type;
            newExpression.token = expression->token;
            newExpression.tokenCount = expression->tokenCount;
            Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
            *e = *expression;
            newExpression.implicitCast = e;
            return newExpression;
        }
        logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast to %s", getTypeFromId(type)->name);
        return newExpression;
    }

    logErrorTokens(expression->token, expression->tokenCount, "Can't implicitly cast to %s", getTypeFromId(type)->name);
    return newExpression;
}
