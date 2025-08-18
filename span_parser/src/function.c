#include "span_parser.h"
#include "span_parser/type.h"
#include <llvm-c/Types.h>

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

void compileRealMainFunction(SpanFunction* mainToCall) {
    // make the real main function
    LLVMTypeRef mainType = LLVMFunctionType(LLVMIntType(32), NULL, 0, 0);
    LLVMValueRef mainFunc = LLVMAddFunction(context.activeProject->llvmModule, "main", mainType);
    LLVMSetLinkage(mainFunc, LLVMExternalLinkage);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainFunc, "entry");
    LLVMPositionBuilderAtEnd(context.builder, entry);

    //TODO: global initializers

    // call the real main function
    LLVMTypeRef mainToCallLLVMType = mainToCall->functionType->llvmType;
    LLVMValueRef val = LLVMBuildCall2(context.builder, mainToCallLLVMType, mainToCall->llvmFunc, NULL, 0, "callMain");
    LLVMBuildRet(context.builder, val);
}

void compileFunction(SpanFunction* function) {
    // save the current block
    LLVMBasicBlockRef lastBlock = context.currentBlock;

    SpanTypeBase* functionType = function->functionType;
    LLVMTypeRef llvmFuncType = functionType->llvmType;
    massert(llvmFuncType != NULL, "llvm type should not be null");
    LLVMValueRef llvmFunc = LLVMAddFunction(context.activeProject->llvmModule, function->scrambledName, llvmFuncType);
    function->llvmFunc = llvmFunc;

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function->llvmFunc, "entry");
    context.currentBlock = entry;
    LLVMPositionBuilderAtEnd(context.builder, entry);

    // parameters
    for (u64 i = 0; i < function->functionType->function.paramTypesCount; i++) {
        char* paramName = function->paramNames[i];
        SpanType paramType = function->functionType->function.paramTypes[i];
        SpanVariable* variable = getVariableFromScope(&function->scope, paramName);
        LLVMTypeRef paramTypeLLVM = getLLVMType(&paramType);
        if (!variable->isReference) {
            variable->llvmValue = LLVMBuildAlloca(context.builder, paramTypeLLVM, variable->name);
            LLVMValueRef paramValue = LLVMGetParam(function->llvmFunc, i);
            LLVMBuildStore(context.builder, paramValue, variable->llvmValue);
        } else {
            variable->llvmValue = LLVMGetParam(function->llvmFunc, i);
        }
    }

    SpanStatement* statement = function->scope.statments;
    massert(statement->type == st_scope, "should be a scope");
    SpanScope* scope = statement->scope.scope;

    compileScope(scope, function);

    // go back to the current block end poistion
    context.currentBlock = lastBlock;
    if (context.currentBlock != NULL) {
        LLVMPositionBuilderAtEnd(context.builder, context.currentBlock);
    }
}

SpanFunction** findFunctions(char* name, u32 namespace_, SpanFunction** buffer, u32* functionsCountOut) {
    u32 functionsCount = 0;
    for (u64 i = 0; i < context.functionsCount; i++) {
        SpanFunction* function = context.functions[i];
        bool validNamespace = function->functionType->namespace_ == namespace_;
        validNamespace = validNamespace || function->functionType->namespace_ == NO_NAMESPACE;
        bool validName = strcmp(function->name, name) == 0;
        if (validNamespace && validName) {
            buffer[functionsCount++] = function;
        }
    }
    *functionsCountOut = functionsCount;
    if (functionsCount == 0) {
        return NULL;
    }
    return buffer;
}

char** getParamNames(SpanAstFunctionDeclaration* decl, u64* paramNamesCountOut) {
    SpanAst* paramList = decl->paramList;
    massert(paramList->type == ast_func_param, "should be a scope");
    SpanAstFunctionParameterDeclaration* params = &paramList->funcParam;
    if (params->paramsCount > 0) {
        *paramNamesCountOut = params->paramsCount;
        char** paramNames = allocArena(context.arena, sizeof(char*) * params->paramsCount);
        for (u64 i = 0; i < params->paramsCount; i++) {
            SpanAst* param = &params->params[i];
            massert(param->type == ast_variable_declaration, "should be a variable declaration");
            SpanAstVariableDeclaration* variableDeclaration = &param->variableDeclaration;
            char* name = variableDeclaration->name;
            paramNames[i] = name;
        }
        return paramNames;
    }
    *paramNamesCountOut = 0;
    return NULL;
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
    function->scope = createSpanScope(body, NULL, function);
    for (u64 i = 0; i < function->functionType->function.paramTypesCount; i++) {
        char* paramName = function->paramNames[i];
        SpanType paramType = function->functionType->function.paramTypes[i];
        SpanVariable* variable = addVariableToScope(&function->scope, paramName, paramType, function->ast);
    }
    function->scope.statmentsCount = 1;
    function->scope.statments = allocArena(context.arena, sizeof(SpanStatement) * function->scope.statmentsCount);
    function->scope.statments[0] = createSpanScopeStatement(body, &function->scope, function);
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

    SpanAstFunctionDeclaration* decl = &ast->functionDeclaration;
    u64 paramNamesCount;
    char** paramNames = getParamNames(decl, &paramNamesCount);
    function.paramNames = paramNames;

    char buffer[BUFFER_SIZE];
    char* scrambledName = getScrambledName(functionDeclaration, buffer);
    u32 scrambledNameSize = strlen(scrambledName);

    SpanFunction* funcBuffer[BUFFER_SIZE];
    u32 functionsCount = 0;
    SpanFunction** functions = findFunctions(name, context.activeProject->namespace_, funcBuffer, &functionsCount);
    bool found = false;
    for (u64 i = 0; i < functionsCount; i++) {
        SpanFunction* function = functions[i];
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
