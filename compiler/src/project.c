#include "compiler.h"
#include <assert.h>

CompilerProject createCompilerProject(Project* project) {
    CompilerProject compilerProject = { 0 };
    compilerProject.project = project;
    char* moduleName = project->directory;
    compilerProject.module = LLVMModuleCreateWithName(moduleName);

    CompilerFunction* functions = arenaAlloc(project->arena, sizeof(CompilerFunction) * project->functionCount);

    for (u64 i = 0; i < project->functionCount; i++) {
        functionId funcId = project->functions[i];
        Function* function = getFunctionFromId(funcId);
        if (function->type == ft_extern_c) {
            assert(false && "Extern C functions not implemented yet");
        }
        functions[i] = createCompilerFunction(funcId, &compilerProject);
    }

    for (u64 i = 0; i < project->functionCount; i++) {
        functionId funcId = project->functions[i];
        Function* function = getFunctionFromId(funcId);
        if (function->type == ft_extern || function->type == ft_extern_c) {
            continue;
        }
        compileFunction(&functions[i], &compilerProject);
    }

    return compilerProject;
}
