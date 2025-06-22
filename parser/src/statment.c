#include "parser.h"


Statement createStatmentFromTokens(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    Statement statement = { 0 };


    Token* start = token;
    switch (token->type) {
        case tt_id: {
            bool hasAssignment = false;
            while (token->type != tt_endl) {
                if (assignLikeToken(token->type)) {
                    hasAssignment = true;
                    break;
                }
                token++;
            }
            token = start;
            if (hasAssignment) {
                statement = createAssignmentStatement(&token, functionId, scope);
                if (statement.kind == sk_invalid) {
                    return statement;
                }
                break;
            }
            statement = createExpressionStatement(&token, functionId, scope);
            if (statement.kind == sk_invalid) {
                return statement;
            }
            break;
        }
        case tt_return: {
            statement = createReturnStatement(&token, functionId, scope);
            if (statement.kind == sk_invalid) {
                return statement;
            }
            break;
        }
        case tt_break: {
            statement = createBreakStatement(&token, functionId, scope);
            if (statement.kind == sk_invalid) {
                return statement;
            }
            break;
        }
        case tt_if: {
            statement = createIfStatement(&token, functionId, scope);
            if (statement.kind == sk_invalid) {
                return statement;
            }
            break;
        }
        case tt_while: {
            statement = createWhileStatement(&token, functionId, scope);
            if (statement.kind == sk_invalid) {
                return statement;
            }
            break;
        }
        default: {
            logErrorTokens(token, 1, "Can't understand statement");
            return statement;
        }
    }

    *tokens = token;
    return statement;
}


Statement createBreakStatement(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    Token* start = token;
    assert(token->type == tt_break && "Not a break statement");
    token++;
    u64 breakAmount = 1;
    if (token->type == tt_int) {
        breakAmount = getTokenInt(token);
        token++;
    }
    if (token->type != tt_endl) {
        logErrorTokens(token, 1, "Expected end line after break");
        Statement statement = { 0 };
        statement.kind = sk_invalid;
        return statement;
    }
    Statement statement = { 0 };
    statement.kind = sk_break;
    Token* end = token;
    statement.tokens = start;
    statement.tokenCount = end - start;
    statement.breakAmount = breakAmount;
    *tokens = token;
    return statement;
}

Statement createWhileStatement(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    assert(token->type == tt_while && "Not a while statement");
    token++;
    OurTokenType delimiters[] = { tt_lbrace };
    Statement statement = { 0 };
    WhileStatementData* data = arenaAlloc(scope->arena, sizeof(WhileStatementData));
    data->condition = arenaAlloc(scope->arena, sizeof(Expression));
    *data->condition = createExpressionFromTokens(&token, delimiters, 1, functionId, scope);
    if (data->condition->type == ek_invalid) {
        return statement;
    }
    *data->condition = implicitCast(data->condition, boolType, scope, functionId);
    if (data->condition->type == ek_invalid) {
        return statement;
    }
    Scope* s = arenaAlloc(scope->arena, sizeof(Scope));
    *s = createScope(scope->function, scope->arena);
    s->isLoop = true;
    addChildToScope(scope, s);
    implementScope(s, &token);
    data->body = s;

    statement.kind = sk_while;
    statement.tokens = data->condition->token - 1;
    statement.tokenCount = data->condition->tokenCount + 1;
    statement.whileData = data;
    *tokens = token;
    return statement;
}

