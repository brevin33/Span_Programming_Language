#include "span_parser/type.h"
#include "span_parser.h"
#include "span_parser/ast.h"
#include "span_parser/utils.h"
#include <llvm-c/Core.h>


SpanTypeBase* addBaseType(SpanTypeBase* base) {
    if (context.baseTypesCount >= context.baseTypesCapacity) {
        context.baseTypes = reallocArena(context.arena, sizeof(SpanTypeBase) * context.baseTypesCapacity * 2, context.baseTypes, sizeof(SpanTypeBase) * context.baseTypesCapacity);
        context.baseTypesCapacity *= 2;
    }
    SpanTypeBase* b = allocArena(context.arena, sizeof(SpanTypeBase));
    *b = *base;
    b->llvmType = NULL;
    u64 index = context.baseTypesCount++;
    context.baseTypes[index] = b;

    SpanTypeBase* existing = findBaseType(base->name, base->namespace_);

    return b;
}

SpanTypeBase* findBaseType(char* name, u32 namespace_) {
    for (u64 i = 0; i < context.baseTypesCount; i++) {
        SpanTypeBase* base = context.baseTypes[i];
        bool validNamespace = base->namespace_ == namespace_;
        validNamespace = validNamespace || base->namespace_ == NO_NAMESPACE;
        bool validName = strcmp(base->name, name) == 0;
        if (validNamespace && validName) {
            return base;
        }
    }
    return NULL;
}

SpanTypeBase* getInvalidTypeBase() {
    SpanTypeBase base;
    base.type = t_invalid;
    base.namespace_ = NO_NAMESPACE;
    base.ast = NULL;
    base.name = "$invalid$";

    SpanTypeBase* existing = findBaseType(base.name, NO_NAMESPACE);
    if (existing != NULL) {
        return existing;
    }

    return addBaseType(&base);
}

SpanType getInvalidType() {
    SpanTypeBase* base = getInvalidTypeBase();
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}

SpanTypeModifier getModifier(SpanAst* ast) {
    SpanTypeModifier modifier;
    switch (ast->type) {
        case ast_tmod_array:
            modifier.type = tm_array;
            modifier.arraySize = ast->tmodArray.size;
            break;
        case ast_tmod_ptr:
            modifier.type = tm_ptr;
            break;
        case ast_tmod_ref:
            modifier.type = tm_ref;
            break;
        case ast_tmod_uptr:
            modifier.type = tm_uptr;
            break;
        case ast_tmod_sptr:
            modifier.type = tm_sptr;
            break;
        case ast_tmod_list:
            modifier.type = tm_list;
            break;
        case ast_tmod_slice:
            modifier.type = tm_slice;
            break;
        default:
            massert(false, "unknown type modifier");
            break;
    }
    return modifier;
}


SpanType getInvalidTypeAst(SpanAst* ast) {
    massert(ast->type == ast_type, "should be an invalid ast");
    SpanTypeBase* base = getInvalidTypeBase();
    SpanType type;
    type.base = base;
    for (u64 i = 0; i < ast->type_.modsCount; i++) {
        SpanTypeModifier modifier = getModifier(&ast->type_.mods[i]);
        type.mods[i] = modifier;
    }
    type.modsCount = ast->type_.modsCount;
    return type;
}

SpanType getType(SpanAst* ast) {
    massert(ast->type == ast_type, "should be a type");
    SpanTypeBase* base = typeFromTypeAst(ast);
    if (base == NULL) {
        return getInvalidTypeAst(ast);
    }
    SpanType type;
    type.base = base;
    type.modsCount = ast->type_.modsCount;
    for (u64 i = 0; i < type.modsCount; i++) {
        type.mods[i] = getModifier(&ast->type_.mods[i]);
    }

    return type;
}

LLVMTypeRef getLLVMType(SpanType* type) {
    LLVMTypeRef llvmType = type->base->llvmType;
    if (llvmType == NULL) {
        addLLVMTypeBaseType(type->base);
        llvmType = type->base->llvmType;
    }
    massert(type->base->type != t_invalid, "type is invalid");
    massert(type->base->type != t_numberic_literal, "type is numberic literal");
    if (type->modsCount != 0) {
        SpanTypeModifier* mod = &type->mods[type->modsCount - 1];
        switch (mod->type) {
            case tm_ptr:
            case tm_uptr:
            case tm_sptr:
                SpanType deref = *type;
                deref.modsCount--;
                return LLVMPointerType(getLLVMType(&deref), 0);
            case tm_ref: {
                SpanType derefType = dereferenceType(type);
                return LLVMPointerType(getLLVMType(&derefType), 0);
            }
            default:
                massert(false, "not implemented");
        }
    }
    return llvmType;
}

