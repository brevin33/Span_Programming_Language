#pragma once
#include "nice_ints.h"


typedef struct _Arena {
    void** start;
    u64 blockCapacity;
    u64 startIndex;
    u64 capacity;
    void* current;
} Arena;

extern Arena* gArena;

Arena* createArena(u64 capacity);

void* arenaAlloc(Arena* arena, u64 size);

void* arenaRealloc(Arena* arena, void* memory, u64 oldSize, u64 size);

void arenaReset(Arena* arena);

void freeArena(Arena* arena);
