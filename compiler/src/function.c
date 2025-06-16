#include "compiler/function.h"
#include "compiler.h"
#include "llvm-c/Types.h"

CompilerFunction createCompilerFunction(functionId funId, CompilerProject* compilerProject) {
    Function* function = getFunctionFromId(funId);
    CompilerFunction compilerFunction = { 0 };
    LLVMTypeRef returnType = getCompilerTypeFromId(function->returnType).llvmType;
    LLVMTypeRef paramTypes[256];
    for (u64 i = 0; i < function->parameterCount; i++) {
        paramTypes[i] = getCompilerTypeFromId(function->parameters[i]).llvmType;
    }
    LLVMTypeRef funcType = LLVMFunctionType(returnType, paramTypes, function->parameterCount, 0);
    LLVMValueRef llvmFunction = LLVMAddFunction(compilerProject->module, function->mangledName, funcType);
    compilerFunction.llvmFunction = llvmFunction;
    compilerFunction.llvmFunctionType = funcType;
    return compilerFunction;
}

void compileFunction(CompilerFunction* compilerFunction, CompilerProject* compilerProject) {
    CompilerScope compilerScope = createCompilerScope(compilerFunction, compilerProject);
    compileScope(&compilerScope, compilerFunction, compilerProject);
}