void createLLVMTypeBaseTypes() {
    for (u64 i = 0; i < context.baseTypesCount; i++) {
        SpanTypeBase* type = context.baseTypes[i];
        addLLVMTypeBaseType(type);
    }
}

bool isTypeEqual(SpanType* type1, SpanType* type2) {
    bool isEqual = type1->base == type2->base;
    isEqual = isEqual && type1->modsCount == type2->modsCount;
    for (u64 i = 0; i < type1->modsCount; i++) {
        isEqual = isEqual && isTypeModifierEqual(type1->mods + i, type2->mods + i);
    }
    return isEqual;
}

bool isTypeReference(SpanType* type) {
    if (type->modsCount == 0) return false;
    return type->mods[type->modsCount - 1].type == tm_ref;
}
bool isTypePointer(SpanType* type) {
    if (type->modsCount == 0) return false;
    return type->mods[type->modsCount - 1].type == tm_ptr;
}
bool isTypeArray(SpanType* type) {
    if (type->modsCount == 0) return false;
    return type->mods[type->modsCount - 1].type == tm_array;
}
bool isTypeSlice(SpanType* type) {
    if (type->modsCount == 0) return false;
    return type->mods[type->modsCount - 1].type == tm_slice;
}
bool isTypeList(SpanType* type) {
    if (type->modsCount == 0) return false;
    return type->mods[type->modsCount - 1].type == tm_list;
}
bool isTypeStruct(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_struct;
}
bool isTypeFunction(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_function;
}
bool isTypeNumbericLiteral(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_numberic_literal;
}
bool isTypeInvalid(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_invalid;
}

bool isIntType(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_int;
}

bool isNumbericType(SpanType* type) {
    return isIntType(type) || isUintType(type) || isFloatType(type) || isTypeNumbericLiteral(type);
}

bool isUintType(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_uint;
}

bool isFloatType(SpanType* type) {
    if (type->modsCount != 0) return false;
    return type->base->type == t_float;
}

bool typeIsReferenceOf(SpanType* type, SpanType* otherType) {
    if (!isTypeReference(type)) return false;
    SpanType derefType = dereferenceType(type);
    return isTypeEqual(&derefType, otherType);
}

char* getTypeName(SpanType* type, char* buffer) {
    char* baseName = type->base->name;
    u32 baseNameSize = strlen(baseName);
    memcpy(buffer, baseName, baseNameSize);
    u32 bufferIndex = baseNameSize;
    for (u64 i = 0; i < type->modsCount; i++) {
        SpanTypeModifier* mod = &type->mods[i];
        switch (mod->type) {
            case tm_array:
                buffer[bufferIndex++] = '[';
                u64 size = mod->arraySize;
                char* number = uintToString(size, buffer + bufferIndex);
                u32 numberSize = strlen(number);
                bufferIndex += numberSize;
                buffer[bufferIndex++] = ']';
                break;
            case tm_ptr:
                buffer[bufferIndex++] = '*';
                break;
            case tm_ref:
                buffer[bufferIndex++] = '&';
                break;
            case tm_uptr:
                buffer[bufferIndex++] = '^';
                break;
            case tm_sptr:
                buffer[bufferIndex++] = '\'';
                break;
            case tm_list:
                buffer[bufferIndex++] = '[';
                buffer[bufferIndex++] = ']';
                break;
            case tm_slice:
                buffer[bufferIndex++] = '[';
                buffer[bufferIndex++] = '.';
                buffer[bufferIndex++] = '.';
                buffer[bufferIndex++] = '.';
                buffer[bufferIndex++] = ']';
                break;
            default:
                massert(false, "unknown type modifier");
                break;
        }
    }
    buffer[bufferIndex] = '\0';
    return buffer;
}

bool isTypeModifierEqual(SpanTypeModifier* mod1, SpanTypeModifier* mod2) {
    if (mod1->type != mod2->type) return false;
    switch (mod1->type) {
        case tm_array:
            return mod1->arraySize == mod2->arraySize;
        case tm_ptr:
        case tm_ref:
        case tm_uptr:
        case tm_sptr:
        case tm_list:
        case tm_slice:
            return true;
        default:
            return false;
    }
}

