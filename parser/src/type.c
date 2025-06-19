#include "parser/type.h"
#include "parser.h"
#include "parser/pool.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


Pool typePool;
map typeMap;

void setupDefaultTypes() {
}

typeId createType(TypeKind kind, char* name) {
    assert(mapGet(&typeMap, name) == NULL);
    poolId id = poolNewItem(&typePool);
    Type* type = poolGetItem(&typePool, id);
    type->kind = kind;
    type->arena = createArena(2048);
    u64 nameSize = strlen(name) + 1;
    type->name = arenaAlloc(type->arena, nameSize);
    memcpy(type->name, name, nameSize);

    mapSet(&typeMap, name, (void*)id);
    return id;
}

typeId getTypeIdFromName(char* name) {
    typeId* val = (typeId*)mapGet(&typeMap, name);
    if (val != NULL) {
        return *val;
    }
    return BAD_ID;
}

Type* getTypeFromId(typeId typeId) {
    return poolGetItem(&typePool, typeId);
}

typeId getPtrType(typeId id) {
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    typeName[nameSize] = '*';
    typeName[nameSize + 1] = '\0';

    typeId ptrType = getTypeIdFromName(typeName);
    if (ptrType == BAD_ID) {
        ptrType = createType(tk_pointer, typeName);
        Type* ptrTypePtr = getTypeFromId(ptrType);
        ptrTypePtr->pointedToType = id;
        return ptrType;
    }
    return ptrType;
}

typeId getArrayType(typeId id, u64 size) {
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[%llu]", size);

    typeId arrayType = getTypeIdFromName(typeName);
    if (arrayType == BAD_ID) {
        arrayType = createType(tk_array, typeName);
        Type* arrayTypePtr = getTypeFromId(arrayType);
        ArrayData* arrayData = arenaAlloc(arrayTypePtr->arena, sizeof(ArrayData));
        arrayData->elementType = id;
        arrayData->size = size;
        arrayTypePtr->arrayData = arrayData;
        return arrayType;
    }
    return arrayType;
}

typeId getUnnamedStructType(typeId* id, u64 numFields) {
    char typeName[1024];
    sprintf(typeName, "__implStruct");
    u64 nameSize = strlen(typeName);
    for (u64 i = 0; i < numFields; i++) {
        typeName[nameSize++] = '_';
        Type* type = getTypeFromId(id[i]);
        u64 typeNameSize = strlen(type->name);
        memcpy(typeName + nameSize, type->name, typeNameSize);
        nameSize += typeNameSize;
    }

    typeId structType = getTypeIdFromName(typeName);
    if (structType == BAD_ID) {
        structType = createType(tk_struct, typeName);
        Type* structTypePtr = getTypeFromId(structType);
        StructData* structData = arenaAlloc(structTypePtr->arena, sizeof(StructData));
        structData->numFields = numFields;
        structData->fields = arenaAlloc(structTypePtr->arena, sizeof(typeId) * numFields);
        for (u64 i = 0; i < numFields; i++) {
            structData->fields[i] = id[i];
        }
        structTypePtr->structData = structData;
        return structType;
    }
    return structType;
}

typeId getUnnamedTaggedUnionType(typeId* id, u64 numFields) {
    char typeName[1024];
    sprintf(typeName, "__implUnion");
    u64 nameSize = strlen(typeName);
    for (u64 i = 0; i < numFields; i++) {
        typeName[nameSize++] = '_';
        Type* type = getTypeFromId(id[i]);
        u64 typeNameSize = strlen(type->name);
        memcpy(typeName + nameSize, type->name, typeNameSize);
        nameSize += typeNameSize;
    }

    typeId unionType = getTypeIdFromName(typeName);
    if (unionType == BAD_ID) {
        unionType = createType(tk_union, typeName);
        Type* unionTypePtr = getTypeFromId(unionType);
        UnionData* unionData = arenaAlloc(unionTypePtr->arena, sizeof(UnionData));
        unionData->numFields = numFields;
        unionData->fields = arenaAlloc(unionTypePtr->arena, sizeof(typeId) * numFields);
        for (u64 i = 0; i < numFields; i++) {
            unionData->fields[i] = id[i];
        }
        unionTypePtr->unionData = unionData;
        return unionType;
    }
    return unionType;
}

typeId getRefType(typeId id) {
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    typeName[nameSize] = '*';
    typeName[nameSize + 1] = '\0';

    typeId refType = getTypeIdFromName(typeName);
    if (refType == BAD_ID) {
        refType = createType(tk_pointer, typeName);
        Type* refTypePtr = getTypeFromId(refType);
        refTypePtr->pointedToType = id;
        return refType;
    }
    return refType;
}

typeId getListType(typeId id) {
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[]");

    typeId listType = getTypeIdFromName(typeName);
    if (listType == BAD_ID) {
        listType = createType(tk_list, typeName);
        Type* listTypePtr = getTypeFromId(listType);
        listTypePtr->pointedToType = id;
        return listType;
    }
    return listType;
}

typeId getMapType(typeId val, typeId key) {
    char typeName[1024];
    Type* type = getTypeFromId(val);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[%llu]", key);

    typeId mapType = getTypeIdFromName(typeName);
    if (mapType == BAD_ID) {
        mapType = createType(tk_map, typeName);
        Type* mapTypePtr = getTypeFromId(mapType);
        MapData* mapData = arenaAlloc(mapTypePtr->arena, sizeof(MapData));
        mapData->keyType = key;
        mapData->valueType = val;
        mapTypePtr->mapData = mapData;
        return mapType;
    }
    return mapType;
}

