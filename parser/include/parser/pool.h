#pragma once

#include "nice_ints.h"
#include "arena.h"

typedef struct _Pool {
    void* memory;
    u64 size;
    u64 capacity;
    u64 elementSize;

    u64* freeList;
    u64 freeListCount;
    u64 freeListCapacity;

    Arena* arena;
} Pool;

typedef u64 poolId;

Pool createPool(u64 elementSize, Arena* arena);

void* poolGetItem(Pool* pool, poolId poolId);
void freepoolId(Pool* pool, poolId poolId);
poolId poolNewItem(Pool* pool);
