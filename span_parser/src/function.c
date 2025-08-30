#include "span_parser.h"
#include "span_parser/expression.h"
#include "span_parser/type.h"
#include <llvm-c/Types.h>

SpanFunction* addFunction(SpanFunction* function) {
    // cast is unique
    if (strcmp(function->name, "cast") == 0) {
        SpanFunction* f = allocArena(context.arena, sizeof(SpanFunction));
        *f = *function;

        f->functionInstancesCount = 0;
        f->functionInstancesCapacity = 1;
        f->functionInstances = allocArena(context.arena, sizeof(SpanFunctionInstance) * f->functionInstancesCapacity);

        // make sure only one param
        if (function->functionType->function.paramTypesCount != 1) {
            logErrorTokens(function->ast->token, 1, "cast function should have exactly one param");
            return f;
        }
        if (context.castFunctionsCount >= context.castFunctionsCapacity) {
            context.castFunctions = reallocArena(context.arena, sizeof(SpanFunction*) * context.castFunctionsCapacity * 2, context.castFunctions, sizeof(SpanFunction*) * context.castFunctionsCapacity);
            context.castFunctionsCapacity *= 2;
        }
        context.castFunctions[context.castFunctionsCount++] = f;
        return f;
    }

    if (context.functionsCount >= context.functionsCapacity) {
        context.functions = reallocArena(context.arena, sizeof(SpanFunction*) * context.functionsCapacity * 2, context.functions, sizeof(SpanFunction*) * context.functionsCapacity);
        context.functionsCapacity *= 2;
    }
    SpanFunction* f = allocArena(context.arena, sizeof(SpanFunction));
    *f = *function;

    f->functionInstancesCount = 0;
    f->functionInstancesCapacity = 1;
    f->functionInstances = allocArena(context.arena, sizeof(SpanFunctionInstance) * f->functionInstancesCapacity);

    context.functions[context.functionsCount++] = f;

    return f;
}

SpanFunctionInstance* findCastFunction(SpanType* valueType, SpanType* castType, u32 namespace_, bool logError, SpanAst* ast) {
    SpanFunction* functions[BUFFER_SIZE];
    u64 functionsCount = 0;
    for (u64 i = 0; i < context.castFunctionsCount; i++) {
        SpanFunction* function = context.castFunctions[i];
        SpanType* returnType = &function->functionType->function.returnType;
        SpanType* paramType = &function->functionType->function.paramTypes[0];
        bool validNamespace = function->functionType->namespace_ == namespace_;
        validNamespace = validNamespace || function->functionType->namespace_ == NO_NAMESPACE;
        if (isTypeEqual(returnType, castType) && isTypeEqual(paramType, valueType) && validNamespace) {
            functions[functionsCount++] = function;
        }
    }
    if (functionsCount == 0) {
        if (logError) {
            char buffer[BUFFER_SIZE];
            char buffer2[BUFFER_SIZE];
            char* valueTypeName = getTypeName(valueType, buffer);
            char* castTypeName = getTypeName(castType, buffer2);
            logErrorAst(ast, "can't find cast function from %s to %s", valueTypeName, castTypeName);
        }
        SpanFunctionInstance functionInstance;
        return NULL;
    }
    if (functionsCount > 1) {
        if (logError) {
            char buffer[BUFFER_SIZE];
            char buffer2[BUFFER_SIZE];
            char* valueTypeName = getTypeName(valueType, buffer);
            char* castTypeName = getTypeName(castType, buffer2);
            logErrorAst(ast, "ambiguous cast function from %s to %s", valueTypeName, castTypeName);
            for (u64 i = 0; i < functionsCount; i++) {
                char* functionName = functions[i]->name;
                logErrorAst(ast, "the cast could have meant this %s", functionName);
            }
        }
        return NULL;
    }
    //TODO: fix this
    SpanFunction* function = functions[0];
    if (function->functionInstancesCount == 0) {
        SpanFunctionInstance functionInstance;
        functionInstance.function = functions[0];
        functionInstance.llvmFunc = NULL;
        functionInstance.substitutions = NULL;
        functionInstance.substitutionsCount = 0;
        function->functionInstances[function->functionInstancesCount++] = functionInstance;
    }
    return &function->functionInstances[0];
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
    massert(mainToCall->functionInstancesCount == 1, "main should only have one instance");
    LLVMValueRef val = LLVMBuildCall2(context.builder, mainToCallLLVMType, mainToCall->functionInstances[0].llvmFunc, NULL, 0, "callMain");
    LLVMBuildRet(context.builder, val);
}

