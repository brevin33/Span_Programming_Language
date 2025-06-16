#pragma once
#include "llvm.h"
#include "parser.h"

typedef struct _CompilerType {
    typeId tid;
    LLVMTypeRef llvmType;
} CompilerType;

extern CompilerType* gCompilerTypes;

void createCompilerTypes();
CompilerType createCompilerType(typeId tid);
CompilerType getCompilerTypeFromId(typeId typeId);
