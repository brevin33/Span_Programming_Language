#include "span_parser.h"

SpanFunction* addFunction(SpanFunction* function) {
    if (context.functionsCount >= context.functionsCapacity) {
        context.functions = reallocArena(context.arena, sizeof(SpanFunction*) * context.functionsCapacity * 2, context.functions, sizeof(SpanFunction*) * context.functionsCapacity);
        context.functionsCapacity *= 2;
    }
    SpanFunction* f = allocArena(context.arena, sizeof(SpanFunction));
    *f = *function;
    f->llvmFunc = NULL;
    context.functions[context.functionsCount++] = f;

    return f;
}

void compileFunction(SpanFunction* function) {
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function->llvmFunc, "entry");
    LLVMPositionBuilderAtEnd(context.builder, entry);
    SpanScope* scope = &function->scope;
    compileScope(scope, function);
}

SpanFunction* findFunctions(char* name, u32 namespace_, SpanFunction* buffer, u32* functionsCountOut) {
    u32 functionsCount = 0;
    for (u64 i = 0; i < context.functionsCount; i++) {
        SpanFunction* function = context.functions[i];
        bool validNamespace = function->functionType->namespace_ == namespace_;
        validNamespace = validNamespace || function->functionType->namespace_ == NO_NAMESPACE;
        bool validName = strcmp(function->name, name) == 0;
        if (validNamespace && validName) {
            buffer[functionsCount++] = *function;
        }
    }
    *functionsCountOut = functionsCount;
    if (functionsCount == 0) {
        return NULL;
    }
    return buffer;
}

char* getScrambledName(SpanAstFunctionDeclaration* decl, char* buffer) {
    SpanAst* returnType = decl->returnType;
    massert(returnType->type == ast_type, "should be a type");
    SpanAstType* type = &returnType->type_;
    char* returnTypeName = type->name;
    u32 returnTypeNameSize = strlen(returnTypeName);

    u64 bufferIndex = 0;
    memcpy(buffer, returnTypeName, returnTypeNameSize);
    bufferIndex += returnTypeNameSize;
    buffer[bufferIndex++] = '$';

    SpanAst* paramList = decl->paramList;
    SpanAstFunctionParameterDeclaration* params = &paramList->funcParam;
    massert(paramList->type == ast_func_param, "should be a scope");
    if (params != NULL) {
        for (u64 i = 0; i < paramList->funcParam.paramsCount; i++) {
            SpanAst* param = &paramList->funcParam.params[i];
            massert(param->type == ast_variable_declaration, "should be a variable declaration");
            SpanAstVariableDeclaration* variableDeclaration = &param->variableDeclaration;
            char* paramName = variableDeclaration->name;
            u32 paramNameSize = strlen(paramName);
            memcpy(buffer + bufferIndex, paramName, paramNameSize);
            bufferIndex += paramNameSize;
            if (i != paramList->funcParam.paramsCount - 1) buffer[bufferIndex++] = ',';
        }
    }
    buffer[bufferIndex++] = 0;
    return buffer;
}

void implementFunction(SpanFunction* function) {
    SpanAst* body = function->ast->functionDeclaration.body;
    massert(body->type == ast_scope, "should be a scope");
    // TODO: implement global scope
    function->scope = createSpanScope(body, NULL);
}

void compilePrototypeFunctions() {
    for (u64 i = 0; i < context.functionsCount; i++) {
        SpanFunction* function = context.functions[i];
        SpanTypeBase* functionType = function->functionType;
        LLVMTypeRef llvmFuncType = functionType->llvmType;
        massert(llvmFuncType != NULL, "llvm type should not be null");
        LLVMValueRef llvmFunc = LLVMAddFunction(context.activeProject->llvmModule, context.functions[i]->scrambledName, llvmFuncType);
        function->llvmFunc = llvmFunc;
    }
}

SpanFunction* prototypeFunction(SpanAst* ast) {
    massert(ast->type == ast_function_declaration, "should be a function");
    SpanAstFunctionDeclaration* functionDeclaration = &ast->functionDeclaration;
    SpanTypeBase* functionType = getFunctionType(ast);
    char* name = functionDeclaration->name;
    SpanFunction function;
    function.functionType = functionType;
    function.name = name;
    function.ast = ast;

    char buffer[BUFFER_SIZE];
    char* scrambledName = getScrambledName(functionDeclaration, buffer);
    u32 scrambledNameSize = strlen(scrambledName);

    SpanFunction funcBuffer[BUFFER_SIZE];
    u32 functionsCount = 0;

    SpanFunction* functions = findFunctions(name, context.activeProject->namespace_, funcBuffer, &functionsCount);
    bool found = false;
    for (u64 i = 0; i < functionsCount; i++) {
        SpanFunction* function = &functions[i];
        bool namespaceMatch = function->functionType->namespace_ == context.activeProject->namespace_;
        bool paramTypesMatch = true;
        SpanTypeBase* funcType2 = function->functionType;
        if (funcType2->function.paramTypesCount != functionType->function.paramTypesCount) {
            continue;
        }
        for (u64 j = 0; j < funcType2->function.paramTypesCount; j++) {
            SpanType paramType2 = funcType2->function.paramTypes[j];
            SpanType paramType1 = functionType->function.paramTypes[j];
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                paramTypesMatch = false;
                break;
            }
        }

        if (namespaceMatch && paramTypesMatch) {
            found = true;
            break;
        }
    }
    if (found) {
        logErrorTokens(ast->token, 1, "function already exists in same namespace with same types");
    }


    function.scrambledName = allocArena(context.arena, scrambledNameSize + 1);
    memcpy(function.scrambledName, scrambledName, scrambledNameSize + 1);

    return addFunction(&function);
}
