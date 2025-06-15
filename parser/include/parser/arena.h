#pragma once
#include "nice_ints.h"


typedef struct _Arena {
    void** start;
    u64 blockCapacity;
    u64 startIndex;
    u64 capacity;
    void* current;
} Arena;

Arena* createArena(u64 capacity);

void* arenaAlloc(Arena* arena, u64 size);

void arenaReset(Arena* arena);

void arenaFree(Arena* arena);