void compileFunction(SpanFunctionInstance* function) {
    if (function->function->isExtern) {
        SpanTypeBase* functionType = function->function->functionType;
        LLVMTypeRef llvmFuncType = functionType->llvmType;
        massert(llvmFuncType != NULL, "llvm type should not be null");
        LLVMValueRef llvmFunc = LLVMAddFunction(context.activeProject->llvmModule, function->function->name, llvmFuncType);
        function->llvmFunc = llvmFunc;
        return;
    }

    // do the type substitutions


    // save the current block
    LLVMBasicBlockRef lastBlock = context.currentBlock;

    SpanTypeBase* functionType = function->function->functionType;
    LLVMTypeRef llvmFuncType = functionType->llvmType;
    massert(llvmFuncType != NULL, "llvm type should not be null");
    LLVMValueRef llvmFunc = LLVMAddFunction(context.activeProject->llvmModule, function->function->scrambledName, llvmFuncType);
    function->llvmFunc = llvmFunc;


    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function->llvmFunc, "entry");
    context.currentBlock = entry;
    LLVMPositionBuilderAtEnd(context.builder, entry);

    // parameters
    for (u64 i = 0; i < function->function->functionType->function.paramTypesCount; i++) {
        char* paramName = function->function->paramNames[i];
        SpanType paramType = function->function->functionType->function.paramTypes[i];
        SpanVariable* variable = getVariableFromScope(&function->function->scope, paramName);
        LLVMTypeRef paramTypeLLVM = getLLVMType(&paramType);
        if (!variable->isReference) {
            variable->llvmValue = LLVMBuildAlloca(context.builder, paramTypeLLVM, variable->name);
            LLVMValueRef paramValue = LLVMGetParam(function->llvmFunc, i);
            LLVMBuildStore(context.builder, paramValue, variable->llvmValue);
        } else {
            variable->llvmValue = LLVMGetParam(function->llvmFunc, i);
        }
    }

    SpanStatement* statement = function->function->scope.statments;
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


bool SpanFunctionInstanceIsEqual(SpanFunctionInstance* instance1, SpanFunctionInstance* instance2) {
    bool sameFunction = instance1->function == instance2->function;
    if (!sameFunction) {
        return false;
    }
    if (instance1->substitutionsCount != instance2->substitutionsCount) {
        return false;
    }
    for (u32 i = 0; i < instance1->substitutionsCount; i++) {
        if (!isSpanTypeSubtitutionTheSame(&instance1->substitutions[i], &instance2->substitutions[i])) {
            return false;
        }
    }
    return true;
}

SpanFunctionInstance* getIfFunctionAlreadyHasInstance(SpanFunction* function, SpanFunctionInstance* instance) {
    for (u32 i = 0; i < function->functionInstancesCount; i++) {
        SpanFunctionInstance* functionInstance = &function->functionInstances[i];
        if (SpanFunctionInstanceIsEqual(instance, functionInstance)) {
            return functionInstance;
        }
    }
    return NULL;
}

