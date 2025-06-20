#include "parser.h"
#include "parser/map.h"
#include <string.h>

Pool functionPool;
map functionMap;

char* getFunctionMangledName(char* name, typeId* paramTypes, u64 numParams, Arena* arena) {
    u64 nameSize = strlen(name) + 1;
    u64 nameCapacity = nameSize + numParams * 10;
    char* mangledName = arenaAlloc(arena, nameCapacity);
    u64 i = 0;
    memcpy(mangledName, name, nameSize);
    i += nameSize;
    for (u64 j = 0; j < numParams; j++) {
        Type* type = getTypeFromId(paramTypes[j]);
        u64 typeNameSize = strlen(type->name);
        u64 newSize = i + typeNameSize + 1;
        while (newSize > nameCapacity) {
            mangledName = arenaRealloc(arena, mangledName, nameCapacity, nameCapacity * 2 + 1);
            nameCapacity *= 2;
        }
        mangledName[i++] = '_';
        memcpy(mangledName + i, type->name, typeNameSize);
        i += typeNameSize;
    }
    mangledName[i] = '\0';
    return mangledName;
}

Function* getFunctionFromId(functionId functionId) {
    return (Function*)poolGetItem(&functionPool, functionId);
}

FunctionList* getFunctionsByName(char* name) {
    FunctionList** val = (FunctionList**)mapGet(&functionMap, name);
    if (val == NULL) {
        return NULL;
    }
    return *val;
}

functionId createFunction(
    char* name, typeId returnType, typeId* paramTypes, char** paramNames, u64 numParams, bool isExtern, bool isExternC, Token* functionStart, TemplateDefinition* templateDefinition, typeId methodType, Arena* arena) {
    poolId id = poolNewItem(&functionPool);
    Function* function = poolGetItem(&functionPool, id);
    function->arena = arena;
    memset(function, 0, sizeof(Function));

    function->methodType = methodType;
    if (methodType != BAD_ID) {
        Type* type = getTypeFromId(methodType);
        if (type->methodsCount >= type->methodsCapacity) {
            type->methods = arenaRealloc(arena, type->methods, sizeof(functionId) * type->methodsCapacity, sizeof(functionId) * type->methodsCapacity * 2);
            type->methodsCapacity *= 2;
        }
        type->methods[type->methodsCount] = id;
        type->methodsCount++;
    }

    if (templateDefinition->numTypes > 0) {
        function->templateDefinition = arenaAlloc(arena, sizeof(TemplateDefinition));
        function->templateDefinition->numTypes = templateDefinition->numTypes;
        function->templateDefinition->interfaces = arenaAlloc(arena, sizeof(typeId) * templateDefinition->numTypes);
        memcpy(function->templateDefinition->interfaces, templateDefinition->interfaces, sizeof(typeId) * templateDefinition->numTypes);
        function->templateDefinition->templateTypeNames = arenaAlloc(arena, sizeof(char*) * templateDefinition->numTypes);
        for (u64 i = 0; i < templateDefinition->numTypes; i++) {
            u64 nameSize = strlen(templateDefinition->templateTypeNames[i]) + 1;
            function->templateDefinition->templateTypeNames[i] = arenaAlloc(arena, nameSize);
            memcpy(function->templateDefinition->templateTypeNames[i], templateDefinition->templateTypeNames[i], nameSize);
        }
    }

    function->isExtern = isExtern;
    char* mangledName = getFunctionMangledName(name, paramTypes, numParams, arena);
    function->mangledName = mangledName;
    function->isExternC = isExternC;
    u64 nameSize = strlen(name) + 1;
    function->name = arenaAlloc(arena, nameSize);
    memcpy(function->name, name, nameSize);
    function->returnType = returnType;

    if (numParams > 0) {
        function->paramTypes = arenaAlloc(arena, sizeof(typeId) * numParams);
        memcpy(function->paramTypes, paramTypes, sizeof(typeId) * numParams);
        function->paramNames = arenaAlloc(arena, sizeof(char*) * numParams);
        for (u64 i = 0; i < numParams; i++) {
            u64 nameSize = strlen(paramNames[i]) + 1;
            function->paramNames[i] = arenaAlloc(arena, nameSize);
            memcpy(function->paramNames[i], paramNames[i], nameSize);
        }
    }
    function->numParams = numParams;

    function->functionStart = functionStart;
    FunctionList** val = (FunctionList**)mapGet(&functionMap, name);
    if (val == NULL) {
        FunctionList* list = arenaAlloc(arena, sizeof(FunctionList));
        list->count = 0;
        list->capacity = 1;
        list->functions = arenaAlloc(arena, sizeof(functionId) * list->capacity);
        list->functions[0] = id;
        mapSet(&functionMap, name, (void*)list);
    } else {
        FunctionList* list = *val;
        if (list->count >= list->capacity) {
            list->functions = arenaRealloc(arena, list->functions, sizeof(functionId) * list->capacity, sizeof(functionId) * list->capacity * 2);
            list->capacity *= 2;
        }
        list->functions[list->count] = id;
        list->count++;
    }
    return id;
}

