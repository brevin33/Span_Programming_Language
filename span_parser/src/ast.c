#include "span_parser.h"
#include "span_parser/tokens.h"

SpanAst createAst(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    ast.type = ast_file;

    ast.file = allocArena(arena, sizeof(SpanAstFile));

    ast.file->globalStatementsCount = 0;
    u64 globalStatementsCapacity = 2;
    ast.file->globalStatements = allocArena(arena, sizeof(SpanAst) * globalStatementsCapacity);

    while (token->type != tt_eof) {
        if (ast.file->globalStatementsCount >= globalStatementsCapacity) {
            ast.file->globalStatements = reallocArena(arena, sizeof(SpanAst) * globalStatementsCapacity * 2, ast.file->globalStatements, sizeof(SpanAst) * globalStatementsCapacity);
            globalStatementsCapacity *= 2;
        }
        ast.file->globalStatements[ast.file->globalStatementsCount++] = AstGeneralParse(arena, &token);
        while (token->type == tt_endl)
            token++;
    }

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstStructParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    Token* startToken = token;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_struct, "Should have struct");
    token++;
    ast.struct_ = allocArena(arena, sizeof(SpanAstStruct));

    if (token->type == tt_id) {
        char buffer[4096];
        tokenGetString(*token, buffer);
        u64 nameLength = strlen(buffer);
        ast.struct_->name = allocArena(arena, nameLength + 1);
        memcpy(ast.struct_->name, buffer, nameLength + 1);
        token++;
    } else {
        ast.struct_->name = NULL;
        logErrorTokens(token, 1, "Expected struct name");
    }

    while (token->type == tt_endl)
        token++;
    if (token->type != tt_lbrace) {
        logErrorTokens(token, 1, "Expected struct body");
        SpanAst err = { 0 };
        return err;
    }
    token++;
    while (token->type == tt_endl)
        token++;

    ast.struct_->fieldsCount = 0;
    u64 fieldsCapacity = 2;
    ast.struct_->fields = allocArena(arena, sizeof(SpanAstVariableDeclaration) * fieldsCapacity);

    while (token->type != tt_rbrace) {
        if (ast.struct_->fieldsCount >= fieldsCapacity) {
            ast.struct_->fields = reallocArena(arena, sizeof(SpanAstVariableDeclaration) * fieldsCapacity * 2, ast.struct_->fields, sizeof(SpanAstVariableDeclaration) * fieldsCapacity);
            fieldsCapacity *= 2;
        }
        SpanAst field = AstVariableDeclarationParse(arena, &token);
        if (field.type == ast_invalid) {
            // skip to the next field
            Token braceTokenStack[1024];
            u32 braceStack = 0;
            while (true) {
                if (token->type == tt_endl && braceStack == 0) break;
                if (token->type == tt_lbrace) {
                    braceStack++;
                    braceTokenStack[braceStack] = *token;
                }
                if (token->type == tt_rbrace) braceStack--;
                token++;
                if (token->type == tt_eof) {
                    logErrorTokens(startToken, 1, "left brace is never closed");
                    *tokens = token;
                    SpanAst err = { 0 };
                    return err;
                }
            }
        } else
            ast.struct_->fields[ast.struct_->fieldsCount++] = field;
        while (token->type == tt_endl)
            token++;
        if (token->type == tt_eof) {
            logErrorTokens(startToken, 1, "struct scope is never closed");
            *tokens = token;
            return ast;
        }
    }
    token++;

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

bool looksLikeType(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_id) {
        return false;
    }
    token++;
    while (true) {
        switch (token->type) {
            case tt_mul:
            case tt_bit_and:
            case tt_uptr:
            case tt_sptr:
                token++;
                break;
            case tt_lbracket:
                token++;
                if (token->type == tt_rbracket) {
                    token++;
                    break;
                }
                if (token->type == tt_elipsis) {
                    token++;
                    if (token->type == tt_rbracket) {
                        token++;
                        break;
                    } else {
                        token -= 2;
                        *tokens = token;
                        return true;
                    }
                }
                if (token->type == tt_int) {
                    token++;
                    if (token->type == tt_rbracket) {
                        token++;
                        break;
                    } else {
                        token -= 2;
                        *tokens = token;
                        return true;
                    }
                }
            default:
                *tokens = token;
                return true;
        }
    }
}

