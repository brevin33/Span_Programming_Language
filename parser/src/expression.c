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
            expression.variable = str;
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


    expression.tokenCount = token - start;
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
        default:
            return -1;  // error
    }
}



bool constNumberExpressionCanEvaluateToType(Expression* constNumber, typeId type) {
}


Expression createBiopExpression(Expression* left, Expression* right, OurTokenType operator, Scope * scope) {
    Expression expression = { 0 };
    expression.type = ek_biop;
    expression.biopData = arenaAlloc(scope->arena, sizeof(BiopData));
    expression.biopData->left = left;
    expression.biopData->right = right;
    expression.biopData->operator= operator;
    bool leftIsNumber = isNumberType(left->tid);
    bool rightIsNumber = isNumberType(right->tid);
    TypeKind leftKind = getTypeFromId(left->tid)->kind;
    TypeKind rightKind = getTypeFromId(right->tid)->kind;
    if (leftIsNumber && rightIsNumber) {
        u64 leftSize = getTypeFromId(left->tid)->numberSize;
        u64 rightSize = getTypeFromId(right->tid)->numberSize;
        if (leftKind == tk_int && rightKind == tk_int) {
            if (leftSize < rightSize) {
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
        return expression;
    }
    if (leftKind == tk_const_number && rightKind == tk_const_number) {
        expression.tid = constNumberType;
        return expression;
    }
    if (leftKind == tk_const_number && rightIsNumber) {
        // TODO: handel biop overloading and constant number
    }
    // TODO: handel biop overloading and constant number
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
        if (newPrecedence <= precedence) {
            *tokens = token;
            return left;
        }

        // 0 is special meaning right away and not nessecarily a expression on the right hand size
        if (newPrecedence == 0) {
            switch (token->type) {
                case tt_dot: {
                    token++;
                    if (left.type == ek_type) {
                        assert(false && "Not implemented typeinfo yet");
                    }
                    assert(false && "Not implemented member access or method call yet");
                    break;
                }
                default: {
                    crash();
                }
            }
            continue;
        }
        Expression right = getSingleExpressionFromTokens(&token, functionId, scope);
        if (right.type == ek_invalid) {
            return right;
        }
    }
}

Expression createExpressionFromTokens(Token** tokens, OurTokenType* delimiters, u64 numDelimiters, functionId functionId, Scope* scope) {
    return _createExpressionFromTokens(tokens, delimiters, numDelimiters, functionId, scope, INT64_MAX);
}
