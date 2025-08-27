#include "span_parser.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/type.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"


void initializeContext(Arena arena) {
    context.arena = arena;
    context.activeProject = NULL;
    context.namespaceCounter = 1;
    context.initialized = true;
    context.baseTypesCapacity = 8;
    context.baseTypesCount = 0;
    context.baseTypes = allocArena(arena, sizeof(SpanTypeBase) * context.baseTypesCapacity);
    context.functionsCapacity = 8;
    context.functionsCount = 0;
    context.functions = allocArena(arena, sizeof(SpanFunction) * context.functionsCapacity);
    context.numberOfErrors = 0;
}

void cleanupSpanContext() {
    context.initialized = false;
}

char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount) {
    u64 lineStartsCount = 0;
    u64 lineStartsCapacity = 8;
    char** lineStarts = allocArena(arena, sizeof(char*) * lineStartsCapacity);
    lineStarts[lineStartsCount++] = fileContents;
    while (*fileContents != '\0') {
        if (*fileContents == '\n') {
            if (lineStartsCount >= lineStartsCapacity) {
                lineStarts = reallocArena(arena, sizeof(char*) * lineStartsCapacity * 2, lineStarts, sizeof(char*) * lineStartsCapacity);
                lineStartsCapacity *= 2;
            }
            lineStarts[lineStartsCount++] = fileContents + 1;
        }
        fileContents++;
    }
    *outLineStartsCount = lineStartsCount;
    return lineStarts;
}

void SpanFilePrototypeFunctions(SpanFile* file) {
    u32 fileDefinitionStartsCapacity = 4;
    file->fileDefinedFunctions = allocArena(context.arena, sizeof(SpanFunction*) * fileDefinitionStartsCapacity);
    file->fileDefinedFunctionsCount = 0;
    for (u64 i = 0; i < file->ast.file.globalStatementsCount; i++) {
        SpanAst* statement = &file->ast.file.globalStatements[i];
        if (statement->type == ast_function_declaration) {
            SpanFunction* function = prototypeFunction(statement);
            if (function == NULL) {
                continue;
            }
            if (file->fileDefinedFunctionsCount >= fileDefinitionStartsCapacity) {
                file->fileDefinedFunctions = reallocArena(context.arena, sizeof(SpanFunction*) * fileDefinitionStartsCapacity * 2, file->fileDefinedFunctions, sizeof(SpanFunction*) * fileDefinitionStartsCapacity);
                fileDefinitionStartsCapacity *= 2;
            }
            file->fileDefinedFunctions[file->fileDefinedFunctionsCount++] = function;
        }
    }
}

void SpanFileImplementFunctions(SpanFile* file) {
    for (u64 i = 0; i < file->fileDefinedFunctionsCount; i++) {
        SpanFunction* function = file->fileDefinedFunctions[i];
        implementFunction(function);
    }
}

u64 SpanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer) {
    u64 line = 0;
    for (u64 i = 0; i < file->fileLineStartsCount; i++) {
        if (file->fileLineStarts[i] > internalPointer) {
            return line;
        }
        line++;
    }
    massert(false, "didn't get passed an internal pointer");
    return line;
}

void SpanFilePrototypeTypes(SpanFile* file) {
    u64 fileDefinitionStartsCapacity = 4;
    file->fileDefinedTypes = allocArena(context.arena, sizeof(SpanTypeBase*) * fileDefinitionStartsCapacity);
    file->fileDefinedTypesCount = 0;
    for (u64 i = 0; i < file->ast.file.globalStatementsCount; i++) {
        SpanAst* statement = &file->ast.file.globalStatements[i];
        if (AstIsTypeDefinition(statement)) {
            SpanTypeBase* type = prototypeType(statement);
            if (type == NULL) {
                continue;
            }
            if (file->fileDefinedTypesCount >= fileDefinitionStartsCapacity) {
                file->fileDefinedTypes = reallocArena(context.arena, sizeof(SpanTypeBase*) * fileDefinitionStartsCapacity * 2, file->fileDefinedTypes, sizeof(SpanTypeBase*) * fileDefinitionStartsCapacity);
                fileDefinitionStartsCapacity *= 2;
            }
            file->fileDefinedTypes[file->fileDefinedTypesCount++] = type;
        }
    }
}

