#pragma once
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/tokens.h"

typedef struct Project {
    char* directory;
    Token* tokens;
    char** sourceFiles;
    char** souceFileNames;
    u64 sourceFileCount;
    Token** functionStarts;
    u64 functionStartCount;
    Arena arena;
} Project;

Project createProject(const char* folder);
