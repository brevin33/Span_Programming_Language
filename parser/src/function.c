#include "parser/function.h"
#include "parser.h"
#include "parser/logging.h"
#include "parser/scope.h"
#include "parser/tokens.h"
#include "parser/type.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

Function* gFunctions = NULL;
u64 gFunctionCount = 0;
u64 gFunctionCapacity = 0;
map gNameToFunctionsWithSameName;

functionId createFunctionFromTokens(Token* tokens, Project* project) {
    Token* token = tokens;
    FunctionType functionType = ft_normal;
    if (token->type == tt_extern) {
        functionType = ft_extern;
    } else if (token->type == tt_extern_c) {
        functionType = ft_extern_c;
    }

    typeId returnType = getTypeIdFromToken(&token);
    if (returnType == 0) {
        logErrorToken("Expected return type", project, token);
        return 0;
    }

    if (token->type != tt_id) {
        logErrorToken("Expected function name", project, token);
        return 0;
    }
    char* name = arenaAlloc(project->arena, strlen(token->str) + 1);
    memcpy(name, token->str, strlen(token->str) + 1);
    token++;

    if (token->type != tt_lparen) {
        logErrorToken("Expected '(' after function name", project, token);
        return 0;
    }
    token++;

    typeId* parameters = NULL;
    char** parameterNames = NULL;
    u64 parameterCount = 0;
    bool variadic = false;

    while (true) {
        if (token->type == tt_rparen) {
            token++;
            break;
        }
        if (token->type == tt_elips) {
            variadic = true;
            token++;
            if (token->type != tt_rparen) {
                logErrorToken("Expected ')' after variadic parameter", project, token - 1);
                return 0;
            }
            break;
        }

        typeId paramType = getTypeIdFromToken(&token);
        if (paramType == 0) {
            logErrorToken("Expected parameter type", project, token);
            return 0;
        }
        char* paramName = NULL;
        if (token->type == tt_id) {
            paramName = arenaAlloc(project->arena, strlen(token->str) + 1);
            memcpy(paramName, token->str, strlen(token->str) + 1);
            token++;
        } else {
            logErrorToken("Expected parameter name", project, token);
            return 0;
        }
        if (parameterCount % 10 == 0) {
            parameters = arenaAlloc(project->arena, sizeof(typeId) * (parameterCount + 10));
            parameterNames = arenaAlloc(project->arena, sizeof(char*) * (parameterCount + 10));
        }
        parameters[parameterCount] = paramType;
        parameterNames[parameterCount++] = paramName;

        if (token->type == tt_comma) {
            token++;
            continue;
        } else if (token->type == tt_rparen) {
            token++;
            break;
        } else {
            logErrorToken("Unexpected token in function parameters", project, token);
            return 0;
        }
    }

    functionId functionId = createFunction(returnType, name, parameters, parameterNames, parameterCount, tokens, project, variadic, functionType);
    return functionId;
}

char* getMangledName(Function* function, Project* project) {
    char* baseName = function->name;
    u64 length = strlen(baseName);
    u64 capacity = length * 2;
    char* mangledName = arenaAlloc(project->arena, capacity + 1);
    memcpy(mangledName, baseName, length);
    for (u64 i = 0; i < function->parameterCount; i++) {
        char* parameterName = function->parameterNames[i];
        u64 parameterLength = strlen(parameterName);
        u64 ogLength = length;
        length += parameterLength + 1;
        if (length >= capacity) {
            capacity *= 2;
            char* newMangledName = arenaAlloc(project->arena, capacity * 2 + 1);
            memcpy(newMangledName, mangledName, length);
            mangledName = newMangledName;
        }
        mangledName[ogLength] = '_';
        memcpy(mangledName + ogLength + 1, parameterName, parameterLength);
    }
    mangledName[length] = '\0';
    return mangledName;
}