void addLLVMTypeBaseType(SpanTypeBase* type) {
    if (type->llvmType != NULL) return;
    switch (type->type) {
        case t_int:
            type->llvmType = LLVMIntType(type->int_.size);
            break;
        case t_uint:
            type->llvmType = LLVMIntType(type->uint.size);
            break;
        case t_void:
            type->llvmType = LLVMVoidType();
            break;
        case t_float:
            switch (type->float_.size) {
                case 32:
                    type->llvmType = LLVMFloatType();
                    break;
                case 64:
                    type->llvmType = LLVMDoubleType();
                    break;
                case 16:
                    type->llvmType = LLVMHalfType();
                    break;
                default:
                    massert(false, "not implemented");
                    break;
            }
            break;
        case t_function: {
            SpanTypeFunction* functionType = &type->function;
            SpanType* returnType = &functionType->returnType;
            LLVMTypeRef returnLLVMType = getLLVMType(returnType);
            LLVMTypeRef paramTypes[BUFFER_SIZE];
            for (u64 i = 0; i < functionType->paramTypesCount; i++) {
                SpanType paramType = functionType->paramTypes[i];
                paramTypes[i] = getLLVMType(&paramType);
            }
            type->llvmType = LLVMFunctionType(returnLLVMType, paramTypes, functionType->paramTypesCount, 0);
            break;
        }
        case t_struct: {
            SpanTypeStruct* structType = &type->struct_;
            LLVMTypeRef structTypes[BUFFER_SIZE];
            for (u64 i = 0; i < structType->fieldsCount; i++) {
                SpanType* fieldType = &structType->fields[i];
                LLVMTypeRef fieldLLVMType = getLLVMType(fieldType);
                structTypes[i] = fieldLLVMType;
            }
            type->llvmType = LLVMStructType(structTypes, structType->fieldsCount, 0);
            break;
        }
        case t_numberic_literal: {
            // should never instantiate this
            type->llvmType = NULL;
            break;
        }
        case t_invalid:
            // should never instantiate this
            type->llvmType = NULL;
            break;
        default:
            massert(false, "unknown type type");
            break;
    }
}

SpanTypeBase* prototypeStuctType(SpanAst* structAst) {
    massert(structAst->type == ast_struct, "should be a struct");
    SpanTypeBase base;
    base.type = t_struct;
    base.namespace_ = context.activeProject->namespace_;
    base.ast = structAst;
    base.name = structAst->struct_.name;

    return addBaseType(&base);
}

SpanType getIntType(u64 size) {
    SpanTypeBase* base = getIntTypeBase(size);
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}
SpanType getFloatType(u64 size) {
    SpanTypeBase* base = getFloatTypeBase(size);
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}
SpanType getUintType(u64 size) {
    SpanTypeBase* base = getUintTypeBase(size);
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}

SpanType getNumbericLiteralType() {
    SpanTypeBase* base = getNumbericLiteralTypeBase();
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}

SpanTypeBase* getNumbericLiteralTypeBase() {
    SpanTypeBase base;
    base.type = t_numberic_literal;
    base.namespace_ = NO_NAMESPACE;
    base.ast = NULL;
    base.name = "$numberic_literal$";
    SpanTypeBase* existing = findBaseType(base.name, NO_NAMESPACE);
    if (existing != NULL) {
        return existing;
    }
    return addBaseType(&base);
}

SpanTypeBase* getVoidTypeBase() {
    SpanTypeBase base;
    base.type = t_void;
    base.namespace_ = NO_NAMESPACE;
    base.ast = NULL;
    base.name = "void";
    SpanTypeBase* existing = findBaseType(base.name, NO_NAMESPACE);
    if (existing != NULL) {
        return existing;
    }
    return addBaseType(&base);
}
SpanType getVoidType() {
    SpanTypeBase* base = getVoidTypeBase();
    SpanType type;
    type.base = base;
    type.modsCount = 0;
    return type;
}

