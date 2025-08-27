#pragma once


#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/llvm.h"

typedef struct _SpanTypeBase SpanTypeBase;

typedef enum _SpanTypeModifierType {
    tm_invalid = 0,
    tm_ptr,
    tm_ref,
    tm_uptr,
    tm_sptr,
    tm_array,
    tm_list,
    tm_slice,
} SpanTypeModifierType;

typedef struct _SpanTypeModifier {
    SpanTypeModifierType type;
    union {
        u64 arraySize;
    };
} SpanTypeModifier;

typedef struct _SpanType {
    SpanTypeBase* base;
    SpanTypeModifier mods[4];
    u64 modsCount;
} SpanType;

typedef enum _SpanTypeType {
    t_invalid = 0,
    t_struct,
    t_int,
    t_uint,
    t_void,
    t_float,
    t_function,
    t_numberic_literal,
} SpanTypeType;


typedef struct _SpanTypeStruct {
    SpanType* fields;
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
    SpanType returnType;
    SpanType* paramTypes;
    u64 paramTypesCount;
} SpanTypeFunction;

typedef struct _SpanTypeBase {
    SpanTypeType type;
    u32 namespace_;
    LLVMTypeRef llvmType;
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


SpanTypeBase* getFunctionType(SpanAst* ast);
SpanTypeBase* typeFromTypeAst(SpanAst* typeAst);
SpanTypeBase* findBaseType(char* name, u32 namespace_);
SpanTypeBase* addBaseType(SpanTypeBase* base);
SpanTypeBase* prototypeType(SpanAst* ast);
SpanTypeBase* prototypeStuctType(SpanAst* structAst);
void implementType(SpanTypeBase* type);
void implementStuctType(SpanTypeBase* structType);
SpanTypeBase* getIntTypeBase(u64 size);
SpanTypeBase* getFloatTypeBase(u64 size);
SpanTypeBase* getUintTypeBase(u64 size);
SpanTypeBase* getNumbericLiteralTypeBase();
SpanTypeBase* getInvalidTypeBase();
SpanTypeBase* getVoidTypeBase();
SpanType getVoidType();
SpanType getIntType(u64 size);
SpanType getFloatType(u64 size);
SpanType getUintType(u64 size);
SpanType getNumbericLiteralType();
SpanType getInvalidType();
SpanType getInvalidTypeAst(SpanAst* ast);
SpanType getType(SpanAst* ast);
bool isTypeEqual(SpanType* type1, SpanType* type2);
bool isTypeModifierEqual(SpanTypeModifier* mod1, SpanTypeModifier* mod2);

char* getTypeName(SpanType* type, char* buffer);

bool isTypeReference(SpanType* type);
bool isTypePointer(SpanType* type);
bool isTypeArray(SpanType* type);
bool isTypeSlice(SpanType* type);
bool isTypeList(SpanType* type);
bool isTypeStruct(SpanType* type);
bool isTypeFunction(SpanType* type);
bool isTypeNumbericLiteral(SpanType* type);
bool isTypeInvalid(SpanType* type);
bool isIntType(SpanType* type);
bool isUintType(SpanType* type);
bool isFloatType(SpanType* type);
bool isNumbericType(SpanType* type);

bool typeIsReferenceOf(SpanType* type, SpanType* otherType);

SpanType dereferenceType(SpanType* type);
SpanType getPointerType(SpanType* type);
SpanType getReferenceType(SpanType* type);

LLVMTypeRef getLLVMType(SpanType* type);
void createLLVMTypeBaseTypes();
void addLLVMTypeBaseType(SpanTypeBase* type);

SpanTypeModifier getModifier(SpanAst* ast);
