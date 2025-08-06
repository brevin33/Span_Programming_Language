#include "span_parser.h"


SpanFunction* addFunction(SpanFunction* function) {
    if (context.functionsCount >= context.functionsCapacity) {
        context.functions = reallocArena(context.arena, sizeof(SpanFunction) * context.functionsCapacity * 2, context.functions, sizeof(SpanFunction) * context.functionsCapacity);
        context.functionsCapacity *= 2;
    }
    context.functions[context.functionsCount++] = *function;
    return &context.functions[context.functionsCount - 1];
}

SpanFunction* findFunction(char* name, u32 namespace) {
    for (u64 i = 0; i < context.functionsCount; i++) {
        SpanFunction* function = &context.functions[i];
        bool validNamespace = function->functionType->namespace == namespace;
        validNamespace = validNamespace || function->functionType->namespace == NO_NAMESPACE;
        bool validName = strcmp(function->scrambledName, name) == 0;
        if (validNamespace && validName) {
            return function;
        }
    }
    return NULL;
}

char* getScrambledName(SpanAstFunctionDeclaration* decl, char* buffer) {
    SpanAst* returnType = decl->returnType;
    massert(returnType->type == ast_type, "should be a type");
    SpanAstType* type = returnType->type_;
    char* returnTypeName = type->name;
    u32 returnTypeNameSize = strlen(returnTypeName);

    u64 bufferIndex = 0;
    memcpy(buffer, returnTypeName, returnTypeNameSize);
    bufferIndex += returnTypeNameSize;
    buffer[bufferIndex++] = '$';

    SpanAst* paramList = decl->paramList;
    massert(paramList->type == ast_func_param, "should be a scope");

    for (u64 i = 0; i < paramList->funcParam->paramsCount; i++) {
        SpanAst* param = &paramList->funcParam->params[i];
        massert(param->type == ast_variable_declaration, "should be a variable declaration");
        SpanAstVariableDeclaration* variableDeclaration = param->variableDeclaration;
        char* paramName = variableDeclaration->name;
        u32 paramNameSize = strlen(paramName);
        memcpy(buffer + bufferIndex, paramName, paramNameSize);
        bufferIndex += paramNameSize;
        if (i != paramList->funcParam->paramsCount - 1) buffer[bufferIndex++] = ',';
    }
    buffer[bufferIndex++] = 0;
    return buffer;
}

SpanFunction* implementFunction(SpanAst* ast) {
    massert(ast->type == ast_function_declaration, "should be a function");
    SpanAstFunctionDeclaration* functionDeclaration = ast->functionDeclaration;
    SpanTypeBase* functionType = getFunctionType(ast);
    char* name = functionDeclaration->name;
    SpanFunction function;
    function.functionType = functionType;
    function.name = name;

    char buffer[BUFFER_SIZE];
    char* scrambledName = getScrambledName(functionDeclaration, buffer);
    u32 scrambledNameSize = strlen(scrambledName);


    SpanFunction* existing = findFunction(scrambledName, functionType->namespace);
    if (existing != NULL) {
        return existing;
    }

    function.scrambledName = allocArena(context.arena, scrambledNameSize + 1);
    memcpy(function.scrambledName, scrambledName, scrambledNameSize + 1);

    return addFunction(&function);
}