typeId getIntType(u64 size) {
    char typeName[1024];
    sprintf(typeName, "i%llu", size);

    typeId intType = getTypeIdFromName(typeName);
    if (intType == BAD_ID) {
        intType = createType(tk_int, typeName);
        Type* intTypePtr = getTypeFromId(intType);
        intTypePtr->numberSize = size;
        return intType;
    }
    return intType;
}

typeId getFloatType(u64 size) {
    assert(size == 64 || size == 32 || size == 16);
    char typeName[1024];
    sprintf(typeName, "f%llu", size);

    typeId floatType = getTypeIdFromName(typeName);
    if (floatType == BAD_ID) {
        floatType = createType(tk_float, typeName);
        Type* floatTypePtr = getTypeFromId(floatType);
        floatTypePtr->numberSize = size;
        return floatType;
    }
    return floatType;
}

typeId getUintType(u64 size) {
    char typeName[1024];
    sprintf(typeName, "u%llu", size);

    typeId uintType = getTypeIdFromName(typeName);
    if (uintType == BAD_ID) {
        uintType = createType(tk_uint, typeName);
        Type* uintTypePtr = getTypeFromId(uintType);
        uintTypePtr->numberSize = size;
        return uintType;
    }
    return uintType;
}

typeId getSliceType(typeId id) {
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[...]");

    typeId sliceType = getTypeIdFromName(typeName);
    if (sliceType == BAD_ID) {
        sliceType = createType(tk_slice, typeName);
        Type* sliceTypePtr = getTypeFromId(sliceType);
        sliceTypePtr->pointedToType = id;
        return sliceType;
    }
    return sliceType;
}

// return 0 if not a number
u64 getNumberSize(char* name) {
    if (name[0] != 'i' && name[0] != 'u' && name[0] != 'f') {
        return 0;
    }
    name++;
    u64 size = 0;
    while (name[0] >= '0' && name[0] <= '9') {
        u8 digit = name[0] - '0';
        if (size > (UINT64_MAX - digit) / 10) {
            return 0;  // overflow detected
        }
        size = size * 10 + digit;
        name++;
    }
    if (name[0] != '\0') {
        return 0;
    }
    return size;
}

typeId _getTypeIdFromTokesn(Token** tokens, bool allowComma, bool allowOr) {
    Token* token = *tokens;
    typeId tid = BAD_ID;
    if (token->type == tt_lparen) {
        token++;
        tid = getTypeIdFromTokens(&token);
        if (token->type != tt_rparen) {
            return BAD_ID;
        }
        token++;
    } else {
        if (token->type != tt_id) {
            return BAD_ID;
        }
        char* name = token->str;
        token++;
        tid = getTypeIdFromName(name);
        if (tid == BAD_ID) {
            // fallbacks if not found
            u64 numberSize = getNumberSize(name);
            if (numberSize > 0) {
                char firstChar = name[0];
                switch (firstChar) {
                    case 'i':
                        tid = getIntType(numberSize);
                        break;
                    case 'u':
                        tid = getUintType(numberSize);
                        break;
                    case 'f':
                        if (numberSize != 32 && numberSize != 64 && numberSize != 16) {
                            return BAD_ID;
                        }
                        tid = getFloatType(numberSize);
                        break;
                    default:
                        return BAD_ID;
                }
            }
        }
    }
    assert(tid != BAD_ID);
    while (true) {
        switch (token->type) {
            case tt_mul: {
                token++;
                tid = getPtrType(tid);
                break;
            }
            case tt_lbracket: {
                token++;
                if (token->type == tt_rbracket) {
                    token++;
                    tid = getListType(tid);
                    break;
                }
                if (token->type == tt_elips) {
                    token++;
                    if (token->type != tt_rbracket) {
                        return BAD_ID;
                    }
                    token++;
                    tid = getSliceType(tid);
                    break;
                }
                if (token->type != tt_int) {
                    return BAD_ID;
                }
                u64 num = getTokenInt(token);
                token++;
                if (token->type != tt_rbracket || num == 0) {
                    return BAD_ID;
                }
                token++;
                tid = getArrayType(tid, num);
                break;
            }
            case tt_lbrace: {
                token++;
                typeId key = getTypeIdFromTokens(&token);
                if (key == BAD_ID) {
                    return BAD_ID;
                }
                if (token->type != tt_rbrace) {
                    return BAD_ID;
                }
                tid = getMapType(tid, key);
                break;
            }
            case tt_bit_and: {
                token++;
                tid = getRefType(tid);
                break;
            }
            case tt_bit_or: {
                if (!allowOr) {
                    *tokens = token;
                    return tid;
                }
                typeId orTypes[128];
                u64 numOrTypes = 0;
                orTypes[numOrTypes++] = tid;
                while (token->type == tt_bit_or) {
                    token++;
                    typeId orType = _getTypeIdFromTokesn(&token, true, false);
                    if (orType == BAD_ID) {
                        return BAD_ID;
                    }
                    orTypes[numOrTypes++] = orType;
                }
                tid = getUnnamedTaggedUnionType(orTypes, numOrTypes);
                break;
            }
            case tt_comma: {
                if (!allowComma) {
                    *tokens = token;
                    return tid;
                }
                typeId andTypes[128];
                u64 numAndTypes = 0;
                andTypes[numAndTypes++] = tid;
                while (token->type == tt_comma) {
                    token++;
                    typeId andType = _getTypeIdFromTokesn(&token, false, allowOr);
                    if (andType == BAD_ID) {
                        return BAD_ID;
                    }
                    andTypes[numAndTypes++] = andType;
                }
                tid = getUnnamedStructType(andTypes, numAndTypes);
                break;
            }
            default: {
                *tokens = token;
                assert(tid != BAD_ID);
                return tid;
            }
        }
    }
}

typeId getTypeIdFromTokens(Token** tokens) {
    return _getTypeIdFromTokesn(tokens, true, true);
}
