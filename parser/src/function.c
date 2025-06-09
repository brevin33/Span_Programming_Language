#include "parser.h"
#include "parser/logging.h"
#include "parser/scope.h"
#include "parser/tokens.h"
#include "parser/type.h"
#include <assert.h>
#include <string.h>

Function* createFunctionFromTokens(Token* tokens, Project* project) {
    Token* token = tokens;

    typeId returnType = getTypeIdFromToken(&token);
    if (returnType == 0) {
        logErrorToken("Expected return type", project, token);
        return NULL;
    }

    if (token->type != tt_id) {
        logErrorToken("Expected function name", project, token);
        return NULL;
    }
    char* name = arenaAlloc(&project->arena, strlen(token->str) + 1);
    memcpy(name, token->str, strlen(token->str) + 1);
    token++;

    if (token->type != tt_lparen) {
        logErrorToken("Expected '(' after function name", project, token);
        return NULL;
    }
    token++;

    typeId* parameters = NULL;
    char** parameterNames = NULL;
    u64 parameterCount = 0;


    while (true) {
        if (token->type == tt_rparen) {
            token++;
            break;
        }

        typeId paramType = getTypeIdFromToken(&token);
        if (paramType == 0) {
            logErrorToken("Expected parameter type", project, token);
            return NULL;
        }
        char* paramName = NULL;
        if (token->type == tt_id) {
            paramName = arenaAlloc(&project->arena, strlen(token->str) + 1);
            memcpy(paramName, token->str, strlen(token->str) + 1);
            token++;
        } else {
            logErrorToken("Expected parameter name", project, token);
            return NULL;
        }
        if (parameterCount % 10 == 0) {
            parameters = arenaAlloc(&project->arena, sizeof(typeId) * (parameterCount + 10));
            parameterNames = arenaAlloc(&project->arena, sizeof(char*) * (parameterCount + 10));
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
            return NULL;
        }
    }

    Function* function = createFunction(returnType, name, parameters, parameterNames, parameterCount, tokens, project);
    return function;
}

Function* createFunction(typeId returnType, char* name, typeId* parameters, char** parameterNames, u64 parameterCount, Token* startToken, Project* project) {
    Function* function = arenaAlloc(&project->arena, sizeof(Function));
    function->returnType = returnType;
    function->name = name;
    function->parameters = parameters;
    function->parameterCount = parameterCount;
    function->startToken = startToken;
    if (project->functionCount % 10 == 0) {
        project->functions = arenaAlloc(&project->arena, sizeof(Function) * (project->functionCount + 10));
    }
    project->functions[project->functionCount++] = *function;
    return function;
}

void implementFunction(Function* function, Project* project) {
    function->scope = arenaAlloc(&project->arena, sizeof(Scope));
    memset(function->scope, 0, sizeof(Scope));
    for (u64 i = 0; i < function->parameterCount; i++) {
        Variable var = { 0 };
        var.name = function->parameterNames[i];
        var.type = function->parameters[i];
        addVariableToScope(function->scope, &var, project);
    }
    Token* token = function->startToken;
    while (token->type != tt_lbrace) {
        token++;
    }
    implementScope(function->scope, function, token, project);
}

Function* getFunctionFromNameAndParmeters(const char* name, typeId* parameters, u64 parameterCount, Project* project, Token* tokenForError) {
    for (u64 i = 0; i < project->functionCount; i++) {
        Function* func = &project->functions[i];
        if (strcmp(func->name, name) == 0 && func->parameterCount == parameterCount) {
            bool match = true;
            for (u64 j = 0; j < parameterCount; j++) {
                if (func->parameters[j] != parameters[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return func;
            }
        }
    }
    // now check for implisit casts
    // make sure that if multiple implicit casts are possible we error out
    Function* foundMatchs[10];
    u64 foundCount = 0;
    for (u64 i = 0; i < project->functionCount; i++) {
        Function* func = &project->functions[i];
        if (strcmp(func->name, name) == 0 && func->parameterCount == parameterCount) {
            bool match = true;
            for (u64 j = 0; j < parameterCount; j++) {
                if (!canStaticCast(parameters[j], func->parameters[j], project)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                if (foundCount >= 10) {
                    logErrorToken("Too many functions found with the same name and compatible parameters: %s with %llu parameters", project, tokenForError, name, parameterCount);
                    return NULL;
                }
                foundMatchs[foundCount++] = func;
            }
        }
    }
    if (foundCount == 0) {
        logErrorToken("Function not found: %s with %llu parameters that match the given input", project, tokenForError, name, parameterCount);
        return NULL;
    }
    if (foundCount > 1) {
        logErrorToken("Multiple functions found with the same name and compatible parameters: %s with %llu parameters", project, tokenForError, name, parameterCount);
        return NULL;
    }
    assert(foundCount == 1 && "Found more than one function with the same name and compatible parameters");
    return foundMatchs[0];
}

Function* getFunctionForBiop(BiopExpresstion* biop, Project* project) {
    // This function should return the function that matches the biop expression
    // For now, we will just return NULL as a placeholder
    return NULL;
}

bool canStaticCast(typeId from, typeId to, Project* project) {
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

    Function* func = getFunctionForBiop(biop, project);
    if (func != NULL) {
        return func->returnType;
    }

    logError("Unsupported operation between types: %s and %s", leftType->name, rightType->name);
    return 0;  // Error case
}