functionId createFunction(typeId returnType, char* name, typeId* parameters, char** parameterNames, u64 parameterCount, Token* startToken, Project* project, bool variadic, FunctionType type) {
    Function function;
    function.returnType = returnType;
    function.name = name;
    function.variadic = variadic;
    function.type = type;
    function.parameters = parameters;
    function.parameterNames = parameterNames;
    function.parameterCount = parameterCount;
    function.startToken = startToken;
    char* mangledName = getMangledName(&function, project);
    function.mangledName = mangledName;

    if (gFunctionCount >= gFunctionCapacity) {
        if (gFunctions == NULL) {
            gNameToFunctionsWithSameName = createMap();
            gFunctionCapacity = 1024;
            gFunctionCount = 1;
            gFunctions = arenaAlloc(project->arena, sizeof(Function) * gFunctionCapacity);
        } else {
            gFunctionCapacity *= 2;
            Function* newFunctions = arenaAlloc(project->arena, sizeof(Function) * gFunctionCapacity);
            if (newFunctions != NULL) {
                memcpy(newFunctions, gFunctions, sizeof(Function) * gFunctionCount);
                gFunctions = newFunctions;
            }
        }
    }
    gFunctions[gFunctionCount] = function;
    Function* inListPtr = &gFunctions[gFunctionCount];
    void* existing = mapGet(&gNameToFunctionsWithSameName, function.name);
    if (existing == NULL) {
        FunctionsWithSameName* functionsWithSameName = arenaAlloc(project->arena, sizeof(FunctionsWithSameName));
        functionsWithSameName->functions = arenaAlloc(project->arena, sizeof(functionId) * 1);
        functionsWithSameName->count = 1;
        functionsWithSameName->capacity = 1;
        mapSet(&gNameToFunctionsWithSameName, function.name, (void*)functionsWithSameName);
        functionsWithSameName->functions[0] = gFunctionCount;
    } else {
        FunctionsWithSameName* functionsWithSameName = *(FunctionsWithSameName**)existing;
        if (functionsWithSameName->count >= functionsWithSameName->capacity) {
            functionsWithSameName->capacity *= 2;
            void* newMemory = arenaAlloc(project->arena, sizeof(functionId) * functionsWithSameName->capacity);
            memcpy(newMemory, functionsWithSameName->functions, sizeof(functionId) * functionsWithSameName->count);
            functionsWithSameName->functions = newMemory;
        }
        Function* existingFunction = NULL;
        for (u64 i = 0; i < functionsWithSameName->count; i++) {
            if (strcmp(function.mangledName, gFunctions[functionsWithSameName->functions[i]].mangledName) == 0) {
                existingFunction = &gFunctions[functionsWithSameName->functions[i]];
                break;
            }
        }
        if (existingFunction != NULL) {
            logErrorToken("Multiple functions with the same name and parameters", project, startToken);
            return 0;
        }
        functionsWithSameName->functions[functionsWithSameName->count++] = gFunctionCount;
    }
    return gFunctionCount++;
}

void implementFunction(functionId funcId, Project* project) {
    Function* function = getFunctionFromId(funcId);
    function->scope = arenaAlloc(project->arena, sizeof(Scope));
    memset(function->scope, 0, sizeof(Scope));
    for (u64 i = 0; i < function->parameterCount; i++) {
        Variable var = { 0 };
        var.name = function->parameterNames[i];
        var.type = function->parameters[i];
        if (var.name != NULL) addVariableToScope(function->scope, &var, project);
    }
    Token* token = function->startToken;
    while (token->type != tt_lbrace) {
        token++;
    }
    implementScope(function->scope, funcId, token, project);
}

