#include "parser/type.h"
#include "parser.h"
#include "parser/pool.h"
#include <stdio.h>
#include <string.h>


Pool typePool;
map typeMap;
typeId constNumberType;
typeId boolType;
typeId typeType;

void setupDefaultTypes() {
    typeId invalidType = createType(tk_invalid, "invalid", 0);
    Type* invalidTypePtr = getTypeFromId(invalidType);
    invalidTypePtr->numberSize = 0;

    typeId typeType = createType(tk_type, "type", 0);
    Type* typeTypePtr = getTypeFromId(typeType);
    typeTypePtr->numberSize = 0;

    typeId i32Type = createType(tk_int, "i32", 0);
    Type* i32TypePtr = getTypeFromId(i32Type);
    i32TypePtr->numberSize = 32;
    aliasType(i32Type, "int");

    typeId f32Type = createType(tk_float, "f32", 0);
    Type* f32TypePtr = getTypeFromId(f32Type);
    f32TypePtr->numberSize = 32;
    aliasType(f32Type, "float");

    typeId u32Type = createType(tk_uint, "u32", 0);
    Type* u32TypePtr = getTypeFromId(u32Type);
    u32TypePtr->numberSize = 32;
    aliasType(u32Type, "uint");

    typeId f64Type = createType(tk_float, "f64", 0);
    Type* f64TypePtr = getTypeFromId(f64Type);
    f64TypePtr->numberSize = 64;
    aliasType(f64Type, "double");

    typeId f16Type = createType(tk_float, "f16", 0);
    Type* f16TypePtr = getTypeFromId(f16Type);
    f16TypePtr->numberSize = 16;
    aliasType(f16Type, "half");

    typeId i1Type = createType(tk_int, "i1", 0);
    Type* i1TypePtr = getTypeFromId(i1Type);
    i1TypePtr->numberSize = 1;

    boolType = createType(tk_int, "i1", 0);
    Type* boolTypePtr = getTypeFromId(boolType);
    boolTypePtr->numberSize = 1;

    typeId u8Type = createType(tk_uint, "u8", 0);
    Type* u8TypePtr = getTypeFromId(u8Type);
    u8TypePtr->numberSize = 8;
    aliasType(u8Type, "char");

    constNumberType = createType(tk_const_number, "__const_number", 0);
}

typeId createType(TypeKind kind, char* name, projectId pid) {
    poolId id = poolNewItem(&typePool);
    Type* type = poolGetItem(&typePool, id);
    memset(type, 0, sizeof(Type));
    type->kind = kind;
    type->arena = createArena(2048);
    u64 nameSize = strlen(name) + 1;
    type->name = arenaAlloc(type->arena, nameSize);
    memcpy(type->name, name, nameSize);
    type->pid = pid;

    TypeList** val = (TypeList**)mapGet(&typeMap, name);
    if (val == NULL) {
        TypeList* list = arenaAlloc(gArena, sizeof(TypeList));
        list->count = 1;
        list->capacity = 1;
        list->types = arenaAlloc(gArena, sizeof(typeId) * list->capacity);
        list->types[0] = id;
        mapSet(&typeMap, name, (void*)list);
    } else {
        TypeList* list = *val;
        if (list->count >= list->capacity) {
            list->types = arenaRealloc(gArena, list->types, sizeof(typeId) * list->capacity, sizeof(typeId) * list->capacity * 2);
            list->capacity *= 2;
        }
        list->types[list->count] = id;
        list->count++;
    }
    return id;
}

void aliasType(typeId id, char* name) {
    assert(id != 0);
    TypeList** val = (TypeList**)mapGet(&typeMap, name);
    if (val == NULL) {
        TypeList* list = arenaAlloc(gArena, sizeof(TypeList));
        list->count = 1;
        list->capacity = 1;
        list->types = arenaAlloc(gArena, sizeof(typeId) * list->capacity);
        list->types[0] = id;
        mapSet(&typeMap, name, (void*)list);
    } else {
        TypeList* list = *val;
        if (list->count >= list->capacity) {
            list->types = arenaRealloc(gArena, list->types, sizeof(typeId) * list->capacity, sizeof(typeId) * list->capacity * 2);
            list->capacity *= 2;
        }
        list->types[list->count] = id;
        list->count++;
    }
}
TypeList* getTypeListFromName(char* name) {
    TypeList** val = (TypeList**)mapGet(&typeMap, name);
    if (val == NULL) {
        return NULL;
    }
    return *val;
}

