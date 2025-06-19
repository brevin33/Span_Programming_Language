#pragma once

#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/tokens.h"
#include "parser/sourceCode.h"

extern Pool projectPool;

typedef struct _Project {
    char* directory;

    Arena* arena;

    sourceCodeId* sourceCodeIds;
    u64 sourceCodeCount;
    u64 sourceCodeCapacity;
} Project;

typedef u64 projectId;

projectId createProject(const char* directory);

Project* getProjectFromId(projectId projectId);

void freeProject(Project* project);