SpanFunctionInstance* findFunction(char* name, u32 namespace_, SpanType* types, u32 typesCount, SpanAst* ast, bool logError) {
    // finding function
    u32 matchingFunctionsCount = 0;
    SpanFunction* buffer[BUFFER_SIZE];
    SpanFunction** matchingFunctions = findFunctions(name, namespace_, buffer, &matchingFunctionsCount);

    // perfect match
    SpanFunction* matchFunctions[BUFFER_SIZE];
    u64 matchFunctionsCount = 0;
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != typesCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = function->functionType->function.paramTypes[j];
            SpanType paramType2 = types[j];
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                paramTypesMatch = false;
                break;
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            SpanFunction* function = matchFunctions[0];
            SpanFunctionInstance functionInstance;
            functionInstance.function = function;
            functionInstance.llvmFunc = NULL;
            functionInstance.substitutions = NULL;
            functionInstance.substitutionsCount = 0;
            SpanFunctionInstance* existing = getIfFunctionAlreadyHasInstance(function, &functionInstance);
            if (existing == NULL) {
                if (function->functionInstancesCount >= function->functionInstancesCapacity) {
                    function->functionInstances
                        = reallocArena(context.arena, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity * 2, function->functionInstances, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity);
                    function->functionInstancesCapacity *= 2;
                }
                function->functionInstances[function->functionInstancesCount++] = functionInstance;
                return &function->functionInstances[function->functionInstancesCount - 1];
            }
            return existing;
        }
        if (logError) {
            logErrorAst(ast, "ambiguous function call");
            for (u64 i = 0; i < matchFunctionsCount; i++) {
                SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
                logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
            }
        }
        return NULL;
    }


    // dereference match
    matchFunctionsCount = 0;
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != typesCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = function->functionType->function.paramTypes[j];
            SpanType paramType2 = types[j];
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                if (!isTypeReference(&paramType2)) {
                    paramTypesMatch = false;
                    break;
                }
                // dereference
                SpanType derefType = dereferenceType(&paramType2);
                if (isTypeEqual(&paramType1, &derefType) == false) {
                    paramTypesMatch = false;
                    break;
                }
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            SpanFunction* function = matchFunctions[0];
            SpanFunctionInstance functionInstance;
            functionInstance.function = function;
            functionInstance.llvmFunc = NULL;
            functionInstance.substitutions = NULL;
            functionInstance.substitutionsCount = 0;
            SpanFunctionInstance* existing = getIfFunctionAlreadyHasInstance(function, &functionInstance);
            if (existing == NULL) {
                if (function->functionInstancesCount >= function->functionInstancesCapacity) {
                    function->functionInstances
                        = reallocArena(context.arena, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity * 2, function->functionInstances, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity);
                    function->functionInstancesCapacity *= 2;
                }
                function->functionInstances[function->functionInstancesCount++] = functionInstance;
                return &function->functionInstances[function->functionInstancesCount - 1];
            }
            return existing;
        }
        if (logError) {
            logErrorAst(ast, "ambiguous function call");
            for (u64 i = 0; i < matchFunctionsCount; i++) {
                SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
                logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
            }
        }
        return NULL;
    }

    // implicit cast match
    matchFunctionsCount = 0;
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != typesCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = types[j];
            SpanType paramType2 = function->functionType->function.paramTypes[j];
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                // implicit cast
                bool worked = canImplicitlyCast(&paramType1, &paramType2, false, ast) >= 0;
                if (!worked) {
                    paramTypesMatch = false;
                    break;
                }
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            SpanFunction* function = matchFunctions[0];
            SpanFunctionInstance functionInstance;
            functionInstance.function = function;
            functionInstance.llvmFunc = NULL;
            functionInstance.substitutions = NULL;
            functionInstance.substitutionsCount = 0;
            SpanFunctionInstance* existing = getIfFunctionAlreadyHasInstance(function, &functionInstance);
            if (existing == NULL) {
                if (function->functionInstancesCount >= function->functionInstancesCapacity) {
                    function->functionInstances
                        = reallocArena(context.arena, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity * 2, function->functionInstances, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity);
                    function->functionInstancesCapacity *= 2;
                }
                function->functionInstances[function->functionInstancesCount++] = functionInstance;
                return &function->functionInstances[function->functionInstancesCount - 1];
            }
            return existing;
        }
        if (logError) {
            logErrorAst(ast, "ambiguous function call");
            for (u64 i = 0; i < matchFunctionsCount; i++) {
                SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
                logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
            }
        }
        return NULL;
    }


    // interface function match
    matchFunctionsCount = 0;
    for (u64 i = 0; i < matchingFunctionsCount; i++) {
        SpanFunction* function = matchingFunctions[i];
        if (function->functionType->function.paramTypesCount != typesCount) {
            continue;
        }
        bool paramTypesMatch = true;
        for (u64 j = 0; j < function->functionType->function.paramTypesCount; j++) {
            SpanType paramType1 = types[j];
            SpanType paramType2 = function->functionType->function.paramTypes[j];
            if (isTypeEqual(&paramType1, &paramType2) == false) {
                // implicit cast
                bool worked = canImplicitlyCast(&paramType1, &paramType2, false, ast) >= 0;
                if (!worked) {
                    if (isInterfaceType(&paramType2) && typeFufillsInterface(&paramType1, &paramType2)) {
                    } else {
                        paramTypesMatch = false;
                        break;
                    }
                }
            }
        }
        if (paramTypesMatch) {
            matchFunctions[matchFunctionsCount++] = function;
        }
    }

    if (matchFunctionsCount != 0) {
        if (matchFunctionsCount == 1) {
            SpanFunction* function = matchFunctions[0];
            SpanTypeSubstitution substitutions[BUFFER_SIZE];
            u64 substitutionsCount = 0;
            // figure out substitutions needed
            for (u64 i = 0; i < function->functionType->function.paramTypesCount; i++) {
                SpanType paramType1 = types[i];
                SpanType paramType2 = function->functionType->function.paramTypes[i];
                if (isTypeEqual(&paramType1, &paramType2) == false) {
                    // implicit cast
                    bool worked = canImplicitlyCast(&paramType1, &paramType2, false, ast) >= 0;
                    if (!worked) {
                        massert(paramType2.base->type == t_interface, "should be an interface type");
                        SpanTypeSubstitution substitution;
                        substitution.type = paramType2.base;
                        substitution.replacement = paramType1;
                        substitutions[substitutionsCount++] = substitution;
                    }
                }
            }

            SpanFunctionInstance functionInstance;
            functionInstance.function = function;
            functionInstance.llvmFunc = NULL;
            functionInstance.substitutions = allocArena(context.arena, sizeof(SpanTypeSubstitution) * substitutionsCount);
            memcpy(functionInstance.substitutions, substitutions, sizeof(SpanTypeSubstitution) * substitutionsCount);
            functionInstance.substitutionsCount = substitutionsCount;
            SpanFunctionInstance* existing = getIfFunctionAlreadyHasInstance(function, &functionInstance);
            if (existing == NULL) {
                if (function->functionInstancesCount >= function->functionInstancesCapacity) {
                    function->functionInstances
                        = reallocArena(context.arena, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity * 2, function->functionInstances, sizeof(SpanFunctionInstance) * function->functionInstancesCapacity);
                    function->functionInstancesCapacity *= 2;
                }
                function->functionInstances[function->functionInstancesCount++] = functionInstance;
                return &function->functionInstances[function->functionInstancesCount - 1];
            }
            return existing;
        }
        if (logError) {
            logErrorAst(ast, "ambiguous function call");
            for (u64 i = 0; i < matchFunctionsCount; i++) {
                SpanAst* functionItCouldHaveBeenAst = matchFunctions[i]->ast;
                logErrorAst(functionItCouldHaveBeenAst, "the function could have meant this");
            }
        }
        return NULL;
    }

    char errorMessage[BUFFER_SIZE];
    sprintf(errorMessage, "could not find function called %s that works with arguments of:", name);
    for (u64 i = 0; i < typesCount; i++) {
        char typeNameBuffer[BUFFER_SIZE];
        SpanType argType = types[i];
        char* argTypeName = getTypeName(&argType, typeNameBuffer);
        if (i == 0) {
            sprintf(errorMessage, "%s %s", errorMessage, argTypeName);
        } else {
            sprintf(errorMessage, "%s, %s", errorMessage, argTypeName);
        }
    }
    if (logError) {
        logErrorAst(ast, errorMessage);
    }
    return NULL;
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

    char* functionName = decl->name;
    u32 functionNameSize = strlen(functionName);
    memcpy(buffer + bufferIndex, functionName, functionNameSize);
    bufferIndex += functionNameSize;
    buffer[bufferIndex++] = '$';



    SpanAst* paramList = decl->paramList;
    SpanAstFunctionParameterDeclaration* params = &paramList->funcParam;
    massert(paramList->type == ast_func_param, "should be a scope");
    if (params != NULL) {
        for (u64 i = 0; i < paramList->funcParam.paramsCount; i++) {
            SpanAst* param = &paramList->funcParam.params[i];
            massert(param->type == ast_variable_declaration, "should be a variable declaration");
            SpanAstVariableDeclaration* variableDeclaration = &param->variableDeclaration;
            SpanAst* type = variableDeclaration->type;
            massert(type->type == ast_type, "should be a type");
            SpanAstType* typeType = &type->type_;
            char* paramTypeName = typeType->name;
            u32 paramTypeNameSize = strlen(paramTypeName);
            memcpy(buffer + bufferIndex, paramTypeName, paramTypeNameSize);
            bufferIndex += paramTypeNameSize;
            if (i != paramList->funcParam.paramsCount - 1) buffer[bufferIndex++] = ',';
        }
    }
    buffer[bufferIndex++] = 0;
    return buffer;
}

void implementFunction(SpanFunction* function) {
    SpanAst* body = function->ast->functionDeclaration.body;
    if (body == NULL) {
        function->isExtern = true;
        return;
    }
    massert(body->type == ast_scope, "should be a scope");
    // TODO: implement global scope
    function->scope = createSpanScope(body, NULL, function);
    function->isExtern = false;
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