Statement createIfStatement(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    assert(token->type == tt_if && "Not an if statement");
    token++;
    OurTokenType delimiters[] = { tt_lbrace };
    Statement statement = { 0 };
    IfStatementData* data = arenaAlloc(scope->arena, sizeof(IfStatementData));
    data->condition = arenaAlloc(scope->arena, sizeof(Expression));
    *data->condition = createExpressionFromTokens(&token, delimiters, 1, functionId, scope);
    if (data->condition->type == ek_invalid) {
        return statement;
    }
    *data->condition = implicitCast(data->condition, boolType, scope, functionId);
    if (data->condition->type == ek_invalid) {
        return statement;
    }
    Scope* s = arenaAlloc(scope->arena, sizeof(Scope));
    *s = createScope(scope->function, scope->arena);
    addChildToScope(scope, s);
    implementScope(s, &token);
    data->body = s;

    while (token->type == tt_endl) {
        token++;
    }
    if (token->type == tt_else) {
        token++;
        while (token->type == tt_endl) {
            token++;
        }
        if (token->type != tt_lbrace) {
            logErrorTokens(token, 1, "Expected '{' after else");
            return statement;
        }
        Scope* s = arenaAlloc(scope->arena, sizeof(Scope));
        *s = createScope(scope->function, scope->arena);
        addChildToScope(scope, s);
        implementScope(s, &token);
        data->elseBody = s;
    } else {
        data->elseBody = NULL;
        token--;  // move back onto the endl
    }

    statement.kind = sk_if;
    statement.tokens = data->condition->token - 1;
    statement.tokenCount = data->condition->tokenCount + 1;
    statement.ifData = data;
    *tokens = token;
    return statement;
}

Statement createExpressionStatement(Token** tokens, functionId functionId, Scope* scope) {
    Statement statement = { 0 };
    Token* token = *tokens;
    OurTokenType delimiters[] = { tt_endl, tt_rbrace };
    Expression expression = createExpressionFromTokens(&token, delimiters, 2, functionId, scope);
    if (expression.type == ek_invalid) {
        return statement;
    }
    Expression* e = arenaAlloc(scope->arena, sizeof(Expression));
    *e = expression;
    statement.kind = sk_expression;
    statement.expressionData = e;
    *tokens = token;
    statement.kind = sk_expression;
    statement.tokens = expression.token;
    statement.tokenCount = expression.tokenCount;
    return statement;
}

Statement createReturnStatement(Token** tokens, functionId functionId, Scope* scope) {
    Statement statement = { 0 };
    Token* token = *tokens;
    assert(token->type == tt_return && "Not a return statement");
    token++;
    OurTokenType delimiters[] = { tt_endl, tt_rbrace };
    Expression expression = createExpressionFromTokens(&token, delimiters, 2, functionId, scope);
    if (expression.type == ek_invalid) {
        return statement;
    }
    Function* function = getFunctionFromId(functionId);
    typeId returnType = function->returnType;
    expression = implicitCast(&expression, returnType, scope, functionId);
    if (expression.type == ek_invalid) {
        return statement;
    }
    *tokens = token;
    statement.kind = sk_return;
    statement.tokens = expression.token - 1;
    statement.tokenCount = expression.tokenCount + 1;
    statement.expressionData = arenaAlloc(scope->arena, sizeof(Expression));
    *statement.expressionData = expression;
    return statement;
}

