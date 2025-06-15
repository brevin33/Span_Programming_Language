#include "parser/expression.h"
#include "parser.h"
#include "parser/tokens.h"
#include "parser/type.h"
#include <string.h>

Expresstion createSingleExpresstion(Token** tokens, functionId funcId, Scope* scope, Project* project) {
    Expresstion expression = { 0 };
    expression.type = et_error;
    Token* token = *tokens;

    switch (token->type) {
        case tt_id: {
            typeId type = getTypeIdFromToken(&token);
            if (type != 0) {
                expression.type = et_type;
                expression.tid = type;
                break;
            }
            char* name = token->str;
            token++;
            if (token->type == tt_lparen) {
                //TODO: make this have helper function for doing it with methods in the same spot
                Token* startToken = token - 1;  // Save the start token for error reporting
                FunctionExpresstion functionExpresstion = { 0 };
                token++;  // Skip the '('
                while (true) {
                    if (token->type == tt_rparen) {
                        token++;
                        break;
                    }
                    OurTokenType dels[] = { tt_comma, tt_rparen };
                    Expresstion parameter = createExpresstionFromTokensDels(&token, dels, 2, funcId, scope, project);
                    if (parameter.type == et_error) {
                        expression.type = et_error;
                        return expression;
                    }
                    if (functionExpresstion.parameterCount % 10 == 0) {
                        Expresstion* parameters = functionExpresstion.parameters;
                        functionExpresstion.parameters = arenaAlloc(project->arena, sizeof(Expresstion) * (functionExpresstion.parameterCount + 10));
                        if (parameters) {
                            memcpy(functionExpresstion.parameters, parameters, sizeof(Expresstion) * functionExpresstion.parameterCount);
                        }
                    }
                    functionExpresstion.parameters[functionExpresstion.parameterCount++] = parameter;
                    if (token->type == tt_comma) {
                        token++;
                        continue;
                    } else if (token->type == tt_rparen) {
                        token++;
                        break;
                    } else {
                        logErrorToken("Unexpected token in function parameters", project, token);
                        expression.type = et_error;
                        return expression;
                    }
                }
                typeId paramTypes[512];
                if (functionExpresstion.parameterCount > 512) {
                    logErrorToken("Too many parameters for function call: %s with %llu parameters", project, token, token->str, functionExpresstion.parameterCount);
                    expression.type = et_error;
                    return expression;
                }
                for (u64 i = 0; i < functionExpresstion.parameterCount; i++) {
                    paramTypes[i] = functionExpresstion.parameters[i].tid;
                }
                functionId fId = getFunctionFromNameAndParmeters(name, paramTypes, functionExpresstion.parameterCount, project, startToken);
                if (fId == 0) {
                    expression.type = et_error;
                    return expression;
                }
                Function* func = getFunctionFromId(fId);
                functionExpresstion.func = fId;

                expression.type = et_function;
                expression.tid = func->returnType;
                expression.functionExpresstion = arenaAlloc(project->arena, sizeof(FunctionExpresstion));
                memcpy(expression.functionExpresstion, &functionExpresstion, sizeof(FunctionExpresstion));
                break;
            }
            Variable* variable = getVariableFromScope(scope, name);
            if (variable == NULL) {
                logErrorToken("Variable not declared before use", project, token - 1);
                expression.type = et_error;
                return expression;
            }
            expression.type = et_variable;
            expression.variable = arenaAlloc(project->arena, strlen(name) + 1);
            memcpy(expression.variable, name, strlen(name) + 1);
            expression.tid = variable->type;
            break;
        }
        case tt_int: {
            expression.type = et_int;
            expression.tid = getTypeIdFromName("__const_uint");
            expression.number = arenaAlloc(project->arena, strlen(token->str) + 1);
            memcpy(expression.number, token->str, strlen(token->str) + 1);
            token++;
            break;
        }
        case tt_float: {
            expression.type = et_float;
            expression.tid = getTypeIdFromName("__const_float");
            expression.number = arenaAlloc(project->arena, strlen(token->str) + 1);
            memcpy(expression.number, token->str, strlen(token->str) + 1);
            token++;
            break;
        }
        case tt_string: {
        }
        default: {
            logErrorToken("Invalid token for expression: %s", project, token, tokenToString(token, NULL, 0));
            return expression;
        }
    }
    *tokens = token;
    return expression;
}

i64 biopPrecedence(OurTokenType type) {
    switch (type) {
        case tt_mul:
        case tt_div:
        case tt_mod:
            return 5;  // Multiplicative
        case tt_add:
        case tt_sub:
            return 6;  // Additive
        case tt_lshift:
        case tt_rshift:
            return 7;  // Shift
        case tt_lt:
        case tt_gt:
        case tt_leq:
        case tt_geq:
            return 8;  // Relational
        case tt_eq:
        case tt_neq:
            return 9;  // Equality
        case tt_bit_and:
            return 10;  // Bitwise AND
        case tt_xor:
            return 11;  // Bitwise XOR
        case tt_bit_or:
            return 12;  // Bitwise OR
        case tt_and:
            return 13;  // Logical AND
        case tt_or:
            return 14;  // Logical OR
        default:
            return -1;  // Invalid operator
    }
}

Expresstion _createExpresstionFromTokensDels(Token** tokens, OurTokenType* dels, u64 delsSize, functionId funcId, Scope* scope, Project* project, i64 prec) {
    Function* function = getFunctionFromId(funcId);
    Token* token = *tokens;
    Expresstion lval = createSingleExpresstion(&token, funcId, scope, project);
    if (lval.type == et_error) {
        return lval;
    }
    Expresstion rval = { 0 };
    while (true) {
        for (u64 i = 0; i < delsSize; i++) {
            if (token->type == dels[i]) {
                *tokens = token;
                return lval;
            }
        }
        OurTokenType op = token->type;
        i64 opPrec = biopPrecedence(op);
        if (opPrec < 0) {
            char buf[256];
            logErrorTokens("Invalid operator in expression: %s", project, token, 1, tokenToString(token, buf, sizeof(buf)));
            Expresstion expression = { 0 };
            expression.type = et_error;
            return expression;
        }
        if (opPrec >= prec) {
            *tokens = token;
            return lval;
        }
        token++;
        Expresstion rhs = _createExpresstionFromTokensDels(tokens, dels, delsSize, funcId, scope, project, opPrec);
        BiopExpresstion biop;
        biop.left = arenaAlloc(project->arena, sizeof(Expresstion));
        memcpy(biop.left, &lval, sizeof(Expresstion));
        biop.operator= op;
        biop.right = arenaAlloc(project->arena, sizeof(Expresstion));
        memcpy(biop.right, &rhs, sizeof(Expresstion));
        lval.type = et_biop;
        lval.tid = getBiopTypeResult(&biop, project);
        lval.biopExpresstion = arenaAlloc(project->arena, sizeof(BiopExpresstion));

        memcpy(lval.biopExpresstion, &biop, sizeof(BiopExpresstion));
        token++;
    }
}
Expresstion createExpresstionFromTokensDels(Token** tokens, OurTokenType* dels, u64 delsSize, functionId funcId, Scope* scope, Project* project) {
    return _createExpresstionFromTokensDels(tokens, dels, delsSize, funcId, scope, project, INT64_MAX);
}

Expresstion createExpresstionFromTokens(Token** tokens, OurTokenType del, functionId funcId, Scope* scope, Project* project) {
    return createExpresstionFromTokensDels(tokens, &del, 1, funcId, scope, project);
}