SpanTypeBase* typeFromTypeAst(SpanAst* typeAst) {
    massert(typeAst->type == ast_type, "should be a type");
    char* typeName = typeAst->type_.name;

    if (strcmp(typeName, "void") == 0) {
        return getVoidTypeBase();
    }

    char firstChar = typeName[0];
    bool isNumber = firstChar == 'i' || firstChar == 'u' || firstChar == 'f';
    isNumber = isNumber && stringIsUint(typeName + 1);
    if (isNumber) {
        u64 size = stringToUint(typeName + 1);
        if (firstChar == 'i') return getIntTypeBase(size);
        if (firstChar == 'u') return getUintTypeBase(size);
        if (firstChar == 'f') return getFloatTypeBase(size);
    }

    SpanTypeBase* existing = findBaseType(typeName, context.activeProject->namespace_);
    if (existing != NULL) {
        return existing;
    }
    logErrorTokens(typeAst->token, 1, "type not found");
    return NULL;
}

SpanTypeBase* prototypeType(SpanAst* ast) {
    if (ast->type == ast_struct) {
        return prototypeStuctType(ast);
    }
    massert(false, "should be a type definition ast");
    return NULL;
}

void implementType(SpanTypeBase* type) {
    if (type->type == t_struct) {
        SpanTypeBase* base = context.baseTypes[0];
        int i = 0;
        implementStuctType(type);
    } else {
        massert(false, "should be a type of struct, enum, or interface");
    }
}

void implementStuctType(SpanTypeBase* structType) {
    massert(structType->type == t_struct, "should be a struct");
    SpanAst* ast = structType->ast;
    u64 fieldsCapacity = 2;

    structType->struct_.fields = allocArena(context.arena, sizeof(SpanType) * fieldsCapacity);
    structType->struct_.fieldsNames = allocArena(context.arena, sizeof(char*) * fieldsCapacity);
    structType->struct_.fieldsCount = 0;
    for (u64 i = 0; i < ast->struct_.fieldsCount; i++) {
        SpanAst* field = &ast->struct_.fields[i];
        massert(field->type == ast_variable_declaration, "should be a variable declaration");
        SpanAst* type = field->variableDeclaration.type;
        massert(type->type == ast_type, "should be a type");
        char* name = field->variableDeclaration.name;
        SpanType base = getType(type);
        if (isTypeReference(&base)) {
            logErrorAst(type, "can't have reference type as a field");
            base = getInvalidType();
        }
        if (structType->struct_.fieldsCount >= fieldsCapacity) {
            structType->struct_.fields = reallocArena(context.arena, sizeof(SpanTypeBase*) * fieldsCapacity * 2, structType->struct_.fields, sizeof(SpanTypeBase*) * fieldsCapacity);
            fieldsCapacity *= 2;
        }
        u64 index = structType->struct_.fieldsCount++;
        structType->struct_.fields[index] = base;
        structType->struct_.fieldsNames[index] = name;
    }
}

SpanTypeBase* getIntTypeBase(u64 size) {
    SpanTypeBase base;
    base.type = t_int;
    base.namespace_ = NO_NAMESPACE;
    base.int_.size = size;
    base.ast = NULL;

    char buffer[BUFFER_SIZE];
    buffer[0] = 'i';
    uintToString(size, buffer + 1);

    SpanTypeBase* existing = findBaseType(buffer, NO_NAMESPACE);
    if (existing != NULL) {
        return existing;
    }

    u64 nameLength = strlen(buffer);
    base.name = allocArena(context.arena, nameLength + 1);
    memcpy(base.name, buffer, nameLength + 1);

    return addBaseType(&base);
}

