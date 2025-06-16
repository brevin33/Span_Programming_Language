#include "compiler.h"


void compileProject(Project* project) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    createCompilerTypes();

    CompilerProject compilerProject = createCompilerProject(project);
}
