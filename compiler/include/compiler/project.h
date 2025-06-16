#pragma once
#include "parser.h"
#include "llvm.h"
#include "llvm-c/Types.h"

typedef struct _CompilerProject {
    Project* project;
    LLVMModuleRef module;
} CompilerProject;

CompilerProject createCompilerProject(Project* project);
