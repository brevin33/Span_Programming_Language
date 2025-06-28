#pragma once
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/tokens.h"
#include "parser/type.h"

typedef struct _SourceCode {
    char* filename;

    char* content;

    char** lines;
    u64* lineLengths;
    u64 lineCount;

    Token* tokens;

    Token** functionStarts;
    u64 functionStartCount;

    Token** typeDeclarations;
    u64 typeDeclarationCount;

    typeId* types;
    u64 typeCount;

    functionId* functions;
    u64 functionCount;

    Arena* arena;
} SourceCode;

extern Pool sourceCodePool;

typedef u64 sourceCodeId;

SourceCode* getSourceCodeFromId(sourceCodeId sourceCodeId);

void protoTypeFunctions(sourceCodeId sourceCodeId);

void protoTypeTypes(sourceCodeId sourceCodeId, projectId projectId);

void implementTypes(sourceCodeId sourceCodeId, projectId projectId);

void freeSourceCode(sourceCodeId sourceCodeId);

void reloadSourceCode(sourceCodeId sourceCodeId);

sourceCodeId createSourceCode(const char* filename);

void implementSourceCodeFunctions(sourceCodeId sourceCodeId);
