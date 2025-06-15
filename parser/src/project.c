#include "parser.h"
#include "parser/expression.h"
#include "parser/logging.h"
#include "parser/tokens.h"
#include <stdio.h>
#include <string.h>


bool looksLikeType(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_id) return false;
    token++;
    while (true) {
        if (token->type == tt_mul || token->type == tt_bit_and || token->type == tt_xor) {
            token++;
            continue;
        }
        if (token->type == tt_lbracket) {
            token++;
            if (token->type == tt_rbracket) {
                token++;
                continue;
            } else if (token->type == tt_int) {
                token++;
                if (token->type != tt_rbracket) {
                    return false;
                }
                token++;
                continue;
            } else {
                return false;
            }
        }
        if (token->type == tt_comma) {
            token++;
            if (looksLikeType(&token)) {
                continue;
            }
            return false;
        }
        if (token->type == tt_bit_or) {
            token++;
            if (looksLikeType(&token)) {
                continue;
            }
            return false;
        }
        break;
    }
    *tokens = token;
    return true;
}

bool looksLikeFunction(Token** tokens) {
    Token* token = *tokens;
    if (token->type == tt_extern || token->type == tt_extern_c) {
        while (token->type != tt_endl)
            token++;
        *tokens = token;
        return true;
    }
    if (!looksLikeType(&token)) return false;
    if (token->type != tt_id) return false;
    token++;
    if (token->type != tt_lparen) return false;
    token++;
    u64 parStack = 1;
    while (true) {
        if (token->type == tt_lparen) {
            parStack++;
            token++;
            continue;
        }
        if (token->type == tt_rparen) {
            parStack--;
            token++;
            if (parStack == 0) break;
            continue;
        }
        if (token->type == tt_lbrace || token->type == tt_rbrace || token->type == tt_eof) {
            return false;
        }
        token++;
    }
    if (token->type != tt_lbrace) {
        return false;
    }
    *tokens = token;
    return true;
}

Project createProject(const char* folder) {
    addBaseTypes();
    Project project = { 0 };
    project.arena = createArena(1024 * 1024);
    u64 folderLength = 0;
    while (folder[folderLength] != '\0') {
        folderLength++;
    }
    project.directory = (char*)arenaAlloc(project.arena, folderLength + 1);
    memcpy(project.directory, folder, folderLength + 1);

    u64 fileCount = 0;
    char** filesInDir = listFilesInDirectory(project.directory, &fileCount, project.arena);
    if (!filesInDir) {
        logError("Failed to list files in directory");
        return project;
    }
    project.souceFileNames = filesInDir;
    project.sourceFiles = arenaAlloc(project.arena, sizeof(char*) * fileCount);
    project.sourceFileCount = fileCount;

    for (u64 fileNumber = 0; fileNumber < fileCount; fileNumber++) {
        char filePath[512];
        snprintf(filePath, sizeof(filePath), "%s/%s", project.directory, filesInDir[fileNumber]);
        char* fileContent = readFile(filePath, project.arena);
        if (!fileContent) {
            logError("Failed to read file content");
            continue;
        }
        project.sourceFiles[fileNumber] = fileContent;
    }

    project.tokens = loadTokensFromDirectory(project.sourceFiles, project.sourceFileCount, project.arena);
    if (!project.tokens) {
        logError("Failed to load tokens from directory: %s", folder);
        project.tokens = NULL;
        return project;
    }

    u64 funcitonStartCapacity = project.sourceFileCount * 10;
    project.functionStarts = arenaAlloc(project.arena, sizeof(Token*) * project.sourceFileCount * 10);
    project.functionStartCount = 0;
    Token* token = project.tokens;
    while (token->type != tt_eop) {
        Token* startToken = token;
        if (looksLikeFunction(&token)) {
            if (project.functionStartCount >= funcitonStartCapacity) {
                funcitonStartCapacity *= 2;
                Token** tempFunctionStarts = arenaAlloc(project.arena, sizeof(Token*) * funcitonStartCapacity);
                memcpy(tempFunctionStarts, project.functionStarts, sizeof(Token*) * project.functionStartCount);
                project.functionStarts = tempFunctionStarts;
            }
            project.functionStarts[project.functionStartCount] = startToken;
            project.functionStartCount++;
        } else if (token->type == tt_eof) {
            token++;
        } else if (token->type == tt_endl) {
            token++;
        } else if (token->type == tt_lbrace) {
            // Skip to the end of the scope
            Token startToken = *token;
            u64 braceCount = 1;
            token++;
            while (braceCount > 0 && token->type != tt_eof) {
                if (token->type == tt_lbrace) {
                    braceCount++;
                } else if (token->type == tt_rbrace) {
                    braceCount--;
                }
                token++;
            }
            if (token->type == tt_eof) {
                logErrorToken("Unmatched brace in file %s at line %u", &project, &startToken, project.souceFileNames[startToken.file], startToken.line);
            }
        } else {
            token++;
            logErrorToken("Unexpected token in file %s at line %u", &project, token, project.souceFileNames[token->file], token->line);
        }
    }

    functionId* functions = arenaAlloc(project.arena, sizeof(functionId) * 10);
    u64 functionCount = 0;
    u64 functionCapacity = 10;
    for (u64 i = 0; i < project.functionStartCount; i++) {
        functionId function = createFunctionFromTokens(project.functionStarts[i], &project);
        if (function == 0) {
            logErrorToken("Failed to create function from tokens", &project, project.functionStarts[i]);
        }
        if (functionCount >= functionCapacity) {
            functionCapacity *= 2;
            functionId* newFunctions = arenaAlloc(project.arena, sizeof(functionId) * functionCapacity);
            memcpy(newFunctions, functions, sizeof(functionId) * functionCount);
            functions = newFunctions;
        }
        functions[functionCount++] = function;
    }

    // Implement all functions
    for (u64 i = 0; i < functionCount; i++) {
        functionId funcId = functions[i];
        Function* function = getFunctionFromId(funcId);
        if (function->type == ft_extern || function->type == ft_extern_c) {
            continue;
        }
        implementFunction(funcId, &project);
    }

    return project;
}
