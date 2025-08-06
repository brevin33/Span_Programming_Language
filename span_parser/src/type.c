#include "span_parser.h"
#include "span_parser/ast.h"


SpanTypeBase* addBaseType(SpanTypeBase* base) {
    if (context.baseTypesCount >= context.baseTypesCapacity) {
        context.baseTypes = reallocArena(context.arena, sizeof(SpanTypeBase) * context.baseTypesCapacity * 2, context.baseTypes, sizeof(SpanTypeBase) * context.baseTypesCapacity);
        context.baseTypesCapacity *= 2;
    }
    u64 index = context.baseTypesCount++;
    context.baseTypes[index] = *base;

    SpanTypeBase* existing = findBaseType(base->name, base->namespace);

    return &context.baseTypes[index];
}

SpanTypeBase* findBaseType(char* name, u32 namespace) {
    for (u64 i = 0; i < context.baseTypesCount; i++) {
        SpanTypeBase* base = &context.baseTypes[i];
        bool validNamespace = base->namespace == namespace;
        validNamespace = validNamespace || base->namespace == NO_NAMESPACE;
        bool validName = strcmp(base->name, name) == 0;
        if (validNamespace && validName) {
            return base;
        }
    }
    return NULL;
}

SpanTypeBase* prototypeStuctType(SpanAst* structAst) {
    massert(structAst->type == ast_struct, "should be a struct");
    SpanTypeBase base;
    base.type = st_struct;
    base.namespace = context.activeProject->namespace;
    base.ast = structAst;
    base.name = structAst->struct_->name;

    return addBaseType(&base);
}

SpanTypeBase* typeFromTypeAst(SpanAst* typeAst) {
    massert(typeAst->type == ast_type, "should be a type");
    char* typeName = typeAst->type_->name;

    char firstChar = typeName[0];
    bool isNumber = firstChar == 'i' || firstChar == 'u' || firstChar == 'f';
    isNumber = isNumber && stringIsUint(typeName + 1);
    if (isNumber) {
        u64 size = stringToUint(typeName + 1);
        if (firstChar == 'i') return getIntType(size);
        if (firstChar == 'u') return getUintType(size);
        if (firstChar == 'f') return getFloatType(size);
    }

    SpanTypeBase* existing = findBaseType(typeName, NO_NAMESPACE);
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
    if (type->type == st_struct) {
        SpanTypeBase* base = &context.baseTypes[0];
        int i = 0;
        implementStuctType(type);
    } else {
        massert(false, "should be a type of struct, enum, or interface");
    }
}

void implementStuctType(SpanTypeBase* structType) {
    massert(structType->type == st_struct, "should be a struct");
    SpanAst* ast = structType->ast;
    u64 fieldsCapacity = 2;

    structType->struct_.fields = allocArena(context.arena, sizeof(SpanTypeBase*) * fieldsCapacity);
    structType->struct_.fieldsNames = allocArena(context.arena, sizeof(char*) * fieldsCapacity);
    structType->struct_.fieldsCount = 0;
    for (u64 i = 0; i < ast->struct_->fieldsCount; i++) {
        SpanAst* field = &ast->struct_->fields[i];
        massert(field->type == ast_variable_declaration, "should be a variable declaration");
        SpanAst* type = field->variableDeclaration->type;
        massert(type->type == ast_type, "should be a type");
        char* name = field->variableDeclaration->name;
        SpanTypeBase* base = typeFromTypeAst(type);
        if (base == NULL) {
            logErrorTokens(type->token, 1, "type not found");
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

SpanTypeBase* getIntType(u64 size) {
    SpanTypeBase base;
    base.type = st_int;
    base.namespace = NO_NAMESPACE;
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
    SpanAstFunctionDeclaration* functionDeclaration = ast->functionDeclaration;

    char* returnTypeName = functionDeclaration->returnType->type_->name;
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
    SpanAstFunctionParameterDeclaration* params = paramList->funcParam;
    for (u64 i = 0; i < params->paramsCount; i++) {
        SpanAst* param = &params->params[i];
        char* paramName = param->variableDeclaration->name;
        u32 paramNameSize = strlen(paramName);
        memcpy(buffer + bufferIndex, paramName, paramNameSize);
        bufferIndex += paramNameSize;
        if (i != params->paramsCount - 1) buffer[bufferIndex++] = ',';
    }
    buffer[bufferIndex++] = ')';
    buffer[bufferIndex++] = 0;

    SpanTypeBase* existing = findBaseType(buffer, NO_NAMESPACE);
    if (existing != NULL) {
        return existing;
    }
    char* name = allocArena(context.arena, bufferIndex);
    memcpy(name, buffer, bufferIndex);

    SpanTypeBase base;
    base.type = st_function;
    base.namespace = context.activeProject->namespace;
    base.ast = ast;
    base.name = name;
    base.function.returnType = typeFromTypeAst(functionDeclaration->returnType);
    base.function.paramTypes = allocArena(context.arena, sizeof(SpanTypeBase*) * params->paramsCount);
    base.function.paramTypesCount = params->paramsCount;
    for (u64 i = 0; i < params->paramsCount; i++) {
        SpanAst* param = &params->params[i];
        massert(param->type == ast_variable_declaration, "should be a variable declaration");
        base.function.paramTypes[i] = typeFromTypeAst(param->variableDeclaration->type);
    }
    return addBaseType(&base);
}

SpanTypeBase* getFloatType(u64 size) {
    SpanTypeBase base;
    base.type = st_float;
    base.namespace = NO_NAMESPACE;
    base.int_.size = size;
    base.ast = NULL;

    char buffer[BUFFER_SIZE];
    buffer[0] = 'f';
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

SpanTypeBase* getUintType(u64 size) {
    SpanTypeBase base;
    base.type = st_uint;
    base.namespace = NO_NAMESPACE;
    base.int_.size = size;
    base.ast = NULL;

    char buffer[BUFFER_SIZE];
    buffer[0] = 'u';
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
