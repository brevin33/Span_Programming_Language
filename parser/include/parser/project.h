#pragma once
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/tokens.h"
#include "parser/function.h"

typedef struct _Project {
    char* directory;
    Token* tokens;
    char** sourceFiles;
    char** souceFileNames;
    u64 sourceFileCount;
    Token** functionStarts;
    u64 functionStartCount;
    Function* functions;
    u64 functionCount;
    Arena arena;
} Project;

Project createProject(const char* folder);
