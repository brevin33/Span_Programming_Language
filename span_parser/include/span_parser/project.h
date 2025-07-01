#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"
#include "span_parser/type.h"

typedef struct _SpanFile {
    Arena arena;

    char* fileName;
    char* fileContents;
    char** fileLineStarts;
    u64 fileLineStartsCount;

    Token* tokens;

    Type* types;
    u64 typesCount;
    u64 typesCapacity;
} SpanFile;


void initializeContext(Arena arena);

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex);

void getTokensForFile(SpanFile* file, u64 fileIndex);

u64 SpanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer);

char* SpanFileGetLine(SpanFile* file, u64 line, char* buffer);

char* getNameSpaceFromTokens(Token** tokens, char* buffer);

typedef struct _TemplateDefinition {
    char** names;
    Type** interfaces;
    u64 templateCount;
} TemplateDefinition;

typedef struct _TemplateInstance {
    Type** types;
    u64 templateCount;
} TemplateInstance;

TemplateDefinition* createTemplateDefinitionFromTokens(Arena arena, Token** tokens);

TemplateInstance* createTemplateInstanceCreateFromTokens(Arena arena, Token** tokens);

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

} SpanProject;

char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount);

SpanProject createSpanProjectHelper(Arena arena, SpanProject* parent, char* path);