bool looksLikeFunctionDeclaration(Token** tokens) {
    Token* token = *tokens;
    if (!looksLikeType(&token)) {
        return false;
    }

    if (token->type != tt_id) {
        return false;
    }
    token++;
    if (token->type != tt_lparen) {
        return false;
    }
    token++;

    u32 parenStack = 1;
    while (true) {
        if (token->type == tt_lparen) {
            parenStack++;
        }
        if (token->type == tt_rparen) {
            parenStack--;
            if (parenStack == 0) {
                break;
            }
        }
        if (token->type == tt_endl) return false;
        token++;
    }
    token++;

    while (token->type == tt_endl)
        token++;

    if (token->type != tt_lbrace) {
        return false;
    }

    *tokens = token;
    return true;
}

static ProgramAction handelErrorFunctionParamParse(Token** tokens, Token firstParenToken) {
    Token parenStackTokenStack[1024];
    parenStackTokenStack[1] = firstParenToken;
    Token* token = *tokens;
    u32 parenStack = 1;
    while (true) {
        if (token->type == tt_rparen) parenStack--;
        if (token->type == tt_lparen) {
            parenStack++;
            parenStackTokenStack[parenStack] = *token;
        }
        if (token->type == tt_endl) break;
        if (token->type == tt_comma && parenStack == 1) break;
        if (parenStack == 0) break;
        if (token->type == tt_eof) {
            logErrorTokens(&parenStackTokenStack[parenStack], 1, "left paren is never closed");
            *tokens = token;
            return pa_return;
        }
        token++;
    }
    if (token->type == tt_endl) {
        if (parenStack != 0) {
            Token parenToken = parenStackTokenStack[parenStack];
            logErrorTokens(&parenToken, 1, "left paren is never closed");
        }
        *tokens = token;
        return pa_break;
    }
    if (parenStack == 0) {
        *tokens = token;
        return pa_break;
    }
    token++;
    *tokens = token;
    return pa_continue;
}

SpanAst AstFunctionParameterDeclarationParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    ast.type = ast_func_param;
    ast.funcParam = allocArena(arena, sizeof(SpanAstFunctionParameterDeclaration));

    if (token->type != tt_lparen) {
        logErrorTokens(token, 1, "Expected ( for function parameters");
        SpanAst err = { 0 };
        return err;
    }
    token++;

    Token firstParenToken = *token;
    if (token->type != tt_rparen) {
        u64 paramsCount = 0;
        u64 paramsCapacity = 2;
        ast.funcParam->params = allocArena(arena, sizeof(SpanAstVariableDeclaration) * paramsCapacity);
        while (true) {
            SpanAst param = AstVariableDeclarationParse(arena, &token);
            if (param.type == ast_invalid) {
                ProgramAction action = handelErrorFunctionParamParse(tokens, firstParenToken);
                if (action == pa_return) {
                    return param;
                } else if (action == pa_break) {
                    break;
                } else if (action == pa_continue) {
                    continue;
                }
            }
            if (paramsCount >= paramsCapacity) {
                ast.funcParam->params = reallocArena(arena, sizeof(SpanAstVariableDeclaration) * paramsCapacity * 2, ast.funcParam->params, sizeof(SpanAstVariableDeclaration) * paramsCapacity);
                paramsCapacity *= 2;
            }
            ast.funcParam->params[paramsCount++] = param;
            if (token->type == tt_rparen) break;
            if (token->type == tt_comma) {
                token++;
                continue;
            }
            if (token->type == tt_eof) {
                logErrorTokens(token, 1, "never closed function param list");
                SpanAst err = { 0 };
                *tokens = token;
                return err;
            }
            logErrorTokens(token, 1, "Expected comma or right paren");

            ProgramAction action = handelErrorFunctionParamParse(tokens, firstParenToken);
            if (action == pa_return) {
                SpanAst err = { 0 };
                return err;
            } else if (action == pa_break) {
                break;
            } else if (action == pa_continue) {
                continue;
            }
        }
    } else {
        ast.funcParam->paramsCount = 0;
        ast.funcParam->params = NULL;
    }
    massert(token->type == tt_rparen, "should be a left paren");
    token++;

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstScopeParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    ast.type = ast_scope;
    ast.scope = allocArena(arena, sizeof(SpanAstScope));

    if (token->type != tt_lbrace) {
        logErrorTokens(token, 1, "Expected scope body");
        SpanAst err = { 0 };
        return err;
    }
    token++;

    // TODO: parse scope
    // bs for testing
    u64 statementCapacity = 2;
    ast.scope->statements = allocArena(arena, sizeof(SpanAst) * statementCapacity);
    ast.scope->statementsCount = 0;
    while (token->type != tt_rbrace) {
        if (token->type == tt_endl) {
            token++;
            continue;
        }
        SpanAst statement = AstGeneralParse(arena, &token);
        if (statement.type == ast_invalid) {
            while (token->type != tt_endl)
                token++;
            continue;
        }
        if (ast.scope->statementsCount >= statementCapacity) {
            ast.scope->statements = reallocArena(arena, sizeof(SpanAst) * statementCapacity * 2, ast.scope->statements, sizeof(SpanAst) * statementCapacity);
            statementCapacity *= 2;
        }
        ast.scope->statements[ast.scope->statementsCount++] = statement;
    }
    massert(token->type == tt_rbrace, "should be a right brace");
    token++;
    ast.scope->statementsCount = 0;
    ast.scope->statements = NULL;

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstFunctionDeclarationParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    ast.functionDeclaration = allocArena(arena, sizeof(SpanAstFunctionDeclaration));
    ast.functionDeclaration->returnType = allocArena(arena, sizeof(SpanAst));
    *ast.functionDeclaration->returnType = AstTypeParse(arena, &token);
    if (ast.functionDeclaration->returnType->type == ast_invalid) {
        SpanAst err = { 0 };
        return err;
    }
    if (token->type != tt_id) {
        logErrorTokens(token, 1, "Expected function name");
        SpanAst err = { 0 };
        return err;
    }
    char buffer[4096];
    tokenGetString(*token, buffer);
    u64 nameLength = strlen(buffer);
    ast.functionDeclaration->name = allocArena(arena, nameLength + 1);
    memcpy(ast.functionDeclaration->name, buffer, nameLength + 1);
    token++;

    SpanAst paramList = AstFunctionParameterDeclarationParse(arena, &token);
    if (paramList.type == ast_invalid) {
        while (token->type == tt_lbrace)
            token++;
    }
    ast.functionDeclaration->paramList = allocArena(arena, sizeof(SpanAstFunctionParameterDeclaration));
    *ast.functionDeclaration->paramList = paramList;

    while (token->type == tt_endl)
        token++;

    SpanAst scope = AstScopeParse(arena, &token);
    if (scope.type == ast_invalid) {
        // TODO: figure out what to do here if anything
    }
    ast.functionDeclaration->body = allocArena(arena, sizeof(SpanAstScope));
    *ast.functionDeclaration->body = scope;

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstGeneralIdParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    Token* t = token;
    SpanAst ast = { 0 };
    massert(token->type == tt_id, "Should have id");

    if (looksLikeFunctionDeclaration(&t)) {  // use t because we don't want to move forward our tokens
        ast = AstFunctionDeclarationParse(arena, &token);
    } else {
        //TODO: expression
    }

    *tokens = token;
    return ast;
}

