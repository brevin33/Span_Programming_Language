#include "parser/type.h"
#include "parser.h"
#include "parser/tokens.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


Arena* gTypesArena;
u64 gTypesCount = 0;
u64 gTypesCapacity = 0;
Type* gTypes = NULL;
map gTypeMap;

void addBaseTypes() {
    static bool exists = false;
    if (exists) return;
    exists = true;
    Type intType = { .kind = tk_int, .name = "i8", .numberSize = 1 };
    addType(&intType);
    intType.name = "i16";
    intType.numberSize = 2;
    addType(&intType);
    intType.name = "i32";
    intType.numberSize = 4;
    addType(&intType);
    intType.name = "i64";
    intType.numberSize = 8;
    addType(&intType);

    Type uintType = { .kind = tk_uint, .name = "u8", .numberSize = 1 };
    addType(&uintType);
    uintType.name = "u16";
    uintType.numberSize = 2;
    addType(&uintType);
    uintType.name = "u32";
    uintType.numberSize = 4;
    addType(&uintType);
    uintType.name = "u64";
    uintType.numberSize = 8;
    addType(&uintType);

    Type floatType = { .kind = tk_float, .name = "f32", .numberSize = 4 };
    addType(&floatType);
    floatType.name = "f64";
    floatType.numberSize = 8;
    addType(&floatType);

    Type voidType = { .kind = tk_void, .name = "void" };
    addType(&voidType);

    Type charType = { .kind = tk_uint, .name = "char", .numberSize = 1 };
    addType(&charType);

    Type boolType = { .kind = tk_int, .name = "bool", .numberSize = 1 };
    addType(&boolType);

    intType = (Type) { .kind = tk_int, .name = "int", .numberSize = 8 };
    addType(&intType);

    uintType = (Type) { .kind = tk_uint, .name = "uint", .numberSize = 8 };
    addType(&uintType);

    floatType = (Type) { .kind = tk_float, .name = "float", .numberSize = 8 };
    addType(&floatType);
    floatType.name = "double";
    floatType.numberSize = 8;
    addType(&floatType);

    Type constInt = { .kind = tk_const_int, .name = "__const_int", .numberSize = 8 };
    addType(&constInt);
    Type constUint = { .kind = tk_const_uint, .name = "__const_uint", .numberSize = 8 };
    addType(&constUint);
    Type constFloat = { .kind = tk_const_float, .name = "__const_float", .numberSize = 8 };
    addType(&constFloat);
    Type constString = { .kind = tk_const_string, .name = "__const_uint" };
    addType(&constString);
}

void addType(Type* type) {
    if (gTypes == NULL) {
        gTypesArena = createArena(1024 * 1024);
        gTypesCapacity = 64;
        gTypeMap = createMap();
        gTypes = arenaAlloc(gTypesArena, sizeof(Type) * gTypesCapacity);
        if (gTypes == NULL) {
            fprintf(stderr, "Failed to allocate memory for types\n");
            exit(EXIT_FAILURE);
        }
        gTypesCount = 0;
        gTypes[0].kind = tk_invalid;  // Initialize the first type as invalid
        gTypes[0].name = "__invalid";  // Initialize the first type name as NULL
        gTypesCount++;
    }
    if (gTypesCount >= gTypesCapacity) {
        gTypesCapacity *= 2;
        Type* newTypes = arenaAlloc(gTypesArena, sizeof(Type) * gTypesCapacity);
        if (newTypes == NULL) {
            fprintf(stderr, "Failed to allocate memory for resized types array\n");
            exit(EXIT_FAILURE);
        }
        memcpy(newTypes, gTypes, sizeof(Type) * gTypesCount);
        gTypes = newTypes;
    }
    gTypes[gTypesCount] = *type;
    u64 nameLength = strlen(type->name) + 1;
    gTypes[gTypesCount].name = arenaAlloc(gTypesArena, nameLength);
    memcpy(gTypes[gTypesCount].name, type->name, nameLength);
    if (mapGet(&gTypeMap, gTypes[gTypesCount].name) != NULL) {
        return;
    }
    mapSet(&gTypeMap, gTypes[gTypesCount].name, (void*)gTypesCount);
    gTypesCount++;
}

typeId getTypeIdFromName(char* name) {
    typeId* id = (typeId*)mapGet(&gTypeMap, name);
    if (id == NULL) {
        return 0;
    }
    return *id;
}

Type* getTypeFromId(typeId id) {
    if (id == 0) {
        return NULL;  // Invalid ID
    }
    if (id >= gTypesCount) {
        return NULL;  // Invalid ID
    }
    return &gTypes[id];  // Return the type with the given ID
}

typeId __getTypeIdFromToken(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_id) {
        return 0;  // Not a valid type token
    }
    char* typeName = token->str;
    typeId typeId = getTypeIdFromName(typeName);
    Type* type = getTypeFromId(typeId);
    if (type == NULL) {
        return 0;
    }
    token++;
    while (true) {
        switch (token->type) {
            case tt_mul: {
                typeId = getTypeIdPtr(typeId);
                token++;
                break;
            }
            case tt_bit_and: {
                typeId = getTypeIdRef(typeId);
                token++;
                break;
            }
            case tt_xor: {
                typeId = getTypeIdUptr(typeId);
                token++;
                break;
            }
            case tt_lbracket: {
                token++;
                if (token->type == tt_rbracket) {
                    typeId = getTypeIdList(typeId);
                    break;
                }
                if (token->type != tt_int || token->type != tt_id) {
                    return 0;
                }
                if (token->type == tt_id) {
                    if (strcmp(token->str, "_") == 0) {
                        // TODO: dynmically find the size of the array
                        assert(false && "Dynamic array size not implemented yet");
                        return 0;
                    } else {
                        return 0;
                    }
                }
                u64 size = getTokenInt(token);
                if (size == 0) {
                    return 0;  // Invalid size
                }
                typeId = getTypeIdArray(typeId, size);
                token++;
                if (token->type != tt_rbracket) {
                    return 0;
                }
                token++;
                break;
            }
            default: {
                *tokens = token;
                return getTypeIdFromName(typeName);
            }
        }
    }
}

