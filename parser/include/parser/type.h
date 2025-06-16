#pragma once

#include "nice_ints.h"
#include "parser/tokens.h"
#include "parser/map.h"


typedef enum _TypeKind {
    tk_invalid = 0,
    tk_void,
    tk_int,
    tk_uint,
    tk_float,
    tk_struct,
    tk_enum,
    tk_ptr,
    tk_ref,
    tk_array,
    tk_list,
    tk_uptr,
    tk_sptr,
    tk_const_int,
    tk_const_uint,
    tk_const_float,
    tk_const_string,
} TypeKind;

typedef u64 typeId;

typedef struct _enumVals {
    u64* enumValues;  // array of enum values
    typeId* enumTypes;
    u64 enumCount;  // number of enum values
} EnumVals;

typedef struct _Struct {
    typeId* members;  // array of member offsets
    u64 memberCount;  // number of members
} StructVals;

typedef struct _ArrayVals {
    typeId elementType;  // type of the elements in the array
    u64 size;  // size of the array
} ArrayVals;

typedef struct _Type {
    TypeKind kind;
    char* name;
    union {
        StructVals structVals;  // for struct types
        EnumVals enumVals;  // for enum types
        typeId pointedToType;
        ArrayVals arrayVals;  // for array types
        typeId listType;
        u64 numberSize;
    };
} Type;

extern Arena* gTypesArena;
extern u64 gTypesCount;
extern u64 gTypesCapacity;
extern Type* gTypes;
extern map gTypeMap;

void addBaseTypes();

void addType(Type* type);

typeId getTypeIdFromName(char* name);

Type* getTypeFromId(typeId id);

typeId getTypeIdFromToken(Token** token);

typeId getTypeIdPtr(typeId id);

typeId getTypeIdArray(typeId id, u64 size);

typeId getTypeIdList(typeId id);

typeId getTypeIdUptr(typeId id);

typeId getTypeIdRef(typeId id);

typeId getTypeIdUnamedStruct(typeId* structTypes, u64 structTypesCount);

typeId getTypeIdUnamedEnum(typeId* enumTypes, u64 enumTypesCount);

void freeTypes();
