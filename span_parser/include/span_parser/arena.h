#pragma once
#include "default.h"

typedef struct _Arena* Arena;
struct _Arena {
    Arena parent;
    char** memory;
    u64 initialSize;
    u64 bufferIndex;
    u64 memoryIndex;
};


// nameing standed is for common operation lead with action then type
// more special lead with type then action
Arena createRootArena(u64 initialSize);
Arena createArena(Arena parent, u64 initialSize);

void* allocArena(Arena arena, u64 size);

void freeArena(Arena arena);

void arenaReset(Arena arena);

void* reallocArena(Arena arena, u64 size, void* original, u64 originalSize);
