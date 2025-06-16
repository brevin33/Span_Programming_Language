#pragma once
#include "parser.h"
#include "llvm.h"

typedef struct _CompilerScope CompilerScope;

typedef struct _CompilerFunction CompilerFunction;

typedef struct _CompilerProject CompilerProject;

typedef struct _CompilerScope {
    Scope* scope;
    CompilerScope* parentScope;
    map nameToLLVMValue;
} CompilerScope;

LLVMValueRef getLLVMValueFromName(CompilerScope* scope, char* name);

void compileScope(CompilerScope* scope, CompilerFunction* compilerFunction, CompilerProject* compilerProject);


CompilerScope createCompilerScope(CompilerFunction* function, CompilerProject* compilerProject);
