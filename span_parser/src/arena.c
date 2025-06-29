#include "span_parser.h"

static const u64 maxArenaMemoryAllocations = 128;

static u64 getMemoryAllocationSize(u64 initialSize, u64 bufferIndex) {
    return initialSize * (bufferIndex + 1);
}

static void* allocInSideArena(Arena arena, u64 size) {
    if (arena->parent != NULL) {
        return allocArena(arena->parent, size);
    } else {
        return malloc(size);
    }
}

static Arena createArenaFromMemory(u64 initialSize, char* mem) {
    Arena arena = (Arena)mem;
    mem += sizeof(struct _Arena);
    arena->memory = (char**)mem;
    mem += sizeof(char*) * maxArenaMemoryAllocations;
    memset(arena->memory, 0, sizeof(char*) * maxArenaMemoryAllocations);
    arena->bufferIndex = 0;
    arena->initialSize = initialSize;
    arena->memoryIndex = 0;
    arena->memory[0] = mem;
    return arena;
}

Arena createRootArena(u64 initialSize) {
    char* mem = malloc(sizeof(struct _Arena) + initialSize + sizeof(char*) * maxArenaMemoryAllocations);
    Arena arena = createArenaFromMemory(initialSize, mem);
    arena->parent = NULL;
    return arena;
}

Arena createArena(Arena parent, u64 initialSize) {
    char* mem = allocArena(parent, sizeof(struct _Arena) + initialSize + sizeof(char*) * maxArenaMemoryAllocations);
    Arena arena = createArenaFromMemory(initialSize, mem);
    arena->parent = parent;
    return arena;
}

void* allocArena(Arena arena, u64 size) {
    u64 memoryAllocationSize = getMemoryAllocationSize(arena->initialSize, arena->bufferIndex);
    while (arena->memoryIndex + size > memoryAllocationSize) {
        arena->bufferIndex++;
        massert(arena->bufferIndex < maxArenaMemoryAllocations, "Arena buffer index overflow");
        // might already be allocated if arena got reset
        if (arena->memory[arena->bufferIndex] == NULL) {
            memoryAllocationSize = getMemoryAllocationSize(arena->initialSize, arena->bufferIndex);
            allocInSideArena(arena, memoryAllocationSize);
        }
        arena->memoryIndex = 0;
    }
    void* mem = arena->memory[arena->bufferIndex] + arena->memoryIndex;
    arena->memoryIndex += size;
    return mem;
}

void freeArena(Arena arena) {
    massert(arena->parent == NULL, "Cannot free arena with parent");
    for (u64 i = 1; i < maxArenaMemoryAllocations; i++) {
        if (arena->memory[i] != NULL) {
            free(arena->memory[i]);
        }
    }
    // the first allocation is the arena itself
    free(arena->memory[0]);
}

void arenaReset(Arena arena) {
    arena->bufferIndex = 0;
    arena->memoryIndex = 0;
}

void* reallocArena(Arena arena, u64 size, void* original, u64 originalSize) {
    void* mem = allocArena(arena, size);
    memcpy(mem, original, originalSize);
    return mem;
}
