#pragma once
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/tokens.h"

typedef struct _SourceCode {
    char* filename;

    char* content;

    char** lines;
    u64* lineLengths;
    u64 lineCount;

    Token* tokens;

    Token* fucntionStarts;

    Arena* arena;
} SourceCode;

extern Pool sourceCodePool;

typedef u64 sourceCodeId;

SourceCode* getSourceCodeFromId(sourceCodeId sourceCodeId);

void freeSourceCode(sourceCodeId sourceCodeId);

void reloadSourceCode(sourceCodeId sourceCodeId);

sourceCodeId createSourceCode(const char* filename);