typeId _getTypeIdFromToken(Token** tokens) {
    Token* token = *tokens;
    typeId elementsBuffer[256];
    u64 elementsCount = 0;
    while (true) {
        typeId type;
        if (token->type == tt_lparen) {
            type = getTypeIdFromToken(&token);
            if (token->type != tt_rparen) {
                return 0;  // Missing closing parenthesis
            }
            token++;
        } else {
            type = __getTypeIdFromToken(tokens);
        }
        if (type == 0) {
            return 0;  // Invalid type
        }
        elementsBuffer[elementsCount++] = type;
        if (token->type == tt_comma) {
            token++;
            continue;
        }
        break;
    }
    if (elementsCount == 1) {
        return elementsBuffer[0];
    }
    return getTypeIdUnamedStruct(elementsBuffer, elementsCount);
}

typeId getTypeIdFromToken(Token** tokens) {
    Token* token = *tokens;
    typeId elementsBuffer[256];
    u64 elementsCount = 0;
    while (true) {
        typeId type;
        if (token->type == tt_lparen) {
            type = getTypeIdFromToken(&token);
            if (token->type != tt_rparen) {
                return 0;  // Missing closing parenthesis
            }
            token++;
        } else {
            type = _getTypeIdFromToken(tokens);
        }
        if (type == 0) {
            return 0;  // Invalid type
        }
        elementsBuffer[elementsCount++] = type;
        if (token->type == tt_bit_or) {
            token++;
            continue;
        }
        break;
    }
    if (elementsCount == 1) {
        return elementsBuffer[0];
    }
    return getTypeIdUnamedEnum(elementsBuffer, elementsCount);
}

typeId getTypeIdPtr(typeId id) {
    Type* type = getTypeFromId(id);
    if (type == NULL) {
        return 0;
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "%s*", type->name);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_ptr;
    newType.name = nameCopy;
    newType.pointedToType = id;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdArray(typeId id, u64 size) {
    Type* type = getTypeFromId(id);
    if (type == NULL) {
        return 0;
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "%s[%llu]", type->name, size);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_array;
    newType.name = nameCopy;
    newType.arrayVals.elementType = id;
    newType.arrayVals.size = size;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdList(typeId id) {
    Type* type = getTypeFromId(id);
    if (type == NULL) {
        return 0;
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "list<%s>", type->name);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_list;
    newType.name = nameCopy;
    newType.listType = id;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdUptr(typeId id) {
    Type* type = getTypeFromId(id);
    if (type == NULL) {
        return 0;
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "%s^", type->name);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_uptr;
    newType.name = nameCopy;
    newType.pointedToType = id;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdRef(typeId id) {
    Type* type = getTypeFromId(id);
    if (type == NULL) {
        return 0;
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "%s&", type->name);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_ref;
    newType.name = nameCopy;
    newType.pointedToType = id;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdUnamedStruct(typeId* structTypes, u64 structTypesCount) {
    if (structTypesCount == 0) {
        return 0;  // No struct types provided
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "struct<%llu>", structTypesCount);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_struct;
    newType.name = nameCopy;
    newType.structVals.members = arenaAlloc(gTypesArena, sizeof(typeId) * structTypesCount);
    memcpy(newType.structVals.members, structTypes, sizeof(typeId) * structTypesCount);
    newType.structVals.memberCount = structTypesCount;
    addType(&newType);
    return gTypesCount - 1;
}

typeId getTypeIdUnamedEnum(typeId* enumTypes, u64 enumTypesCount) {
    if (enumTypesCount == 0) {
        return 0;  // No enum types provided
    }
    char newTypeName[256];
    snprintf(newTypeName, sizeof(newTypeName), "enum<%llu>", enumTypesCount);
    typeId existingTypeId = getTypeIdFromName(newTypeName);
    if (existingTypeId != 0) {
        return existingTypeId;
    }
    char* nameCopy = arenaAlloc(gTypesArena, strlen(newTypeName) + 1);
    memcpy(nameCopy, newTypeName, strlen(newTypeName) + 1);
    Type newType = { 0 };
    newType.kind = tk_enum;
    newType.name = nameCopy;
    newType.enumVals.enumValues = arenaAlloc(gTypesArena, sizeof(u64) * enumTypesCount);
    newType.enumVals.enumTypes = arenaAlloc(gTypesArena, sizeof(typeId) * enumTypesCount);
    memcpy(newType.enumVals.enumValues, enumTypes, sizeof(u64) * enumTypesCount);
    memcpy(newType.enumVals.enumTypes, enumTypes, sizeof(typeId) * enumTypesCount);
    newType.enumVals.enumCount = enumTypesCount;
    addType(&newType);
    return gTypesCount - 1;
}

void freeTypes() {
    arenaFree(gTypesArena);
}
