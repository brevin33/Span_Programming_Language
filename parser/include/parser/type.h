#pragma once

#include "parser/map.h"
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/tokens.h"

typedef u64 typeId;
typedef u64 sourceCodeId;
typedef u64 functionId;

typedef enum _TypeKind : u8 {
    tk_void,
    tk_int,
    tk_uint,
    tk_float,
    tk_pointer,
    tk_array,
    tk_struct,
    tk_union,
    tk_enum,
    tk_func,
    tk_list,
    tk_map,
    tk_slice,
} TypeKind;

typedef struct _StructData {
    u64 numFields;
    typeId* fields;
} StructData;

typedef struct _UnionData {
    u64 numFields;
    typeId* fields;
} UnionData;

typedef struct _ArrayData {
    typeId elementType;
    u64 size;
} ArrayData;

typedef struct _MapData {
    typeId keyType;
    typeId valueType;
} MapData;

typedef struct _Type {
    TypeKind kind;
    Arena* arena;
    char* name;
    union {
        void* data;
        typeId pointedToType;
        StructData* structData;
        ArrayData* arrayData;
        UnionData* unionData;
        MapData* mapData;
        u64 numberSize;
    };
    functionId* methods;
} Type;


extern Pool typePool;
extern map typeMap;


void setupDefaultTypes();

typeId getTypeIdFromTokens(Token** tokens);

typeId createType(TypeKind kind, char* name);

Type* getTypeFromId(typeId typeId);

typeId getTypeIdFromName(char* name);

typeId getPtrType(typeId id);

typeId getArrayType(typeId id, u64 size);

typeId getUnnamedStructType(typeId* id, u64 numFields);

typeId getUnnamedTaggedUnionType(typeId* id, u64 numFields);

typeId getRefType(typeId id);

typeId getListType(typeId id);

typeId getMapType(typeId val, typeId key);

typeId getIntType(u64 size);

typeId getFloatType(u64 size);

typeId getUintType(u64 size);

typeId getSliceType(typeId id);