void SpanFileImplementTypes(SpanFile* file) {
    for (u64 i = 0; i < file->fileDefinedTypesCount; i++) {
        implementType(file->fileDefinedTypes[i]);
    }
}

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex) {
    SpanFile file;
    file.fileName = fileName;
    char filePath[BUFFER_SIZE];
    sprintf(filePath, "%s/%s", directory, fileName);
    file.fileContents = readFile(context.arena, filePath);
    file.fileLineStarts = getLineStarts(context.arena, file.fileContents, &file.fileLineStartsCount);
    return file;
}

char* getNameSpaceFromTokens(Token** tokens, char* buffer) {
    Token* token = *tokens;
    if (token->type != tt_id && token[1].type != tt_colon_colon) {
        return NULL;
    }
    char* namespace_ = tokenGetString(*token, buffer);
    token += 2;
    *tokens = token;
    return namespace_;
}

void SpanFileGetTokensForFile(SpanFile* file, u64 fileIndex) {
    file->tokens = createTokens(context.arena, file->fileContents, fileIndex);

    u64 fileDefinitionStartsCapacity = 4;
    u64 fileDefinitionStartsCount = 0;
    Token** fileDefinitionStarts = allocArena(context.arena, sizeof(Token*) * fileDefinitionStartsCapacity);
}

void SpanFileGetAstForFile(SpanFile* file) {
    file->ast = createAst(context.arena, &file->tokens);
}

SpanProject createSpanProjectHelper(Arena arena, SpanProject* parent, char* path) {
    if (!context.initialized) {
        initializeContext(arena);
    }
    massert(path != NULL, "path should not be null");
    SpanProject project;
    context.activeProject = &project;
    project.arena = arena;
    project.llvmModule = NULL;

    char buffer[BUFFER_SIZE];
    char* dirName = getDirectoryNameFromPath(path, buffer);
    u32 dirNameLength = strlen(dirName);

    project.name = allocArena(project.arena, dirNameLength + 1);
    memcpy(project.name, dirName, dirNameLength + 1);

    project.parent = parent;
    project.namespace_ = context.namespaceCounter++;

    u64 directoryCount;
    char** directories = getDirectoryNamesInDirectory(project.arena, path, &directoryCount);
    project.childCount = 0;
    project.childCapacity = directoryCount;
    if (directoryCount > 0) project.children = allocArena(project.arena, sizeof(SpanProject) * directoryCount);
    else
        project.children = NULL;
    for (u64 i = 0; i < directoryCount; i++) {
        char* directory = directories[i];
        char directoryPath[BUFFER_SIZE];
        sprintf(directoryPath, "%s/%s", path, directory);
        SpanProject child = createSpanProjectHelper(project.arena, &project, directoryPath);
        project.children[project.childCount++] = child;
    }

    char** fileNames = getFileNamesInDirectory(project.arena, path, &project.fileCount);
    u64 fileCount = project.fileCount;
    project.files = allocArena(project.arena, sizeof(SpanFile) * fileCount);
    project.fileCount = 0;
    for (u64 i = 0; i < fileCount; i++) {
        char fileExtBuffer[BUFFER_SIZE];
        char* fileType = getFileType(fileNames[i], fileExtBuffer);
        if (strcmp(fileType, ".span") != 0) {
            continue;
        }
        project.files[i] = createSpanFile(project.arena, fileNames[i], path, i);
        SpanFileGetTokensForFile(&project.files[i], i);
        SpanFileGetAstForFile(&project.files[i]);
        project.fileCount++;
    }

    for (u64 i = 0; i < project.fileCount; i++) {
        SpanFilePrototypeTypes(&project.files[i]);
    }

    for (u64 i = 0; i < project.fileCount; i++) {
        SpanFileImplementTypes(&project.files[i]);
    }

    for (u64 i = 0; i < project.fileCount; i++) {
        SpanFilePrototypeFunctions(&project.files[i]);
    }

    for (u64 i = 0; i < project.fileCount; i++) {
        SpanFileImplementFunctions(&project.files[i]);
    }

    if (context.numberOfErrors > 0) {
        redprintf("\nfailed to create span project!\n");
        redprintf("number of errors: %u\n", context.numberOfErrors);
    } else {
        greenprintf("successfully created span project\n");
    }
    return project;
}

