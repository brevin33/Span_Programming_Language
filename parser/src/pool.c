#include "parser.h"
#include <string.h>

Pool createPool(u64 elementSize, Arena* arena) {
    Pool pool;
    pool.memory = arenaAlloc(arena, elementSize * 512);
    pool.size = 0;
    pool.capacity = 512;
    pool.elementSize = elementSize;

    pool.freeList = arenaAlloc(arena, sizeof(u64) * 512);
    pool.freeListCount = 0;
    pool.freeListCapacity = 512;

    pool.arena = arena;
    return pool;
}

poolId getIdForPool(Pool* pool) {
    if (pool->freeListCount == 0) {
        poolId id = pool->size++;
        if (id >= pool->capacity) {
            pool->memory = arenaRealloc(pool->arena, pool->memory, pool->elementSize * pool->capacity, pool->elementSize * pool->capacity * 2);
            pool->capacity *= 2;
        }
        return id;
    }
    poolId id = pool->freeList[--pool->freeListCount];
    return id;
}

void* poolGetItem(Pool* pool, poolId poolId) {
    assert(poolId != BAD_ID);
    return (char*)pool->memory + poolId * pool->elementSize;
}

void freepoolId(Pool* pool, poolId poolId) {
    if (pool->freeListCount >= pool->freeListCapacity) {
        pool->freeList = arenaRealloc(pool->arena, pool->freeList, sizeof(poolId) * pool->freeListCapacity, sizeof(poolId) * pool->freeListCapacity * 2);
        pool->freeListCapacity *= 2;
    }
    pool->freeList[pool->freeListCount++] = poolId;
}


poolId poolNewItem(Pool* pool) {
    poolId id = getIdForPool(pool);
    return id;
}
