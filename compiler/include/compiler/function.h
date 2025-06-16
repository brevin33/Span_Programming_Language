#pragma once
#include "llvm.h"
#include "parser.h"

typedef struct _CompilerFunction {
    Function* function;
    LLVMValueRef llvmFunction;
    LLVMTypeRef llvmFunctionType;
} CompilerFunction;

typedef struct _CompilerProject CompilerProject;

CompilerFunction createCompilerFunction(functionId function, CompilerProject* compilerProject);
void compileFunction(CompilerFunction* compilerFunction, CompilerProject* compilerProject);
