#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"

#define NO_NAMESPACE 0

typedef u32 Namespace;

typedef struct _SpanFile {
    Arena arena;

    char* fileName;
    char* fileContents;
    char** fileLineStarts;
    u64 fileLineStartsCount;

    Token* tokens;

    SpanAst ast;
} SpanFile;

void initializeContext(Arena arena);

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex);

void SpanFileGetTokensForFile(SpanFile* file, u64 fileIndex);

void SpanFileGetAstForFile(SpanFile* file);

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
    u32 namespace;
} SpanProject;

char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount);

SpanProject createSpanProjectHelper(Arena arena, SpanProject* parent, char* path);

Namespace getNamespace(char* name);

SpanFile* SpanFileFromTokenAndNamespace(Token token, Namespace namespace);

SpanProject* SpanProjectFromNamespace(Namespace namespace);