bool isNumberType(typeId typeId) {
    Type* type = getTypeFromId(typeId);
    return type->kind == tk_int || type->kind == tk_uint || type->kind == tk_float;
}

typeId getTypeIdFromName(char* name) {
    TypeList** val = (TypeList**)mapGet(&typeMap, name);
    if (val != NULL) {
        TypeList* list = *val;
        if (list->count > 1) {
            return BAD_ID;
        }
        return list->types[0];
    }
    return BAD_ID;
}

Type* getTypeFromId(typeId typeId) {
    return poolGetItem(&typePool, typeId);
}

typeId getPtrType(typeId id) {
    assert(id != 0);
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    typeName[nameSize] = '*';
    typeName[nameSize + 1] = '\0';

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->pointedToType == id) {
                return typeId;
            }
        }
    }
    typeId ptrType = createType(tk_pointer, typeName, 0);
    Type* ptrTypePtr = getTypeFromId(ptrType);
    ptrTypePtr->pointedToType = id;
    return ptrType;
}

typeId getArrayType(typeId id, u64 size) {
    assert(id != 0);
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[%llu]", size);

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->arrayData->elementType == id && type->arrayData->size == size) {
                return typeId;
            }
        }
    }
    typeId arrayType = createType(tk_array, typeName, 0);
    Type* arrayTypePtr = getTypeFromId(arrayType);
    ArrayData* arrayData = arenaAlloc(arrayTypePtr->arena, sizeof(ArrayData));
    arrayData->elementType = id;
    arrayData->size = size;
    arrayTypePtr->arrayData = arrayData;
    return arrayType;
}

char* getNumberAsString(u64 num, Arena* arena) {
    u64 numberOfDigits = 0;
    while (num > 0) {
        num /= 10;
        numberOfDigits++;
    }
    char* str = arenaAlloc(arena, numberOfDigits + 1);
    for (u64 i = 0; i < numberOfDigits; i++) {
        str[i] = '0' + (num % 10);
        num /= 10;
    }
    str[numberOfDigits] = '\0';
    return str;
}

typeId getUnnamedStructType(typeId* id, u64 numFields) {
    assert(id != 0);
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

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->structData->numFields == numFields) {
                bool isSame = true;
                for (u64 j = 0; j < numFields; j++) {
                    if (type->structData->fields[j] != id[j]) {
                        isSame = false;
                        break;
                    }
                }
                if (isSame) {
                    return typeId;
                }
            }
        }
    }
    typeId structType = createType(tk_struct, typeName, 0);
    Type* structTypePtr = getTypeFromId(structType);
    StructData* structData = arenaAlloc(structTypePtr->arena, sizeof(StructData));
    structData->numFields = numFields;
    structData->fields = arenaAlloc(structTypePtr->arena, sizeof(typeId) * numFields);
    structData->fieldNames = arenaAlloc(structTypePtr->arena, sizeof(char*) * numFields);
    for (u64 i = 0; i < numFields; i++) {
        structData->fields[i] = id[i];
        structData->fieldNames[i] = getNumberAsString(i, structTypePtr->arena);
    }
    structTypePtr->structData = structData;
    return structType;
}

typeId getUnnamedEnumType(typeId* id, u64 numFields) {
    assert(id != 0);
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

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->enumData->numFields == numFields) {
                bool isSame = true;
                for (u64 j = 0; j < numFields; j++) {
                    if (type->enumData->fields[j] != id[j]) {
                        isSame = false;
                        break;
                    }
                }
                if (isSame) {
                    return typeId;
                }
            }
        }
    }
    typeId enumType = createType(tk_union, typeName, 0);
    Type* enumTypePtr = getTypeFromId(enumType);
    EnumData* enumData = arenaAlloc(enumTypePtr->arena, sizeof(UnionData));
    enumData->numFields = numFields;
    enumData->fields = arenaAlloc(enumTypePtr->arena, sizeof(typeId) * numFields);
    for (u64 i = 0; i < numFields; i++) {
        enumData->fields[i] = id[i];
        enumData->fieldNames[i] = getNumberAsString(i, enumTypePtr->arena);
    }
    enumTypePtr->enumData = enumData;
    return enumType;
    return enumType;
}