SpanProject createSpanProject(Arena arena, char* path) {
    return createSpanProjectHelper(arena, NULL, path);
}

u32 _getNamespace(char* name, SpanProject* project) {
    if (strcmp(name, project->name) == 0) {
        return project->namespace_;
    }
    for (u64 i = 0; i < project->childCount; i++) {
        u32 namespace_ = _getNamespace(name, &project->children[i]);
        if (namespace_ != NO_NAMESPACE) {
            return namespace_;
        }
    }
    return NO_NAMESPACE;
}

u32 getNamespace(char* name) {
    SpanProject* project = context.activeProject;
    return _getNamespace(name, project);
}

SpanProject* _SpanProjectFromNamespace(u32 namespace_, SpanProject* project) {
    if (project->namespace_ == namespace_) {
        return project;
    }
    for (u64 i = 0; i < project->childCount; i++) {
        SpanProject* child = &project->children[i];
        SpanProject* found = _SpanProjectFromNamespace(namespace_, child);
        if (found != NULL) {
            return found;
        }
    }
    massert(false, "namespace not found");
    return NULL;
}

SpanProject* SpanProjectFromNamespace(u32 namespace_) {
    SpanProject* project = context.activeProject;
    return _SpanProjectFromNamespace(namespace_, project);
}

SpanFile* SpanFileFromTokenAndNamespace(Token token, u32 namespace_) {
    massert(namespace_ != NO_NAMESPACE, "namespace must be valid");
    SpanProject* project = SpanProjectFromNamespace(namespace_);
    u16 fileIndex = token.file;
    SpanFile* file = &project->files[fileIndex];
    return file;
}


void compileSpanProject(SpanProject* project) {
    if (context.numberOfErrors > 0) {
        redprintf("can't compile project with errors\n");
        redprintf("number of errors: %u\n", context.numberOfErrors);
        return;
    }
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllDisassemblers();
    char* triple = LLVMGetDefaultTargetTriple();

    context.activeProject = project;
    context.llvmContext = LLVMContextCreate();
    context.currentBlock = NULL;

    project->llvmModule = LLVMModuleCreateWithName(project->name);
    context.builder = LLVMCreateBuilder();
    createLLVMTypeBaseTypes();

    for (u64 i = 0; i < context.functionsCount; i++) {
        SpanFunction* function = context.functions[i];
        char* name = context.functions[i]->name;
        if (strcmp(name, "main") == 0) {
            compileFunction(function);
            compileRealMainFunction(function);
        }
    }

    char* error;
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMTargetMachineRef targetMachine;
    targetMachine = LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

    char* ir = LLVMPrintModuleToString(project->llvmModule);
    printf("\n%s\n\n", ir);
    LLVMDisposeMessage(ir);

    if (LLVMVerifyModule(project->llvmModule, LLVMAbortProcessAction, &error) != 0) {
        redprintf("LLVMVerifyModule failed: %s\n", error);
        LLVMDisposeMessage(error);
        massert(false, "LLVMVerifyModule failed");
    } else {
        greenprintf("successfully verified module\n");
    }

    bool removed = rmdir("build");
    bool success = mkdir("build");
    if (!success) {
        redprintf("failed to create build directory\n");
        return;
    } else {
        greenprintf("successfully created build directory\n");
    }

    char* outputFileName = "build/output.obj";
    if (LLVMTargetMachineEmitToFile(targetMachine, project->llvmModule, outputFileName, LLVMObjectFile, &error) != 0) {
        redprintf("Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        assert(false);
    } else {
        greenprintf("successfully emitted object file: %s\n", outputFileName);
    }
    char* objectFiles[BUFFER_SIZE];
    u64 objectFilesCount = 1;
    objectFiles[0] = outputFileName;

    char* exeName = "build/output.exe";
    success = linkExe(objectFiles, objectFilesCount, exeName);
    if (!success) {
        redprintf("failed to link exe: %s\n", exeName);
        return;
    } else {
        greenprintf("successfully linked exe: %s\n", exeName);
    }

    LLVMDisposeBuilder(context.builder);
    LLVMDisposeModule(project->llvmModule);
    LLVMContextDispose(context.llvmContext);
}
