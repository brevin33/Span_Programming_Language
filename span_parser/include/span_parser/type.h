#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"

typedef struct _SpanTypeBase SpanTypeBase;

typedef enum _SpanTypeType : u8 {
    st_invalid = 0,
    st_struct,
    st_int,
    st_uint,
    st_float,
    st_function,
} SpanTypeType;

typedef struct _SpanTypeStruct {
    SpanTypeBase** fields;
    char** fieldsNames;
    u64 fieldsCount;
} SpanTypeStruct;

typedef struct _SpanTypeInt {
    u64 size;
} SpanTypeInt;

typedef struct _SpanTypeFloat {
    u64 size;
} SpanTypeFloat;

typedef struct _SpanTypeUint {
    u64 size;
} SpanTypeUint;

typedef struct _SpanTypeFunction {
    SpanTypeBase* returnType;
    SpanTypeBase** paramTypes;
    u64 paramTypesCount;
} SpanTypeFunction;

typedef struct _SpanTypeBase {
    SpanTypeType type;
    u32 namespace;
    char* name;
    SpanAst* ast;
    union {
        SpanTypeStruct struct_;
        SpanTypeInt int_;
        SpanTypeFloat float_;
        SpanTypeUint uint;
        SpanTypeFunction function;
    };
} SpanTypeBase;

typedef struct _SpanType {
    SpanTypeBase* base;
    SpanAst* mods;
    u64 modsCount;
} SpanType;

SpanTypeBase* getFunctionType(SpanAst* ast);
SpanTypeBase* typeFromTypeAst(SpanAst* typeAst);
SpanTypeBase* findBaseType(char* name, u32 namespace);
SpanTypeBase* addBaseType(SpanTypeBase* base);
SpanTypeBase* prototypeType(SpanAst* ast);
SpanTypeBase* prototypeStuctType(SpanAst* structAst);
void implementType(SpanTypeBase* type);
void implementStuctType(SpanTypeBase* structType);
SpanTypeBase* getIntType(u64 size);
SpanTypeBase* getFloatType(u64 size);
SpanTypeBase* getUintType(u64 size);