functionId prototypeFunction(Token** tokens) {
    Arena* funcArena = createArena(2048);
    Token* token = *tokens;
    bool isExtern = false;
    bool isExternC = false;
    Token* functionStart = token;
    if (token->type == tt_extern) {
        isExtern = true;
        token++;
    } else if (token->type == tt_extern_c) {
        isExternC = true;
        token++;
    }

    // return type
    typeId returnType = getTypeIdFromTokens(&token);
    typeId methodType = BAD_ID;
    if (returnType == BAD_ID) {
        logErrorTokens(token, 1, "Expected return type");
        return BAD_ID;
    }

    Token* methodTypeToken = token;
    methodType = getTypeIdFromTokens(&token);
    if (methodType != BAD_ID) {
        if (token->type == tt_dot) {
            token++;
        } else {
            // resest did not want to be a method
            methodType = BAD_ID;
            token = methodTypeToken;
        }
    }

    // function name
    if (token->type != tt_id) {
        logErrorTokens(token, 1, "Expected function name");
        return BAD_ID;
    }
    char* name = token->str;
    token++;

    // template definition
    TemplateDefinition templateDefinition = getTemplateDefinitionFromTokens(&token, funcArena, false);

    // parameter list
    if (token->type != tt_lparen) {
        logErrorTokens(token, 1, "Expected parameter list");
        return BAD_ID;
    }
    token++;
    u64 numParams = 0;
    typeId paramTypes[256];
    char* paramNames[256];
    if (token->type != tt_rparen) {
        while (true) {
            typeId paramType = getTypeIdFromTokens(&token);
            if (paramType == BAD_ID) {
                logErrorTokens(token, 1, "Expected parameter type");
                return BAD_ID;
            }
            if (token->type != tt_id) {
                logErrorTokens(token, 1, "Expected parameter name");
                return BAD_ID;
            }
            char* paramName = token->str;
            token++;
            if (token->type == tt_comma) {
                token++;
                continue;
            } else if (token->type == tt_rparen) {
                break;
            } else {
                logErrorTokens(token, 1, "Expected comma or right parenthesis");
                return BAD_ID;
            }
        }
    }
    token++;
    if (!isExtern && !isExternC) {
        if (token->type != tt_lbrace) {
            logErrorTokens(token, 1, "Expected function body");
            return BAD_ID;
        }
        token++;
    }
    *tokens = token;
    return createFunction(name, returnType, paramTypes, paramNames, numParams, isExtern, isExternC, functionStart, &templateDefinition, methodType, funcArena);
}

void implementFunction(functionId functionId) {
    Function* function = getFunctionFromId(functionId);
    Token* token = function->functionStart;
    while (token->type != tt_lbrace) {
        token++;
    }
    token++;
    function->scope = createScope(functionId, function->arena);
    for (u64 i = 0; i < function->numParams; i++) {
        addVariableToScope(&function->scope, function->functionStart, function->paramNames[i], function->paramTypes[i]);
    }
    implementScope(&function->scope, &token);
}
