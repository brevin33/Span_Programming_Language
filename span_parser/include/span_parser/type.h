#pragma once
#include "span_parser/arena.h"
#include "span_parser/default.h"
#include "span_parser/tokens.h"


typedef struct _SpanFile SpanFile;
typedef struct _TemplateDefinition TemplateDefinition;
typedef struct _TemplateInstance TemplateInstance;
typedef struct _SpanProject SpanProject;

typedef enum _TypeKind : u8 {
    tk_invalid = 0,
    tk_int,
    tk_uint,
    tk_float,
    tk_bool,
    tk_interface,
} TypeKind;

typedef enum _TypeOrigin : u8 {
    to_invalid = 0,
    to_original,
    to_distinct,
    to_alias,
} TypeOrigin;

typedef struct _NumberType {
    u64 bits;
} NumberType;

typedef struct _Type Type;
typedef struct _Type {
    Arena arena;
    TypeKind kind;
    TypeOrigin origin;
    char* name;
    Type** alises;
    u64 alisesCount;
    Type* baseType;  // most of the time this is NULL
    TemplateDefinition* templateDefinition;
    union {  // anything bigger than a 64 bits should be a pointer to a struct
        NumberType number;
    };
} Type;

Type* TypeFromTokens(SpanFile* file, Token** tokens, bool logError);

Type* TypeCreateFromTokens(SpanFile* file, Token** tokens);

Type* TypeFromNameNamespaceTemplate(SpanProject* project, char* name, char* namespace, TemplateDefinition* templateDefinition);

Type* getNumberType(u64 bits, TypeKind kind);

Type* TypeCreateAlias(SpanFile* file, Type* baseType, const char* name);

Type* TypeCreateAliasGlobal(Type* baseType, const char* name);

Type* TypeCreateDistinct(SpanFile* file, Type* baseType, const char* name);

void freeType(Type* type);
