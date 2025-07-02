#pragma once
#include "span_parser/arena.h"
#include "span_parser/default.h"
#include "span_parser/tokens.h"


typedef struct _SpanFile SpanFile;
typedef struct _TemplateDefinition TemplateDefinition;
typedef struct _TemplateInstance TemplateInstance;
typedef struct _SpanProject SpanProject;
typedef struct _Type Type;

typedef enum _TypeKind : u8 {
    tk_invalid = 0,
    tk_int,
    tk_uint,
    tk_float,
    tk_bool,
    tk_ptr,
    tk_ref,
    tk_list,
    tk_array,
    tk_map,
    tk_struct,
    tk_enum,
    tk_union,
    tk_interface,
    tk_alias,
    tk_distinct,
} TypeKind;

typedef struct _IntType {
    u64 bits;
} IntType;

typedef struct _UintType {
    u64 bits;
} UintType;

typedef struct _FloatType {
    u64 bits;
} FloatType;

typedef struct _BoolType {
    u64 bits;
} BoolType;

typedef struct _AliasType {
    Type* type;
} AliasType;

typedef struct _Type {
    TypeKind kind;
    char* _name;  // this will be null for stuff like ptrs. use get name function if you need the name
    union {
        IntType intType;
        UintType uintType;
        FloatType floatType;
        BoolType boolType;
        AliasType aliasType;  // no need for pointer as struct is 64 bits
    };
} Type;

Type* getNumberType(u64 bits, TypeKind kind);

Type* TypeCreateGloablAlias(Type* baseType, char* name);

char* TypeGetName(Type* type, char* buffer);
