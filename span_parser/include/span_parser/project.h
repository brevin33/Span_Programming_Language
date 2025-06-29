#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"

typedef struct _SpanFile {
    Arena arena;
    char* fileName;
    char* fileContents;
    char** fileLineStarts;
    u64 fileLineStartsCount;
    Token* tokens;
} SpanFile;

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex);

void getTokensForFile(SpanFile* file, u64 fileIndex);

u64 spanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer);

char* spanFileGetLine(SpanFile* file, u64 line, char* buffer);

typedef struct _SpanProject {
    Arena arena;  // arenas for all substantial data
    SpanFile* files;
    u64 fileCount;
} SpanProject;

char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount);