SpanAst AstGeneralParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    massert(token->type != tt_endl, "Should not have end line");
    SpanAst ast = { 0 };
    ast.token = token;
    switch (token->type) {
        case tt_id: {
            ast = AstGeneralIdParse(arena, &token);
            break;
        }
        case tt_struct: {
            ast = AstStructParse(arena, &token);
            break;
        }
        default: {
            massert(false, "Unexpected token");
            break;
        }
    }

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstVariableDeclarationParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    while (token->type == tt_endl)
        token++;
    SpanAst ast = { 0 };
    ast.type = ast_variable_declaration;
    ast.token = token;
    ast.variableDeclaration = allocArena(arena, sizeof(SpanAstVariableDeclaration));

    SpanAst type = AstTypeParse(arena, &token);
    if (type.type == ast_invalid) {
        return type;
    }

    if (token->type != tt_id) {
        logErrorTokens(token, 1, "Expected field name");
        SpanAst err = { 0 };
        return err;
    }
    char buffer[4096];
    tokenGetString(*token, buffer);
    u64 nameLength = strlen(buffer);
    ast.variableDeclaration->name = allocArena(arena, nameLength + 1);
    memcpy(ast.variableDeclaration->name, buffer, nameLength + 1);
    token++;

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTypeParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.type = ast_type;
    ast.token = token;
    ast.type_ = allocArena(arena, sizeof(SpanAstType));
    if (token->type != tt_id) {
        logErrorTokens(token, 1, "Expected type name");
        SpanAst err = { 0 };
        return err;
    }
    char buffer[4096];
    tokenGetString(*token, buffer);
    u64 nameLength = strlen(buffer);

    ast.type_->name = allocArena(arena, nameLength + 1);
    memcpy(ast.type_->name, buffer, nameLength + 1);
    token++;

    ast.type_->modsCount = 0;
    u64 modsCapacity = 2;
    ast.type_->mods = allocArena(arena, sizeof(SpanAst) * modsCapacity);

    while (true) {
        SpanAst tmod = AstTmodParse(arena, &token);
        if (tmod.type == ast_invalid) {
            break;
        }

        if (ast.type_->modsCount >= modsCapacity) {
            ast.type_->mods = reallocArena(arena, sizeof(SpanAst) * modsCapacity * 2, ast.type_->mods, sizeof(SpanAst) * modsCapacity);
            modsCapacity *= 2;
        }
        ast.type_->mods[ast.type_->modsCount++] = tmod;
    }

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTmodParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    switch (token->type) {
        case tt_mul: {
            ast = AstTmodPtrParse(arena, &token);
            break;
        }
        case tt_bit_and: {
            ast = AstTmodRefParse(arena, &token);
            break;
        }
        case tt_uptr: {
            ast = AstTmodUptrParse(arena, &token);
            break;
        }
        case tt_sptr: {
            ast = AstTmodSptrParse(arena, &token);
            break;
        }
        case tt_lbracket: {
            ast = AstTmodListLikeParse(arena, &token);
            break;
        }
        default: {
            // no error because we return error to show there is no more mods to parse
            return ast;
        }
    }
    if (ast.type == ast_invalid) {
        // no error because we return error to show there is no more mods to parse
        return ast;
    }
    *tokens = token;
    return ast;
}

SpanAst AstTmodPtrParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_mul, "Should have *");
    token++;
    ast.type = ast_tmod_ptr;
    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTmodListLikeParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_lbracket, "Should have [");
    token++;

    if (token->type == tt_rbracket) {
        ast.type = ast_tmod_list;
        token++;
    } else if (token->type == tt_elipsis) {
        ast.type = ast_tmod_slice;
        token++;
        if (token->type != tt_rbracket) {
            SpanAst err = { 0 };
            return err;
        }
        token++;
    } else if (token->type == tt_int) {
        ast.type = ast_tmod_array;
        u64 size = getTokenInt(*token);
        token++;
        if (token->type != tt_rbracket) {
            SpanAst err = { 0 };
            return err;
        }
        token++;
        ast.tmodArray.size = size;
    } else {
        SpanAst err = { 0 };
        return err;
    }

    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTmodRefParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_bit_and, "Should have &");
    token++;
    ast.type = ast_tmod_ref;
    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTmodUptrParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_uptr, "Should have ^");
    token++;
    ast.type = ast_tmod_uptr;
    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}

SpanAst AstTmodSptrParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_sptr, "Should have *");
    token++;
    ast.type = ast_tmod_sptr;
    ast.tokenLength = token - *tokens;
    *tokens = token;
    return ast;
}
