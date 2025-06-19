#include "parser.h"
#include "parser/tokens.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

Pool sourceCodePool;

bool looksLikeScope(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_lbrace) {
        return false;
    }
    token++;
    u64 braceStack = 1;
    while (braceStack > 0) {
        switch (token->type) {
            case tt_lbrace:
                braceStack++;
                break;
            case tt_rbrace:
                braceStack--;
                break;
            case tt_eof:
                return false;
            default:
                break;
        }
        token++;
    }
    *tokens = token;
    return true;
}

bool looksLikeTemplateList(Token** tokens) {
    Token* token = *tokens;

    if (token->type != tt_lt) {
        return false;
    }
    token++;

    int count = 0;
    while (token->type != tt_gt) {
        if (token->type != tt_id) {
            return false;
        }
        token++;
        if (token->type == tt_colon) {
            token++;
            if (token->type != tt_id) {
                return false;
            }
            token++;
        }
        count++;
    }
    token++;
    if (count == 0) {
        return false;
    }

    *tokens = token;
    return true;
}

bool looksLikeType(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_id) {
        return false;
    }
    token++;

    looksLikeTemplateList(&token);
    bool done = false;
    while (!done) {
        switch (token->type) {
            case tt_mul:
                token++;
                break;
            case tt_bit_and:
                token++;
                break;
            case tt_lbracket:
                token++;
                // dynamic size array
                if (token->type == tt_rbracket) {
                    token++;
                    break;
                }
                // fixed size array
                if (token->type == tt_int) {
                    token++;
                    if (token->type != tt_rbracket) {
                        return false;
                    }
                    break;
                }
                // auto size array
                if (token->type == tt_elips) {
                    token++;
                    if (token->type != tt_rbracket) {
                        return false;
                    }
                    break;
                }
                return false;
            case tt_lbrace:
                token++;
                // map from key: hashable to value of type before
                if (token->type == tt_rbrace) {
                    token++;
                    break;
                }
                // map with type specified
                if (looksLikeType(&token)) {
                    token++;
                    if (token->type != tt_rbrace) {
                        return false;
                    }
                    break;
                }
                return false;
            default:
                done = true;
                break;
        }
    }
    *tokens = token;
    return true;
}

bool looksLikeFunction(Token** tokens) {
    Token* token = *tokens;

    bool isExtern = false;
    if (token->type == tt_extern || token->type == tt_extern_c) {
        isExtern = true;
        token++;
    }

    // return type
    if (!looksLikeType(&token)) {
        return false;
    }
    // function name
    if (token->type != tt_id) {
        return false;
    }
    token++;
    // parameter list
    if (token->type != tt_lparen) {
        return false;
    }
    token++;
    while (token->type != tt_rparen) {
        // we already know it's a funciton if this closes so we don't care about what is inside
        if (token->type == tt_eof) {
            return false;
        }
        token++;
    }
    token++;

    if (!isExtern) {
        if (!looksLikeScope(&token)) {
            return false;
        }
    } else {
        if (token->type != tt_endl) {
            return false;
        }
    }

    *tokens = token;
    return true;
}

SourceCode* getSourceCodeFromId(sourceCodeId sourceCodeId) {
    return (SourceCode*)poolGetItem(&sourceCodePool, sourceCodeId);
}

void freeSourceCode(sourceCodeId sourceCodeId) {
    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
    freeArena(sourceCode->arena);
    freepoolId(&sourceCodePool, sourceCodeId);
}

void loadSourceCode(sourceCodeId sourceCodeId) {
    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);

    sourceCode->content = readFile(sourceCode->filename, sourceCode->arena);
    assert(sourceCode->content != NULL && "Failed to read file");

    u64 lineCapacity = 1024;
    sourceCode->lineCount = 0;
    sourceCode->lines = arenaAlloc(sourceCode->arena, sizeof(char*) * lineCapacity);
    sourceCode->lineLengths = arenaAlloc(sourceCode->arena, sizeof(u64) * lineCapacity);
    sourceCode->lines[sourceCode->lineCount] = sourceCode->content;

    u64 i = 0;
    while (sourceCode->content[i] != '\0') {
        if (sourceCode->content[i] == '\n') {
            sourceCode->lineLengths[sourceCode->lineCount] = sourceCode->content + i - sourceCode->lines[sourceCode->lineCount];
            sourceCode->lineCount++;
            if (sourceCode->lineCount >= lineCapacity) {
                sourceCode->lines = arenaRealloc(sourceCode->arena, sourceCode->lines, sizeof(char*) * lineCapacity, sizeof(char*) * lineCapacity * 2);
                sourceCode->lineLengths = arenaRealloc(sourceCode->arena, sourceCode->lineLengths, sizeof(u64) * lineCapacity, sizeof(u64) * lineCapacity * 2);
                lineCapacity *= 2;
            }
            sourceCode->lines[sourceCode->lineCount] = sourceCode->content + i + 1;
        }
        i++;
    }

    sourceCode->tokens = loadTokensFromFile(sourceCode->content, sourceCode->arena, 0);

    Token* token = sourceCode->tokens;
    while (token->type != tt_eof) {
        if (looksLikeFunction(&token)) {
            sourceCode->fucntionStarts = token;
        } else if (token->type == tt_endl) {
            token++;
        } else {
            logErrorTokens(token, 1, "Unexpected token in source code");
        }
    }
}

void reloadSourceCode(sourceCodeId sourceCodeId) {
    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
    char oldFilename[1024];
    u64 filenameLength = strlen(sourceCode->filename);
    assert(filenameLength < 1024);
    memcpy(oldFilename, sourceCode->filename, filenameLength + 1);

    arenaReset(sourceCode->arena);

    sourceCode->filename = arenaAlloc(sourceCode->arena, filenameLength + 1);
    memcpy(sourceCode->filename, oldFilename, filenameLength + 1);

    loadSourceCode(sourceCodeId);
}

sourceCodeId createSourceCode(const char* filename) {
    sourceCodeId sourceCodeId = poolNewItem(&sourceCodePool);
    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);

    sourceCode->arena = createArena(1024 * 1024);

    u64 filenameLength = strlen(filename);
    sourceCode->filename = arenaAlloc(sourceCode->arena, filenameLength + 1);
    memcpy(sourceCode->filename, filename, filenameLength + 1);

    loadSourceCode(sourceCodeId);

    return sourceCodeId;
}