Statement createAssignmentStatement(Token** tokens, functionId functionId, Scope* scope) {
    Statement statement = { 0 };
    Token* token = *tokens;
    OurTokenType delimiters[] = { tt_comma, tt_assign };
    AssignmentStatementData* data = arenaAlloc(scope->arena, sizeof(AssignmentStatementData));
    data->numAssignee = 0;
    u64 expressionCapacity = 1;
    data->assignee = arenaAlloc(scope->arena, sizeof(Expression) * expressionCapacity);
    while (true) {
        Token* start = token;
        Expression expression;
        typeId type = getTypeIdFromTokens(&token);
        if (type != BAD_ID && token->type == tt_id) {
            // create variable
            char* name = token->str;
            bool exists = variableExistsInScope(scope, name);
            if (exists) {
                logErrorTokens(token, 1, "Variable already exists in scope");
            }
            addVariableToScope(scope, token, name, type);
            Variable* variable = getVariableByName(scope, name);
            expression.type = ek_variable;
            expression.variable = variable->name;
            expression.token = variable->token;
            expression.tokenCount = 1;
            expression.variable = variable->name;
            expression.tid = getRefType(type);
            token++;
        } else if (type != BAD_ID) {
            token = start;
            expression = createExpressionFromTokens(&token, delimiters, 2, functionId, scope);
        } else {
            expression = createExpressionFromTokens(&token, delimiters, 2, functionId, scope);
        }
        if (expression.type == ek_invalid) {
            return statement;
        }
        if (data->numAssignee >= expressionCapacity) {
            data->assignee = arenaRealloc(scope->arena, data->assignee, sizeof(Expression) * expressionCapacity, sizeof(Expression) * (expressionCapacity * 2));
            expressionCapacity *= 2;
        }
        data->assignee[data->numAssignee] = expression;
        data->numAssignee++;
        if (token->type == tt_comma) {
            token++;
            continue;
        }
        if (token->type == tt_assign) {
            token++;
            break;
        }
    }

    OurTokenType delimiters2[] = { tt_endl, tt_rbrace };
    Expression value = createExpressionFromTokens(&token, delimiters2, 2, functionId, scope);
    if (value.type == ek_invalid) {
        return statement;
    }
    if (data->numAssignee == 1) {
        value = implicitCast(&value, data->assignee[0].tid, scope, functionId);
        if (value.type == ek_invalid) {
            return statement;
        }
        data->values = arenaAlloc(scope->arena, sizeof(Expression));
        *data->values = value;
        data->numValues = 1;
        if (value.type == ek_invalid) {
            return statement;
        }
        *tokens = token;
        statement.kind = sk_assignment;
        statement.assignmentData = data;
        statement.tokens = data->assignee[0].token;
        Token* s = data->assignee[0].token;
        Token* e = data->values->token + data->values->tokenCount;
        statement.tokenCount = e - s;
        return statement;
    } else {
        if (value.type == ek_grouped_data) {
            GroupedData* groupedData = value.groupedData;
            if (data->numAssignee != groupedData->numFields) {
                Token* s = data->assignee[0].token;
                Token* e = value.token + value.tokenCount;
                u64 tokenCount = e - s;
                logErrorTokens(s, tokenCount, "number of values being doesn't match number of fields in struct");
            }
            data->values = arenaAlloc(scope->arena, sizeof(Expression) * data->numAssignee);
            data->numValues = data->numAssignee;
            for (u64 i = 0; i < data->numAssignee; i++) {
                Expression e = implicitCast(&groupedData->expressions[i], data->assignee[i].tid, scope, functionId);
                if (e.type == ek_invalid) {
                    return statement;
                }
                data->values[i] = e;
            }
            *tokens = token;
            statement.kind = sk_assignment;
            statement.assignmentData = data;
            statement.tokens = data->assignee[0].token;
            Token* s = data->assignee[0].token;
            Token* e = data->values[data->numValues - 1].token + data->values[data->numValues - 1].tokenCount;
            statement.tokenCount = e - s;
            return statement;
        } else if (getTypeFromId(value.tid)->kind == tk_struct) {
            Type* type = getTypeFromId(value.tid);
            u64 numFields = type->structData->numFields;
            if (data->numAssignee != numFields) {
                Token* s = data->assignee[0].token;
                Token* e = value.token + value.tokenCount;
                u64 tokenCount = e - s;
                logErrorTokens(s, tokenCount, "number of values being doesn't match number of fields in struct");
            }
            assert(false && "Not implemented");
            data->values = arenaAlloc(scope->arena, sizeof(Expression) * data->numAssignee);
            data->numValues = data->numAssignee;
            for (u64 i = 0; i < data->numAssignee; i++) {
                Expression getStructVal = getStructValue(&value, i, scope, data->assignee[i].token, data->assignee[i].tokenCount);
                Expression e = implicitCast(&getStructVal, data->assignee[i].tid, scope, functionId);
                if (e.type == ek_invalid) {
                    return statement;
                }
                data->values[i] = e;
            }
            *tokens = token;
            statement.kind = sk_assignment;
            statement.assignmentData = data;
            statement.tokens = data->assignee[0].token;
            Token* s = data->assignee[0].token;
            Token* e = data->values[data->numValues - 1].token + data->values[data->numValues - 1].tokenCount;
            statement.tokenCount = e - s;
            return statement;
        } else {
            Token* s = data->assignee[0].token;
            Token* e = value.token + value.tokenCount;
            u64 tokenCount = e - s;
            logErrorTokens(s, tokenCount, "number of values being doesn't match number of values on right hand side");
        }
    }

    statement.kind = sk_invalid;
    return statement;
}
