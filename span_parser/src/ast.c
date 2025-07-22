#include "span_parser.h"

SpanAst createAst(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    ast.type = ast_file;

    ast.file = allocArena(arena, sizeof(SpanAstFile));

    ast.file->globalStatementsCount = 0;
    ast.file->globalStatementsCapacity = 2;
    ast.file->globalStatements = allocArena(arena, sizeof(SpanAst) * ast.file->globalStatementsCapacity);

    while (token->type != tt_eof) {
        if (ast.file->globalStatementsCount >= ast.file->globalStatementsCapacity) {
            ast.file->globalStatements = reallocArena(arena, sizeof(SpanAst) * ast.file->globalStatementsCapacity * 2, ast.file->globalStatements, sizeof(SpanAst) * ast.file->globalStatementsCapacity);
            ast.file->globalStatementsCapacity *= 2;
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
    ast.struct_->fieldsCapacity = 2;
    ast.struct_->fields = allocArena(arena, sizeof(SpanAstStructField) * ast.struct_->fieldsCapacity);

    while (token->type != tt_rbrace) {
        if (ast.struct_->fieldsCount >= ast.struct_->fieldsCapacity) {
            ast.struct_->fields = reallocArena(arena, sizeof(SpanAstStructField) * ast.struct_->fieldsCapacity * 2, ast.struct_->fields, sizeof(SpanAstStructField) * ast.struct_->fieldsCapacity);
            ast.struct_->fieldsCapacity *= 2;
        }
        SpanAst field = AstStructFieldParse(arena, &token);
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

    *tokens = token;
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
        token++;
    }

    while (token->type == tt_endl)
        token++;

    if (token->type != tt_lbrace) {
        return false;
    }

    *tokens = token;
    return true;
}

SpanAst AstGeneralIdParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    SpanAst ast = { 0 };
    ast.token = token;
    massert(token->type == tt_id, "Should have id");

    if (looksLikeFunctionDeclaration(tokens)) {
        //TODO: function declaration
    } else {
        //TODO: expression
    }
}

SpanAst AstGeneralParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
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

SpanAst AstStructFieldParse(Arena arena, Token** tokens) {
    Token* token = *tokens;
    while (token->type == tt_endl)
        token++;
    SpanAst ast = { 0 };
    ast.type = ast_struct_field;
    ast.token = token;
    ast.structField = allocArena(arena, sizeof(SpanAstStructField));

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
    ast.structField->name = allocArena(arena, nameLength + 1);
    memcpy(ast.structField->name, buffer, nameLength + 1);
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
    ast.type_->modsCapacity = 2;
    ast.type_->mods = allocArena(arena, sizeof(SpanAst) * ast.type_->modsCapacity);

    while (true) {
        SpanAst tmod = AstTmodParse(arena, &token);
        if (tmod.type == ast_invalid) {
            break;
        }

        if (ast.type_->modsCount >= ast.type_->modsCapacity) {
            ast.type_->mods = reallocArena(arena, sizeof(SpanAst) * ast.type_->modsCapacity * 2, ast.type_->mods, sizeof(SpanAst) * ast.type_->modsCapacity);
            ast.type_->modsCapacity *= 2;
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
