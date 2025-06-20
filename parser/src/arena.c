#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Arena* gArena;

Arena* createArena(u64 capacity) {
    u64 totalSize = sizeof(Arena) + capacity;
    char* mem = malloc(totalSize);
    Arena* arena = (Arena*)mem;
    mem += sizeof(Arena);

    arena->start = (void**)malloc(8 * sizeof(void*));
    memset(arena->start, 0, 8 * sizeof(void*));
    arena->blockCapacity = 8;
    assert(arena->start != NULL && "Failed to allocate memory for arena start.");

    arena->startIndex = 0;
    arena->capacity = capacity;
    arena->start[0] = mem;
    assert(arena->start[0] != NULL && "Failed to allocate memory for arena.");
    arena->current = arena->start[0];
    return arena;
}

void* arenaRealloc(Arena* arena, void* memory, u64 oldSize, u64 size) {
    if (memory == NULL) {
        return arenaAlloc(arena, size);
    }
    void* newMemory = arenaAlloc(arena, size);
    memcpy(newMemory, memory, oldSize);
    return newMemory;
}

void* arenaAlloc(Arena* arena, u64 size) {
    size = (size + 7) & ~7;  // Align to 8 bytes

    assert(arena != NULL && "Arena pointer is NULL.");
    assert(size > 0 && "Size must be greater than 0.");

    u64 newSize = (u64)arena->current - (u64)arena->start[arena->startIndex] + size;
    u64 maxSize = (u64)arena->capacity * (arena->startIndex + 1);
    while (newSize > maxSize) {
        arena->startIndex++;
        if (arena->startIndex >= arena->blockCapacity) {
            arena->blockCapacity *= 2;
            arena->start = (void**)realloc(arena->start, arena->blockCapacity * sizeof(void*));
            memset(arena->start + arena->blockCapacity / 2, 0, (arena->blockCapacity / 2) * sizeof(void*));
            assert(arena->start != NULL && "Failed to reallocate memory for arena start.");
        }
        if (arena->start[arena->startIndex] == NULL) {
            arena->start[arena->startIndex] = malloc(arena->capacity * (arena->startIndex + 1));
            assert(arena->start[arena->startIndex] != NULL && "Failed to allocate memory for new arena block.");
        }
        arena->current = arena->start[arena->startIndex];
        newSize = (u64)arena->current - (u64)arena->start[arena->startIndex] + size;
        maxSize = (u64)arena->capacity * (arena->startIndex + 1);
    }
    void* allocatedMemory = arena->current;
    arena->current += size;
    return allocatedMemory;
}

void arenaReset(Arena* arena) {
    assert(arena != NULL && "Arena pointer is NULL.");
    arena->current = arena->start[0];
    arena->startIndex = 0;
}

void freeArena(Arena* arena) {
    assert(arena != NULL && "Arena pointer is NULL.");
    for (u64 i = 1; i < arena->blockCapacity; i++) {
        if (arena->start[i] != NULL) {
            free(arena->start[i]);
            arena->start[i] = NULL;
        }
    }
    void* mem = arena->start[0] - sizeof(Arena);
    free(arena->start);
    free(mem);
}
