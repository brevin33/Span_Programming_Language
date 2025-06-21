#include "parser.h"
#include "parser/type.h"
#include <stdlib.h>

Expression getSingleExpressionFromTokens(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    Expression expression = { 0 };
    Token* start = token;
    expression.token = start;
    switch (token->type) {
        case tt_float:
        case tt_int: {
            expression.type = tk_int;
            expression.number = token->str;
            expression.tid = constNumberType;
            token++;
            break;
        }
        case tt_id: {
            char* str = token->str;
            typeId type = getTypeIdFromTokens(&token);
            if (type != BAD_ID) {
                expression.type = ek_type;
                expression.tid = type;
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
            expression.variable = str;
            expression.tid = getRefType(variable->type);
            break;
        }
        case tt_lparen: {
            token++;
            OurTokenType delimiters[] = { tt_rparen, tt_endl };
            u64 numDelimiters = 1;
            expression = createExpressionFromTokens(&token, delimiters, numDelimiters, functionId, scope);
            if (expression.type == ek_invalid) {
                return expression;
            }
            if (token->type == tt_endl) {
                logErrorTokens(start, 1, "Unterminated parenthesis");
            }
            token++;
            break;
        }
        default: {
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

        // special handeling for these operators
        switch (op) {
            case tt_comma: {
                assert(false && "Not implemented comma operator yet");
            }
            case tt_dot: {
                assert(false && "Not implemented dot operator yet");
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

Expression boolCast(Expression* expression, Scope* scope, functionId functionId) {
    Expression newExpression = { 0 };
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
    Expression newExpression = { 0 };
    expressionAcutalType(expression, scope);
    bool eisNumber = isNumberType(expression->tid);
    bool tisNumber = isNumberType(type);
    TypeKind ekind = getTypeFromId(expression->tid)->kind;
    TypeKind tkind = getTypeFromId(type)->kind;
    if (eisNumber && tisNumber) {
        u64 esize = getTypeFromId(expression->tid)->numberSize;
        u64 tsize = getTypeFromId(type)->numberSize;
        if (ekind == tkind) {
            if (esize == tsize) {
                return *expression;
            }
            if (esize > tsize) {
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
