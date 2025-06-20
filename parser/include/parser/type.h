#pragma once

#include "parser/map.h"
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/template.h"
#include "parser/tokens.h"

typedef u64 typeId;
typedef u64 sourceCodeId;
typedef u64 functionId;
typedef u64 projectId;

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
    tk_interface,
    tk_const_number,
} TypeKind;

typedef struct _StructData {
    u64 numFields;
    typeId* fields;
    char** fieldNames;
} StructData;

typedef struct _EnumData {
    u64 numFields;
    typeId* fields;
    char** fieldNames;
} EnumData;

typedef struct _UnionData {
    u64 numFields;
    typeId* fields;
    char** fieldNames;
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
    u32 pid;
    Arena* arena;
    char* name;
    union {
        void* data;
        typeId pointedToType;
        StructData* structData;
        ArrayData* arrayData;
        UnionData* unionData;
        EnumData* enumData;
        MapData* mapData;
        u64 numberSize;
    };
    bool isTemplate;
    functionId* methods;
    u64 methodsCount;
    u64 methodsCapacity;
    functionId* functionsUsingThis;
    u64 functionsUsingThisCount;
    u64 functionsUsingThisCapacity;
    TemplateDefinition* templateDefinition;
} Type;

typedef struct _TypeList {
    typeId* types;
    u64 count;
    u64 capacity;
} TypeList;

extern Pool typePool;
extern map typeMap;
extern typeId constNumberType;

void setupDefaultTypes();

typeId getTypeIdFromTokens(Token** tokens);

typeId createType(TypeKind kind, char* name, projectId pid);

Type* getTypeFromId(typeId typeId);

typeId getTypeIdFromName(char* name);

bool isNumberType(typeId typeId);

TypeList* getTypeListFromName(char* name);

void aliasType(typeId typeId, char* name);

typeId getPtrType(typeId id);

typeId getArrayType(typeId id, u64 size);

typeId getUnnamedStructType(typeId* id, u64 numFields);

typeId getUnnamedEnumType(typeId* id, u64 numFields);

typeId getRefType(typeId id);

typeId getListType(typeId id);

typeId getMapType(typeId val, typeId key);

typeId getIntType(u64 size);

typeId getFloatType(u64 size);

typeId getUintType(u64 size);

typeId getSliceType(typeId id);