functionId getFunctionFromNameAndParmeters(const char* name, typeId* parameters, u64 parameterCount, Project* project, Token* tokenForError) {
    FunctionsWithSameName** functionsWithSameNamePtr = (FunctionsWithSameName**)mapGet(&gNameToFunctionsWithSameName, name);
    if (functionsWithSameNamePtr == NULL) {
        return 0;
    }
    FunctionsWithSameName* functionsWithSameNamer = *functionsWithSameNamePtr;
    for (u64 i = 0; i < functionsWithSameNamer->count; i++) {
        functionId functionId = functionsWithSameNamer->functions[i];
        Function* func = &gFunctions[functionId];
        if (func->parameterCount == parameterCount) {
            bool match = true;
            for (u64 j = 0; j < parameterCount; j++) {
                if (func->parameters[j] != parameters[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return functionId;
            }
        }
    }

    // now check for implisit casts
    // make sure that if multiple implicit casts are possible we error out
    functionId matchFunc = 0;
    for (u64 i = 0; i < functionsWithSameNamer->count; i++) {
        functionId functionId = functionsWithSameNamer->functions[i];
        Function* func = &gFunctions[functionId];
        if (func->parameterCount == parameterCount) {
            bool match = true;
            for (u64 j = 0; j < parameterCount; j++) {
                if (!canImplCast(parameters[j], func->parameters[j], project)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                if (matchFunc != 0) {
                    logErrorToken("Multiple functions found with the same name and compatible parameters: %s", project, tokenForError, name);
                    return 0;
                }
                matchFunc = functionId;
            }
        }
    }
    return matchFunc;
}

Function* getFunctionFromId(functionId functionId) {
    assert(functionId != 0);
    return &gFunctions[functionId];
}

functionId getFunctionForBiop(BiopExpresstion* biop, Project* project) {
    // This function should return the function that matches the biop expression
    // For now, we will just return NULL as a placeholder
    return 0;
}

bool canImplCast(typeId from, typeId to, Project* project) {
    if (from == to) {
        return true;  // No cast needed
    }
    Type* fromType = getTypeFromId(from);
    Type* toType = getTypeFromId(to);

    // const numbers can be cast to their non-const counterparts
    if (toType->kind == tk_int && fromType->kind == tk_const_int) {
        return true;
    }
    if (toType->kind == tk_int && fromType->kind == tk_const_uint) {
        return true;
    }
    if (toType->kind == tk_uint && fromType->kind == tk_const_uint) {
        return true;
    }
    if (toType->kind == tk_float && fromType->kind == tk_const_float) {
        return true;
    }
    if (toType->kind == tk_float && fromType->kind == tk_const_int) {
        return true;
    }
    if (toType->kind == tk_float && fromType->kind == tk_const_uint) {
        return true;
    }


    // can do for same type of number
    if (toType->kind == tk_int && fromType->kind == tk_int) {
        if (toType->numberSize >= fromType->numberSize) {
            return true;
        }
        return false;
    }

    if (toType->kind == tk_uint && fromType->kind == tk_uint) {
        if (toType->numberSize >= fromType->numberSize) {
            return true;
        }
        return false;
    }

    if (toType->kind == tk_float && fromType->kind == tk_float) {
        // we don't care about size for float, as long as they are both floats
        return true;
    }


    return false;
}

bool BiopIsMathematical(BiopExpresstion* biop) {
    switch (biop->operator) {
        case tt_add:
        case tt_sub:
        case tt_mul:
        case tt_div:
        case tt_mod:
            return true;
        default:
            return false;
    }
}

typeId getBiopTypeResult(BiopExpresstion* biop, Project* project) {
    Type* leftType = getTypeFromId(biop->left->tid);
    Type* rightType = getTypeFromId(biop->right->tid);

    if (BiopIsMathematical(biop)) {
        // constant number stuff
        if (leftType->kind == tk_const_int && rightType->kind == tk_const_uint) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_uint && rightType->kind == tk_const_int) {
            return biop->right->tid;
        }
        if (leftType->kind == tk_const_int && rightType->kind == tk_const_float) {
            return biop->right->tid;
        }
        if (leftType->kind == tk_const_uint && rightType->kind == tk_const_float) {
            return biop->right->tid;
        }
        if (leftType->kind == tk_const_float && rightType->kind == tk_const_int) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_float && rightType->kind == tk_const_uint) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_int && rightType->kind == tk_const_int) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_uint && rightType->kind == tk_const_uint) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_float && rightType->kind == tk_const_float) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_int && rightType->kind == tk_const_int) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_uint && rightType->kind == tk_const_uint) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_float && rightType->kind == tk_const_float) {
            return biop->left->tid;
        }
        if (leftType->kind == tk_const_int && rightType->kind == tk_int) {
            return biop->right->tid;
        }
        if (leftType->kind == tk_const_uint && rightType->kind == tk_uint) {
            return biop->right->tid;
        }
        if (leftType->kind == tk_const_float && rightType->kind == tk_float) {
            return biop->right->tid;
        }

        // variable number stuff
        if (leftType->kind == tk_int && rightType->kind == tk_int) {
            if (rightType->numberSize == leftType->numberSize) {
                return biop->left->tid;
            } else if (rightType->numberSize > leftType->numberSize) {
                return biop->right->tid;
            } else {
                return biop->left->tid;
            }
        }

        if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
            if (rightType->numberSize == leftType->numberSize) {
                return biop->left->tid;
            } else if (rightType->numberSize > leftType->numberSize) {
                return biop->right->tid;
            } else {
                return biop->left->tid;
            }
        }

        if (leftType->kind == tk_float && rightType->kind == tk_float) {
            if (rightType->numberSize == leftType->numberSize) {
                return biop->left->tid;
            } else if (rightType->numberSize > leftType->numberSize) {
                return biop->right->tid;
            } else {
                return biop->left->tid;
            }
        }


        if (biop->operator== tt_add || biop->operator== tt_sub) {
            if (leftType->kind == tk_ptr && rightType->kind == tk_int) {
                return biop->left->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_int && rightType->kind == tk_ptr) {
                return biop->right->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_ptr && rightType->kind == tk_uint) {
                return biop->left->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_uint && rightType->kind == tk_ptr) {
                return biop->right->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_const_int && rightType->kind == tk_ptr) {
                return biop->right->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_ptr && rightType->kind == tk_const_int) {
                return biop->left->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_ptr && rightType->kind == tk_float) {
                return biop->left->tid;  // Pointer type remains the same
            }
            if (leftType->kind == tk_float && rightType->kind == tk_ptr) {
                return biop->right->tid;  // Pointer type remains the same
            }
        }
    }

    functionId funcId = getFunctionForBiop(biop, project);
    Function* func = getFunctionFromId(funcId);
    if (funcId != 0) {
        return func->returnType;
    }

    logError("Unsupported operation between types: %s and %s", leftType->name, rightType->name);
    return 0;  // Error case
}
