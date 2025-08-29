#pragma once

#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"
#include "span_parser/type.h"
#include "span_parser/function.h"

#define NO_NAMESPACE 0

typedef struct _SpanFile {
    char* fileName;
    char* fileContents;
    char** fileLineStarts;
    u64 fileLineStartsCount;

    Token* tokens;

    SpanTypeBase** fileDefinedTypes;
    u64 fileDefinedTypesCount;

    SpanFunction** fileDefinedFunctions;
    u64 fileDefinedFunctionsCount;

    SpanAst ast;

} SpanFile;

void initializeContext(Arena arena);

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex);

void SpanFileGetTokensForFile(SpanFile* file, u64 fileIndex);

void SpanFileGetAstForFile(SpanFile* file);

void SpanFilePrototypeTypes(SpanFile* file);
void compilePrototypeSpanFile(SpanFile* file);

void SpanFileImplementTypes(SpanFile* file);

void SpanFilePrototypeFunctions(SpanFile* file);

void SpanFileImplementFunctions(SpanFile* file);

u64 SpanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer);

char* SpanFileGetLine(SpanFile* file, u64 line, char* buffer);

char* getNameSpaceFromTokens(Token** tokens, char* buffer);

typedef struct _SpanProject SpanProject;
typedef struct _SpanProject {
    Arena arena;  // arenas for all substantial data
    char* name;
    SpanFile* files;
    u64 fileCount;
    SpanProject* parent;
    SpanProject* children;
    u64 childCount;
    u64 childCapacity;
    u32 namespace_;
    LLVMModuleRef llvmModule;
    LLVMTargetDataRef dataLayout;
} SpanProject;

char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount);

SpanProject createSpanProjectHelper(Arena arena, SpanProject* parent, char* path);

u32 getNamespace(char* name);

SpanFile* SpanFileFromTokenAndNamespace(Token token, u32 namespace_);

SpanProject* SpanProjectFromNamespace(u32 namespace_);