SpanTypeBase* getFunctionType(SpanAst* ast) {
    massert(ast->type == ast_function_declaration, "should be a function");
    SpanAstFunctionDeclaration* functionDeclaration = &ast->functionDeclaration;

    char* returnTypeName = functionDeclaration->returnType->type_.name;
    u32 returnTypeNameSize = strlen(returnTypeName);
    char buffer[BUFFER_SIZE];
    u32 bufferIndex = 0;
    memcpy(buffer, returnTypeName, returnTypeNameSize);
    bufferIndex += returnTypeNameSize;
    buffer[bufferIndex++] = '$';
    buffer[bufferIndex++] = 'f';
    buffer[bufferIndex++] = 'n';
    buffer[bufferIndex++] = '(';
    SpanAst* paramList = functionDeclaration->paramList;
    massert(paramList->type == ast_func_param, "should be a scope");
    SpanAstFunctionParameterDeclaration* params = &paramList->funcParam;
    if (params != NULL) {
        for (u64 i = 0; i < params->paramsCount; i++) {
            SpanAst* param = &params->params[i];
            char* paramName = param->variableDeclaration.name;
            SpanAst* type = param->variableDeclaration.type;
            massert(type->type == ast_type, "should be a type");
            SpanAstType* typeType = &type->type_;
            char* paramTypeName = typeType->name;
            u32 paramTypeNameSize = strlen(paramTypeName);
            memcpy(buffer + bufferIndex, paramTypeName, paramTypeNameSize);
            bufferIndex += paramTypeNameSize;
            if (i != params->paramsCount - 1) buffer[bufferIndex++] = ',';
        }
    }
    buffer[bufferIndex++] = ')';
    buffer[bufferIndex++] = 0;

    SpanTypeBase* existing = findBaseType(buffer, context.activeProject->namespace_);
    if (existing != NULL) {
        return existing;
    }
    char* name = allocArena(context.arena, bufferIndex);
    memcpy(name, buffer, bufferIndex);

    SpanTypeBase base;
    base.type = t_function;
    base.namespace_ = context.activeProject->namespace_;
    base.ast = ast;
    base.name = name;
    base.function.returnType = getType(functionDeclaration->returnType);
    if (params == NULL) {
        base.function.paramTypes = NULL;
        base.function.paramTypesCount = 0;
    } else {
        if (params->paramsCount > 0) base.function.paramTypes = allocArena(context.arena, sizeof(SpanType) * params->paramsCount);
        else
            base.function.paramTypes = NULL;
        base.function.paramTypesCount = params->paramsCount;
        for (u64 i = 0; i < params->paramsCount; i++) {
            SpanAst* param = &params->params[i];
            massert(param->type == ast_variable_declaration, "should be a variable declaration");
            base.function.paramTypes[i] = getType(param->variableDeclaration.type);
        }
    }
    return addBaseType(&base);
}

SpanTypeBase* getFloatTypeBase(u64 size) {
    SpanTypeBase base;
    base.type = t_float;
    base.namespace_ = NO_NAMESPACE;
    base.int_.size = size;
    base.ast = NULL;

    char buffer[BUFFER_SIZE];
    buffer[0] = 'f';
    uintToString(size, buffer + 1);

    SpanTypeBase* existing = findBaseType(buffer, context.activeProject->namespace_);
    if (existing != NULL) {
        return existing;
    }

    u64 nameLength = strlen(buffer);
    base.name = allocArena(context.arena, nameLength + 1);
    memcpy(base.name, buffer, nameLength + 1);

    return addBaseType(&base);
}

SpanType getPointerType(SpanType* type) {
    SpanType newType = *type;
    int backIndex = type->modsCount - 1;
    newType.mods[backIndex + 1].type = tm_ptr;
    newType.modsCount++;
    return newType;
}
SpanType getReferenceType(SpanType* type) {
    SpanType newType = *type;
    int backIndex = type->modsCount - 1;
    newType.mods[backIndex + 1].type = tm_ref;
    newType.modsCount++;
    return newType;
}

SpanType dereferenceType(SpanType* type) {
    SpanType newType = *type;
    massert(type->modsCount > 0, "should be a pointer");
    int backIndex = type->modsCount - 1;
    SpanTypeModifier* mod = &type->mods[backIndex];
    massert(mod->type == tm_ptr || mod->type == tm_ref, "should be a pointer");
    switch (mod->type) {
        case tm_ptr: {
            newType.mods[backIndex].type = tm_ref;
            break;
        }
        case tm_ref:
            newType.modsCount--;
            break;
        default:
            massert(false, "unknown type modifier");
            break;
    }
    return newType;
}

SpanTypeBase* getUintTypeBase(u64 size) {
    SpanTypeBase base;
    base.type = t_uint;
    base.namespace_ = NO_NAMESPACE;
    base.int_.size = size;
    base.ast = NULL;

    char buffer[BUFFER_SIZE];
    buffer[0] = 'u';
    uintToString(size, buffer + 1);

    SpanTypeBase* existing = findBaseType(buffer, context.activeProject->namespace_);
    if (existing != NULL) {
        return existing;
    }

    u64 nameLength = strlen(buffer);
    base.name = allocArena(context.arena, nameLength + 1);
    memcpy(base.name, buffer, nameLength + 1);

    return addBaseType(&base);
}