typeId getRefType(typeId id) {
    assert(id != 0);
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    typeName[nameSize] = '&';
    typeName[nameSize + 1] = '\0';

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->pointedToType == id) {
                return typeId;
            }
        }
    }
    typeId refType = createType(tk_ref, typeName, type->pid);
    Type* refTypePtr = getTypeFromId(refType);
    refTypePtr->pointedToType = id;
    return refType;
}

typeId getListType(typeId id) {
    assert(id != 0);
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[]");

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->pointedToType == id) {
                return typeId;
            }
        }
    }
    typeId listType = createType(tk_list, typeName, 0);
    Type* listTypePtr = getTypeFromId(listType);
    listTypePtr->pointedToType = id;
    return listType;
}

typeId getMapType(typeId val, typeId key) {
    assert(val != 0 && key != 0);
    char typeName[1024];
    Type* type = getTypeFromId(val);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[%llu]", key);

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->mapData->keyType == key && type->mapData->valueType == val) {
                return typeId;
            }
        }
    }
    typeId mapType = createType(tk_map, typeName, 0);
    Type* mapTypePtr = getTypeFromId(mapType);
    MapData* mapData = arenaAlloc(mapTypePtr->arena, sizeof(MapData));
    mapData->keyType = key;
    mapData->valueType = val;
    mapTypePtr->mapData = mapData;
    return mapType;
}

typeId getIntType(u64 size) {
    char typeName[1024];
    sprintf(typeName, "i%llu", size);

    typeId intType = getTypeIdFromName(typeName);
    if (intType == BAD_ID) {
        intType = createType(tk_int, typeName, 0);
        Type* intTypePtr = getTypeFromId(intType);
        intTypePtr->numberSize = size;
        return intType;
    }
    assert(getTypeFromId(intType)->numberSize == size);
    return intType;
}

typeId getFloatType(u64 size) {
    assert(size == 64 || size == 32 || size == 16);
    char typeName[1024];
    sprintf(typeName, "f%llu", size);

    typeId floatType = getTypeIdFromName(typeName);
    if (floatType == BAD_ID) {
        floatType = createType(tk_float, typeName, 0);
        Type* floatTypePtr = getTypeFromId(floatType);
        floatTypePtr->numberSize = size;
        return floatType;
    }
    assert(getTypeFromId(floatType)->numberSize == size);
    return floatType;
}

typeId getUintType(u64 size) {
    char typeName[1024];
    sprintf(typeName, "u%llu", size);

    typeId uintType = getTypeIdFromName(typeName);
    if (uintType == BAD_ID) {
        uintType = createType(tk_uint, typeName, 0);
        Type* uintTypePtr = getTypeFromId(uintType);
        uintTypePtr->numberSize = size;
        return uintType;
    }
    assert(getTypeFromId(uintType)->numberSize == size);
    return uintType;
}

typeId getSliceType(typeId id) {
    assert(id != 0);
    char typeName[1024];
    Type* type = getTypeFromId(id);
    u64 nameSize = strlen(type->name);

    memcpy(typeName, type->name, nameSize);
    sprintf(typeName + nameSize, "[...]");

    TypeList* list = getTypeListFromName(typeName);
    if (list != NULL) {
        for (u64 i = 0; i < list->count; i++) {
            typeId typeId = list->types[i];
            Type* type = getTypeFromId(typeId);
            if (type->pointedToType == id) {
                return typeId;
            }
        }
    }
    typeId sliceType = createType(tk_slice, typeName, 0);
    Type* sliceTypePtr = getTypeFromId(sliceType);
    sliceTypePtr->pointedToType = id;
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
            } else {
                return BAD_ID;
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
                tid = getUnnamedEnumType(orTypes, numOrTypes);
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

typeId getActualTypeId(typeId typeId) {
    assert(typeId != 0);
    Type* type = getTypeFromId(typeId);
    while (type->kind == tk_ref) {
        typeId = type->pointedToType;
        type = getTypeFromId(typeId);
    }
    return typeId;
}

typeId getTypeIdFromTokens(Token** tokens) {
    return _getTypeIdFromTokesn(tokens, true, true);
}
